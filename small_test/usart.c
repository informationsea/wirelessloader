/*
 *   USART Library for AVR Series
 *   Copyright (C) 2012 Y.Okamura
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "usart.h"

#ifdef USART_SOFTWARE_RESET
volatile char usart_received;
static volatile char receive_data;

#ifdef USART0_RX_vect
#define USART_RX_VECTOR USART0_RX_vect
#define USART_TX_VECTOR USART0_UDRE_vect
#else
#define USART_RX_VECTOR USART_RX_vect
#define USART_TX_VECTOR USART_UDRE_vect
#endif

ISR(USART_RX_VECTOR)
{
    receive_data = UDR0;

    switch (receive_data) {
    case 0x03:
        cli();
        wdt_reset();
        wdt_enable(WDTO_15MS);
        while(1);
        break;
    default:
        usart_received = 1;
        break;
    }
}
#endif

#ifdef USART_PRINTF
FILE mystdout = FDEV_SETUP_STREAM(usart_putc2, NULL,
                                  _FDEV_SETUP_WRITE);
#endif


void usart_putc(const char data)
{
    loop_until_bit_is_set(UCSR0A,UDRE0);
    UDR0 = data; // Transmit
}

char usart_getc(void)
{
#ifndef USART_SOFTWARE_RESET
    loop_until_bit_is_set(UCSR0A,RXC0);
    return UDR0;
#else
    while (!usart_received)
        ;
    usart_received = 0;
    return receive_data;
#endif
}

void usart_putstr(const char* str)
{
    while (*str) {
        usart_putc(*str++);
    }
}

#ifdef USART_SUPPORT_PROGMEM
void usart_putstr_P(PGM_P str)
{
    char ch;
    while((ch = pgm_read_byte(str++)) != '\0') {
        usart_putc(ch);
    }
}
#endif

#ifdef USART_GETSTR
size_t usart_getstr_s(char* str, size_t len)
{
    uint16_t i;
    for (i = 0; i < len - 1; i++) {
        char ch = usart_getc();
        switch (ch) {
        case '\r':
            goto finish;
        case 0x1b:
            str -= i;
            i = 0;
            goto finish;
        default:
            usart_putc(ch);
            *str++ = ch;
            break;
        }
    }
  finish: *str = 0;
    usart_putc('\r');
    usart_putc('\n');
    return i;
}

#endif

#ifdef USART_GETI
USART_INTEGER usart_geti()
{
    long i = 0;
#ifdef USART_SUPPORT_SIGNED
    int8_t sign = 1;
#endif
    while (1) {
        char ch = usart_getc();
        if ('0' <= ch && ch <= '9') { // Accept only digits
            i = i * 10 + (ch - '0');
            usart_putc(ch);
        } else if (ch == '\r') { // Exit with return code
            usart_putc('\r');
            usart_putc('\n');
#ifdef USART_SUPPORT_SIGNED
            return i * sign;
#else
            return i;
#endif
#ifdef USART_SUPPORT_SIGNED
        } else if (ch == '-') { // negative number
            usart_putc(ch);
            sign = -1;
        } else if (ch == '+') { // positive number
            usart_putc(ch);
            sign = +1;
#endif
        } else if (ch == 0x1b) { // cancel
            usart_putc('\r');
            usart_putc('\n');
            usart_putc('>');
            i = 0;
#ifdef USART_SUPPORT_SIGNED
            sign = 1;
#endif
        }
    }
    return 0;
}
#endif

#ifdef USART_PUTI
void usart_puti(USART_INTEGER n)
{
    char str[10]; // Maximum length of string is 10
    int i = 0, j;

#ifdef USART_SIGNED_SUPPORT
    if(n < 0){
        usart_putc('-');
        n = -n;
    }
#endif

    while (n > 0) {
        str[i++] = n % 10 + '0';
        n /= 10;
    }

    for (j = i - 1; 0 <= j; j--) {
        usart_putc(str[j]);
    }

    if (i == 0) {
        usart_putc('0');
    }
}
#endif

#ifdef USART_PUTHEX

void usart_puthex(unsigned char ch)
{
    unsigned char up = (ch >> 4) & 0xf;
    unsigned char down = ch & 0xf;

    if (up < 10) {
        usart_putc(up + '0');
    } else {
        usart_putc(up + 'A' - 10);
    }

    if (down < 10) {
        usart_putc(down + '0');
    } else {
        usart_putc(down + 'A' - 10);
    }
}

#endif

#ifdef USART_PRINTF

int usart_putc2(char c, FILE *stream)
{
    if (c == '\n')
        usart_putc('\r');
    usart_putc(c);
    return 0;
}

#endif
