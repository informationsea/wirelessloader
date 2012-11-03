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


#ifndef _USART_H_
#define _USART_H_

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdint.h>

#ifdef UBRRH // Support Old AVR
#define UBRR0H UBRRH
#define UBRR0L UBRRL
#define UCSR0A UCSRA
#define U2X0   U2X
#define UCSR0B UCSRB
#define RXEN0  RXEN
#define TXEN0  TXEN
#define UDRE0  UDRE
#define UDR0   UDR
#define RXC0   RXC
#endif

#ifdef USART_PRINTF
extern FILE mystdout;
#endif

#ifdef USART_SOFTWARE_RESET
#include <avr/wdt.h>
extern volatile char usart_received;
#endif

/**
 * @brief Initialize USART with baudrate 9606
 * @note If you want to use not standard baudrate, you have to edit usart.h
 */
static inline void usart_init_common(void)
{
#ifndef USART_SOFTWARE_RESET
    UCSR0B = _BV(RXEN0) | _BV(TXEN0); // Enable receiver and transmitter
#else
    UCSR0B = _BV(RXCIE0) | _BV(RXEN0) | _BV(TXEN0); // Enable receiver and transmitter, receiver interrupt
    usart_received = 0; // clear receive flag

    // Disable watch dog timer
    MCUSR = 0; // Clear reset flags
    cli();
    wdt_reset();
    wdt_disable();
    sei();
#endif

#ifdef USART_PRINTF
    stdout = &mystdout;
#endif

}

/**
 * @brief Initialize USART with baudrate 9606
 * @note If you want to use not standard baudrate, you have to edit usart.h
 */
static inline void usart_init_9600(void)
{
#define BAUD 9600
#include <util/setbaud.h>
    UBRR0H = UBRRH_VALUE;
    UBRR0L = UBRRL_VALUE;
#if USE_2X
    UCSR0A |= (1 << U2X0);
#else
    UCSR0A &= ~(1 << U2X0);
#endif
    usart_init_common();
}

/**
 * @brief Initialize USART with baudrate 38400
 * @note If you want to use not standard baudrate, you have to edit usart.h
 */
static inline void usart_init_38400(void)
{
#undef BAUD  // avoid compiler warning
#define BAUD 38400
#include <util/setbaud.h>
    UBRR0H = UBRRH_VALUE;
    UBRR0L = UBRRL_VALUE;
#if USE_2X
    UCSR0A |= (1 << U2X0);
#else
    UCSR0A &= ~(1 << U2X0);
#endif
    usart_init_common();
}

/**
 * @brief Initialize USART with baudrate 115200
 * @note If you want to use not standard baudrate, you have to edit usart.h
 */
static inline void usart_init_115200(void)
{
#undef BAUD  // avoid compiler warning
#define BAUD 115200
#include <util/setbaud.h>
    UBRR0H = UBRRH_VALUE;
    UBRR0L = UBRRL_VALUE;
#if USE_2X
    UCSR0A |= (1 << U2X0);
#else
    UCSR0A &= ~(1 << U2X0);
#endif
    usart_init_common();
}

/**
 * @brief Transmit one character
 * @param data one character
 */
void usart_putc(const char data);

/**
 * @brief Receive one character
 * @return a received character
 */
char usart_getc(void);

/**
 * @brief Transmit a string
 * @param str a string
 */
void usart_putstr(const char* str);

#ifdef USART_SUPPORT_PROGMEM
/**
 * @brief Transmit a string in the program memory
 * @param str a string in the program memory
 */
void usart_putstr_P(PGM_P str);
#endif

#ifdef USART_GETSTR
/**
 * @brief Receive a string
 * @param str a string buffer
 * @param len a length of the buffer
 */
size_t usart_getstr_s(char* str, size_t len);

/**
 * @brief Receive a string
 * @param str a string buffer
 */
#define usart_getstr(str) usart_getstr_s((str), sizeof(str))
#endif

#ifdef USART_SUPPORT_SIGNED
#define USART_INTEGER long
#else
#define USART_INTEGER unsigned long
#endif

#ifdef USART_GETI
/**
 * @brief Receive a long integer in human readable character
 * @return a long integer
 * @note if your want to use signed integer, you have to set USART_SUPPORT_SIGNED
 */
USART_INTEGER usart_geti(void);
#endif

#ifdef USART_PUTI
/**
 * @brief Transmit a long integer in human readable character
 * @param num a long integer
 * @note if your want to use signed integer, you have to set USART_SUPPORT_SIGNED
 */
void usart_puti(USART_INTEGER num);
#endif

#ifdef USART_PUTHEX
/**
 * @brief Transmit a byte integer in hexadecimal string
 * @param ch a byte integer
 */
void usart_puthex(unsigned char ch);
#endif

#ifdef USART_PRINTF
/**
 * @brief Transmit a character
 * @param c a character
 * @param stream File stream
 */
int usart_putc2(char c, FILE *stream);
#endif


#endif /* _USART_H_ */
