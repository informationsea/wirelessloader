#include <avr/io.h>
#include <avr/boot.h>
#include <util/delay.h>
#include <util/crc16.h>
#include <avr/signature.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include "usart.h"
#include "common.h"

#define VERSION 0x0001

void fuse_read(void);
void show_version(void);
void erase_program(void);
void write_program(void);
void read_program(void);
void crc32_program(void);
void exit(void);

#define LENGTH(x) (sizeof(x)/sizeof(x[0]))

#if __AVR_ATmega328P__
#define ONCE_PAGENUM 4
#elif defined(__AVR_ATmega644P__)
#define ONCE_PAGENUM 4
#else
#define ONCE_PAGENUM 4
#endif

#define TIMEOUT 10 // (seconds)

LOCKBITS = BLB1_MODE_2;

int main(void)
{
    //boot_spm_interrupt_enable();
    uint32_t timeout_count = 0;
    volatile char reset = MCUSR;
    (void)reset;

    // Disable watch dog timer
    MCUSR = 0; // Clear reset flags
    cli();
    wdt_reset();
    wdt_disable();
    sei();

    usart_init(BAUDRATE);

    boot_lock_bits_set_safe(_BV(BLB11));

    for (timeout_count = 0; timeout_count < TIMEOUT*1000; timeout_count++) {
        if (UCSR0A & _BV(RXC0)) {
            goto next;
        }
        _delay_ms(1);
    }
    
    exit();

  next:

    while(1) {
        usart_putc(CODE_READY);
        char ch = usart_getc();
        switch(ch) {
        case CODE_WRITE_PROGRAM:
            write_program();
            break;
        case CODE_READ_PROGRAM:
            read_program();
            break;
        case CODE_READ_INFORMATION:
            fuse_read();
            break;
        case CODE_CRC:
            crc32_program();
            break;
        case CODE_EXIT:
            exit();
            break;
        case CODE_READ_VERSION:
            show_version();
            break;
        case CODE_ERASE_PROGRAM:
            erase_program();
            break;
        case '\r': // ignore
        case 0x03:
            break;
        default:
            usart_putc(CODE_UNKNOWN);
            break;
        }
    }

    while(1);
}

void exit(void)
{
    usart_putc(CODE_EXIT);
    _delay_ms(10);
    boot_rww_enable_safe();
    _delay_ms(10);
    // jump to reset vector
    asm volatile ("jmp 0\n");
}

uint8_t hex_to_byte(uint8_t first, uint8_t second)
{
    uint8_t byte = 0;
    if (isdigit(first)) {
        byte = first - '0';
    } else if (isupper(first)) {
        byte = first - 'A' + 10;
    } else if (islower(first)) {
        byte = first - 'a' + 10;
    }

    byte <<= 4;

    if (isdigit(second)) {
        byte += second - '0';
    } else if (isupper(second)) {
        byte += second - 'A' + 10;
    } else if (islower(second)) {
        byte += second - 'a' + 10;
    }

    return byte;
}


void fuse_read(void)
{
    usart_putc(CODE_READ_INFORMATION);
    usart_puthex(boot_lock_fuse_bits_get(GET_LOW_FUSE_BITS));
    usart_puthex(boot_lock_fuse_bits_get(GET_HIGH_FUSE_BITS));
    usart_puthex(boot_lock_fuse_bits_get(GET_EXTENDED_FUSE_BITS));
    usart_puthex(boot_lock_fuse_bits_get(GET_LOCK_BITS));
#ifdef SIGNATURE_0
    usart_puthex(SIGNATURE_0);
#else
    usart_puthex(boot_signature_byte_get(0x00));
#endif
#ifdef SIGNATURE_1
    usart_puthex(SIGNATURE_1);
#else
    usart_puthex(boot_signature_byte_get(0x02));
#endif
#ifdef SIGNATURE_2
    usart_puthex(SIGNATURE_2);
#else
    usart_puthex(boot_signature_byte_get(0x04));
#endif
    usart_puthex(ONCE_PAGENUM);
}

void show_version(void)
{
    usart_putc(CODE_READ_VERSION);
    usart_puthex((VERSION >> 8) & 0xff);
    usart_puthex(VERSION& 0xff);
}

#define HEADER_LENGTH (4+1) // address + checksum

void write_program(void)
{
    uint16_t i, j;
    uint8_t k;
    uint32_t address[ONCE_PAGENUM];
    uint8_t buffer[ONCE_PAGENUM][SPM_PAGESIZE];
    bool last_write = false;

    uint8_t buffer2[ONCE_PAGENUM][SPM_PAGESIZE+HEADER_LENGTH];

    for (i = 0; i < LENGTH(address); i++)
        address[i] = 0;
    for (i = 0; i < LENGTH(buffer); i++)
        memset(buffer[i], 0xff, LENGTH(buffer[0]));

    while (1) {
        uint8_t will_write_pagenum = 0;
        uint8_t *buffer2_pointer = buffer2[0];
        for (k = 0; k < ONCE_PAGENUM; ++k) {
            uint8_t receive = usart_getc();
            switch (receive) {
            case CODE_CONTINE:
                for (i = 0; i < SPM_PAGESIZE + HEADER_LENGTH; ++i, buffer2_pointer++) {
                    *buffer2_pointer = usart_getc();
                }
                continue;
            case CODE_END:
                last_write = true;
                goto dowrite;
            default:
                usart_putc(CODE_ERROR);
                return;
            }

#if 0
            uint8_t receive;
            uint8_t checksum = 0;
            receive = usart_getc();

            switch(receive) {
            case CODE_CONTINE:
                break;
            case CODE_END:
                last_write = true;
                goto dowrite;
            default:
                usart_putc(CODE_ERROR);
                return;
            }

            for (i = 0; i < 4; ++i) {
                receive = usart_getc();
                checksum += receive;
                address[k] <<= 8;
                address[k] |= receive;
            }
            //boot_page_erase_safe(address[0]);
            for (i = 0; i < SPM_PAGESIZE; ++i) {
                receive = usart_getc();
                checksum += receive;
                buffer[k][i] = receive;
                // address[0] += 1;
            }
            checksum += usart_getc();
            if (checksum) {
                usart_putc(CODE_ERROR);
                usart_putc(CODE_ERROR_CHECKSUM);
                return;
            }
            //PORTA = k+1;
#endif
        }

        dowrite:
        will_write_pagenum = k;
        for (k = 0; k < will_write_pagenum; ++k) {
            uint8_t *buffer2_pointer = buffer2[k];
            uint8_t checksum = 0;
            uint32_t address = 0;

            for (i = 0; i < SPM_PAGESIZE + HEADER_LENGTH; ++i) {
                checksum += *buffer2_pointer++;
            }

            if (checksum) {
                usart_putc(CODE_ERROR_CHECKSUM);
                usart_putc(CODE_ERROR);
                return;
            }

            buffer2_pointer = buffer2[k];

            for (i = 0; i < 4; i++) {
                address <<= 8;
                address |= *buffer2_pointer++;
            }

            if (address % SPM_PAGESIZE) {
                usart_putc(CODE_ERROR_WRONG_PAGESIZE);
                usart_putc(CODE_ERROR);
                return;
            }
            if (address > BOOTSTART) {
                usart_putc(CODE_ERROR_OUT_OF_RANGE);
                usart_putc(CODE_ERROR);
                return;
            }

            //boot_page_erase_safe(address);
            for (j = 0; j < SPM_PAGESIZE; j += 2) {
                union{
                    uint8_t bytes[2];
                    uint16_t data;
                } onedata;
                onedata.bytes[0] = *(buffer2_pointer++);
                onedata.bytes[1] = *(buffer2_pointer++);
                boot_page_fill_safe(address + j, onedata.data);
            }
            boot_page_write_safe(address);
            boot_rww_enable_safe();

#if 1
            for (j = 0; j < SPM_PAGESIZE; j++) {
                uint8_t byte = pgm_read_byte(address+j);
                if (byte != buffer2[k][j+4]) {
                    usart_puthex(address >> 8);
                    usart_puthex(address);
                    usart_puthex(j);
                    usart_puthex(byte);
                    usart_puthex(buffer2[k][j+4]);
                    usart_putc(CODE_ERROR);
                    return;
                }
            }
#endif
        }

        usart_putc(CODE_OK);
        if (last_write)
            return;
    }
}

void erase_program(void)
{
    size_t address;
    usart_putc(CODE_ERASE_PROGRAM);
    for (address = 0; address < BOOTSTART; address += SPM_PAGESIZE) {
        boot_page_erase_safe(address);
    }
    usart_putc(CODE_OK);
}

void crc32_program(void)
{
    size_t address;
    uint16_t crc = 0;
    usart_putc(CODE_CRC);
    usart_puthex(((BOOTSTART) >> 8) & 0xff);
    usart_puthex(((BOOTSTART) >> 0) & 0xff);
    boot_rww_enable_safe();
    for (address = 0; address < BOOTSTART; ++address) {
        crc = _crc16_update(crc, pgm_read_byte(address));
    }
    usart_puthex((crc >> 8) & 0xff);
    usart_puthex((crc >> 0) & 0xff);
}

void read_program(void) {
    uint8_t buffer[SPM_PAGESIZE];
    size_t address;
    for (address = 0; address < BOOTSTART; address += SPM_PAGESIZE) {
        size_t i;
        bool should_transrate = false;
        for (i = 0; i < SPM_PAGESIZE; ++i) {
            buffer[i] = pgm_read_byte(address + i);
            if (buffer[i] != 0xff)
                should_transrate = true;
        }
        if (should_transrate) {
            uint8_t checksum = 0;
            int8_t j;
            usart_putc(CODE_CONTINE);
            for (j = 3; j >= 0; --j) {
                usart_putc((address >> (8*j)) & 0xff);
                checksum += (address >> (8*j)) & 0xff;
            }
            for (i = 0; i < SPM_PAGESIZE; ++i) {
                usart_putc(buffer[i]);
                checksum += buffer[i];
            }
            usart_putc(checksum);
        }
    }
    usart_putc(CODE_OK);
}
