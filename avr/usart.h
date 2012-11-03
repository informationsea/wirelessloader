/**
 * @file usart.h
 * @brief USART module for Atmel AVR
 * @author informationsea
 */

#ifndef __USART_H__
#define __USART_H__

#include <avr/io.h>

#define DISABLE_DOUBLE_BOADRATE

/**
 * @brief Initialize USART
 * @parameter baud baud rate
 * @warning DO NOT pass variable as an argument.
 */
static inline void usart_init(uint32_t baud)
{
#ifdef DISABLE_DOUBLE_BOADRATE
    uint16_t ubrr = (unsigned int)((double)F_CPU / 16 / baud - 0.5);
#else
    uint16_t ubrr = (unsigned int)((double)F_CPU / 8 / baud - 0.5);
#endif

    UBRR0H = (uint8_t) (ubrr >> 8);
    UBRR0L = (uint8_t) ubrr;
#ifndef DISABLE_DOUBLE_BOADRATE
    UCSR0A = _BV(U2X0);
#else
    UCSR0A = 0;
#endif
    UCSR0B = _BV(RXEN0) | _BV(TXEN0); // Permit sending and receiving
}

/**
 * @brief Send one character
 */
void usart_putc(const char data);

/**
 * @brief Receive one character
 */
char usart_getc(void);

/**
 * @brief Send one byte as hexadecimal
 */
void usart_puthex(const char ch);

#endif
