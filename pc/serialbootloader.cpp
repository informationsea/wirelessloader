#include "serialbootloader.h"

#include <stdio.h>
#include <string.h>


#ifdef __WIN32__

#else
#include <unistd.h>
#endif

#include "debug.h"
#include "../avr/common.h"

SerialBootloaderWrtier::SerialBootloaderWrtier(const char* port, uint32_t speed) :
_serialport(port, speed)
{
    _port = port;
    _speed = speed;
    memset(_signature, 0xff, sizeof(_signature));
    memset(_fuse, 0xff, sizeof(_fuse));
    _lockbits = 0xff;
    _once_pagenum = 1;
}

SerialBootloaderWrtier::~SerialBootloaderWrtier()
{
    if (_serialport.is_opened())
        _serialport.close();
}

bool SerialBootloaderWrtier::open()
{
    if(!_serialport.open())
        return false;

    /*
    char trash[1024];
    _serialport.read(trash, sizeof(trash), 0);
     */
    _serialport.writec(0x03);
#ifdef __WIN32__
    Sleep(300);
#else
    usleep(1000*300);
#endif
    _serialport.writec(CODE_READ_VERSION);

    char buf[128];
    unsigned int version = 0xffff;
    memset(buf, 0, sizeof(buf));
    for (uint8_t try_num = 0; try_num < 10; ++try_num) {
        fprintf(stderr, ".");
        int ch = _serialport.readc(1000);
        if (ch == CODE_READ_VERSION) {
            SSIZE_t readbytes = _serialport.read(buf, 4, 1000, true);
            if (readbytes == 4) {
                sscanf(buf, "%4x", &version);
                DEBUGF("Receive %s\n", buf);
                break;
            } else {
                DEBUGF("Cannot read bytes\n");
                fprintf(stderr, "Cannot receive the serial bootloader version. Please confirm that serial bootloader is installed.\n");
                close();
                return false;
            }
        } else if (ch < 0 && ch == CODE_ERROR) {
            DEBUGF("Read error or Receive error code\n");
            fprintf(stderr, "Cannot receive the serial bootloader version. Please confirm that serial bootloader is installed.\n");
            close();
            return false;
        } else {
            DEBUGF("Reading... %c\n", ch);
            if ((try_num % 4) == 3) {
                _serialport.writec(0x03);
#ifdef __WIN32__
                Sleep(100);
#else
                usleep(1000*100);
#endif
                _serialport.writec(CODE_READ_VERSION);
            }
        }
    }

    if (version == 0xffff) {
        DEBUGF("Timeout\n");
        fprintf(stderr, "Cannot receive the serial bootloader version. Please confirm that serial bootloader is installed.\n");
        close();
        return false;
    }

    if (version != 0x0001) {
        fprintf(stderr, "This bootloader version is not supported. %04x\n", version);
        close();
        return false;
    }

    fprintf(stderr, " Serial bootloader version %04x\n", version);


    _serialport.writec(CODE_READ_INFORMATION);
    for (uint8_t try_num = 0; try_num < 3; ++try_num) {
        int ch = _serialport.readc(1000);
        if (ch == CODE_READ_INFORMATION) {
            SSIZE_t readbytes = _serialport.read(buf, 16, 1000, true);
            if (readbytes == 16) {
                unsigned int bytes[8];
                int reads = sscanf(buf, "%2x%2x%2x%2x%2x%2x%2x%2x", &bytes[0], &bytes[1], &bytes[2], &bytes[3], &bytes[4], &bytes[5], &bytes[6], &bytes[7]);
                _fuse[0]      = bytes[0];
                _fuse[1]      = bytes[1];
                _fuse[2]      = bytes[2];
                _lockbits     = bytes[3];
                _signature[0] = bytes[4];
                _signature[1] = bytes[5];
                _signature[2] = bytes[6];
                _once_pagenum = bytes[7];

                DEBUGF("Receive %s %d\n", buf, _once_pagenum);
                if (reads == 8)
                    goto onreceivefuse;
            } else {
                fprintf(stderr, "Cannot receive fuse and lockbits, signature\n");
                close();
                return false;
            }
        } else if ((ch < 0 || ch == CODE_ERROR) && !_serialport.is_timeout()) {
            fprintf(stderr, "Cannot receive fuse and lockbits, signature\n");
            close();
            return false;
        }
    }
    fprintf(stderr, "Cannot receive fuse and lockbits, signature\n");
    close();
    return false;
    onreceivefuse:
    _avrinfo = AVRInfo::get_info((const uint8_t*) _signature);
    if (_avrinfo == 0) {
        fprintf(stderr, "Error: Unknown device: signature %02x %02x %02x\n", _signature[0], _signature[1], _signature[2]);
        return false;
    }
    fprintf(stderr, "Device: %s\n", _avrinfo->name());

    return true;
}

bool SerialBootloaderWrtier::close()
{
    if (_serialport.is_opened())
        return _serialport.close();
    return true;
}

bool SerialBootloaderWrtier::get_signature_bytes(uint8_t buf[3])
{
    if (!_serialport.is_opened())
        return false;
    memcpy(_signature, buf, sizeof(_signature));
    return true;
}

bool SerialBootloaderWrtier::get_fuse_bytes(uint8_t buf[3])
{
    if (!_serialport.is_opened())
        return false;
    memcpy(_fuse, buf, sizeof(_fuse));
    return true;
}

bool SerialBootloaderWrtier::get_lock_bits(uint8_t* lock)
{
    if (!_serialport.is_opened())
        return false;
    *lock = _lockbits;
    return true;
}

bool SerialBootloaderWrtier::erase_program(void)
{
    if (!_serialport.is_opened())
        return false;
    _serialport.writec(CODE_ERASE_PROGRAM);

    DEBUGF("Erasing...\n");

    while(1) {
        int ch = _serialport.readc(1000);
        switch(ch) {
        case CODE_OK:
            DEBUGF("OK\n");
            return true;
        case CODE_ERROR:
            DEBUGF("Error\n");
            return false;
        default:
            DEBUGF("Read: %c %02x\n", ch, ch);
            break;
        }
    }
    return true;
}

bool SerialBootloaderWrtier::write_program(const AVRProgram *program)
{
    if (!_serialport.is_opened())
        return false;
    const uint8_t *buffer = program->buffer();
    bool write_first = true;
    uint8_t wrote_page_num = 0;

    uint16_t lastpage = 0;
    for (uint16_t pagenum = 0; pagenum < _avrinfo->pagenum(); ++pagenum) {
        for (uint16_t i = 0; i < _avrinfo->pagesize(); ++i) {
            if (buffer[pagenum*_avrinfo->pagesize() + i] != 0xff)
                lastpage = pagenum;
        }
    }

    for (uint16_t pagenum = 0; pagenum < _avrinfo->pagenum(); ++pagenum) {
        bool should_write = false;
        for (uint16_t i = 0; i < _avrinfo->pagesize(); ++i) {
            if (buffer[pagenum*_avrinfo->pagesize() + i] != 0xff)
                should_write = true;
        }
        if (should_write) {
            uint8_t progress = pagenum*100/lastpage;
            fprintf(stderr, "\r%d%%  ", progress);

            if (write_first) { // send write code and receive ack
                _serialport.writec(CODE_WRITE_PROGRAM);
                char ch = _serialport.readc(1000);
                DEBUGF("Receive code %02x %c\n", ch, ch);
                if (ch != CODE_WRITE_PROGRAM && ch != CODE_READY) {
                    if (ch < 0)
                        _serialport.perror("Cannot receive echo code.");
                    else
                        fprintf(stderr, "Unknown echo %c\n", ch);
                    return false;
                }
                write_first = false;
            }

            // continue sending...
            _serialport.writec(CODE_CONTINE);
            uint32_t addr = pagenum*_avrinfo->pagesize();
            uint8_t addr_bytes[4] = {addr >> 24, addr >> 16, addr >> 8, addr};
            SSIZE_t wrote1 = _serialport.write(addr_bytes, sizeof(addr_bytes));
            SSIZE_t wrote2 = _serialport.write(buffer + addr, _avrinfo->pagesize());

            uint8_t checksum = 0;
            for (uint8_t i = 0; i < sizeof(addr_bytes); i++)
                checksum += addr_bytes[i];
            for (uint32_t i = 0; i < _avrinfo->pagesize(); ++i) {
                checksum += buffer[pagenum*_avrinfo->pagesize() + i];
            }
            checksum *= -1;
            _serialport.write(&checksum, 1);

            wrote_page_num += 1;
            DEBUGF("Write %x %u\n", addr, wrote_page_num);

            if (!wrote1 || !wrote2) {
                fprintf(stderr, "Cannot write to serial port\n");
                return false;
            }

            if (wrote_page_num >= _once_pagenum) {
                if (!write_program_helper(addr)) {
                    return false;
                } else {
                    wrote_page_num = 0;
                    DEBUGF("OK\n");
                }
            }
            //usleep(10000000);
        }
    }


    fprintf(stderr, "\r100%%  \n");

    DEBUGF("Send CODE_END\n");
    _serialport.writec(CODE_END);

    DEBUGF("wrote_page_num = %d\n", wrote_page_num);
    if (wrote_page_num && !write_program_helper(0)) {
        return false;
    }

    return true;
}


bool SerialBootloaderWrtier::write_program_helper(uint32_t addr)
{
    DEBUGF("%s called with address %x\n", __func__, addr);
    for(int trynum = 0; trynum < 10; trynum++) {
        int ch = _serialport.readc(1000);
        if (ch == 'O') {
            DEBUGF("Wrote %x %c\n", addr, ch);
            return true;
        } else if ((ch < 0 || ch == CODE_ERROR) && !_serialport.is_timeout()) {
            DEBUGF("Error %x %c\n", addr, ch);
            if (ch < 0) {
                _serialport.perror("Error while writing");
            } else {
                fprintf(stderr, "Unknown error on writing\n");
            }
            _serialport.writec(CODE_END);
            return false;
        } else if (ch < 0 && _serialport.is_timeout()) {
            trynum += 3;
            DEBUGF("Waiting..\n");
        } else {
            DEBUGF("Read: %c\n", ch);
        }
    }
    fprintf(stderr, "\nTimeout\n");
    return false;
}


bool SerialBootloaderWrtier::read_program(AVRProgram *program)
{
    if (!_serialport.is_opened())
        return false;

    size_t buffer_size = _avrinfo->pagenum() * _avrinfo->pagesize();
    uint8_t *buffer = (uint8_t *)malloc(buffer_size);
    memset(buffer, 0xff, buffer_size);
    if (!_serialport.writec(CODE_READ_PROGRAM)) {
        _serialport.perror("Cannot write to serial port");
        return false;
    }

    do {
        int ch = _serialport.readc(1000);
        if (ch < 0) {
            _serialport.perror("Cannot read.");
            return false;
        }
        DEBUGF("Read character %c\n", ch);
        switch(ch) {
        case CODE_READY:
            break;
        case CODE_CONTINE: {
            uint8_t address_array[4];
            uint32_t address = 0;
            uint8_t checksum, local_checksum = 0;
            int i;
            _serialport.read(address_array, sizeof(address_array), 1000, true);
            for (i = 0; i < 4; i++) {
                address |= (address_array[i]) << (24-8*i);
                local_checksum += address_array[i];
            }

            DEBUGF("Read Address %x\n", address);

            if (address + _avrinfo->pagesize() > buffer_size) {
                fprintf(stderr, "Out of range (Address %x)\n", address);
                return false;
            }

            if (!_serialport.read(buffer+address, _avrinfo->pagesize(), 1000, true)) {
                _serialport.perror("Cannot read data.");
                return false;
            }

            for (i = 0; i < _avrinfo->pagesize(); ++i) {
                local_checksum += *(buffer+address+i);
            }

            checksum = _serialport.readc(1000);
            DEBUGF("Checksum %02x %02x\n", checksum, local_checksum);
            if (checksum != local_checksum) {
                fprintf(stderr, "Checksum error\n");
                return false;
            }

            break;
        }
        case CODE_OK: {
            program->set(buffer, buffer_size);
            return true;
        }
        default:
            DEBUGF("Unknown response %c\n", ch);
            fprintf(stderr, "Unknown response.\n");
            return false;
            break;
        }

    } while(1);

    return false;
}

// copied from avr-libc
uint16_t crc16_update(uint16_t crc, uint8_t a)
{
    uint8_t i;

    crc ^= a;
    for (i = 0; i < 8; ++i)
    {
        if (crc & 1)
            crc = (crc >> 1) ^ 0xA001;
        else
            crc = (crc >> 1);
    }

    return crc;
}

bool SerialBootloaderWrtier::verify_program(const AVRProgram *program)
{
    if (!_serialport.is_opened())
        return false;
    _serialport.writec(CODE_CRC);
    char readbuf[128];
    memset(readbuf, 0, sizeof(readbuf));
    SSIZE_t readlen;

    for (uint8_t try_num = 0; try_num < 10; ++try_num) {
        int ch = _serialport.readc(1000);
        if (ch == CODE_CRC) {
            readlen = _serialport.read(readbuf, 8, 1000, true);
            if (readlen != 8) {
                DEBUGF("Receive length error %d\n", readlen);
                fprintf(stderr, "Error: cannot read CRC\n");
                return false;
            }
            goto doverify;
        } else if (((ch < 0 || ch == CODE_ERROR) && !_serialport.is_timeout())) {
            DEBUGF("Error code %d\n", ch);
            fprintf(stderr, "Error: cannot read CRC\n");
            return false;
        } else if (ch < 0 && _serialport.is_timeout()) {
            try_num += 3;
            DEBUGF("Waiting...\n");
        } else {
            DEBUGF("Unknown code %d %c\n", ch, ch);
        }
    }
    DEBUGF("Cannot read CRC\n");
    fprintf(stderr, "Error: cannot read CRC\n");
    return false;


    doverify:
    unsigned int bootloader_start;
    unsigned int crc16_rom;
    size_t reads = sscanf(readbuf, "%4x%4x", &bootloader_start, &crc16_rom);
    if (reads != 2) {
        fprintf(stderr, "Error: cannot read CRC\n");
        return false;
    }

    DEBUGF("Verify : %04x %04x %s\n", bootloader_start, crc16_rom, readbuf);

    const uint8_t *buffer = program->buffer();
    uint32_t address;
    uint16_t crc16 = 0;
    for (address = 0; address < bootloader_start; ++address) {
        crc16 = crc16_update(crc16, buffer[address]);
    }
    DEBUGF("CRC: %04x\n", crc16);
    if (crc16 == crc16_rom)
        return true;
    fprintf(stderr, "Verify failed.\n");
    return false;
}

void SerialBootloaderWrtier::run(void)
{
    int ch, i;
    if (!_serialport.is_opened())
        return;
    DEBUGF("Exit code\n");
    if (!_serialport.writec(CODE_EXIT)) {
        _serialport.perror("Cannot send exit code.");
        return;
    }
    for(i = 0; i < 3; i++) {
        ch = _serialport.readc(1000);
        if (ch < 0) {
            _serialport.perror("Cannot confirm exit code.");
            return;
        }
        switch (ch) {
            case CODE_EXIT:
                DEBUGF("Exit successfully.\n");
                return;
            case CODE_READY:
                break;
            default:
                DEBUGF("Unknown code received %c\n", ch);
                break;
        }
    }

    fprintf(stderr, "Cannot confirm exit code.");
}


