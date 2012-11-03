/**
 * @file usart.c
 */
#include "usart.h"

void usart_putc(const char data)
{
    loop_until_bit_is_set(UCSR0A,UDRE0);
    UDR0 = data;
}

char usart_getc(void)
{
    loop_until_bit_is_set(UCSR0A,RXC0);
    return UDR0;
}

void usart_puthex(const char ch)
{
    char up = (ch >> 4) & 0xf;
    char down = ch & 0xf;

    if (up < 10) {
        usart_putc(up + '0');
    } else {
        usart_putc(up + 'a' - 10);
    }

    if (down < 10) {
        usart_putc(down + '0');
    } else {
        usart_putc(down + 'a' - 10);
    }
}

