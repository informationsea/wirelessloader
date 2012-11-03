#include <avr/io.h>
#include <avr/boot.h>
#include <util/delay.h>
#include <avr/signature.h>
#include <avr/pgmspace.h>
#include <stdint.h>
#include <stdbool.h>
#include <avr/eeprom.h>

#include <usart.h>

int main(void)
{
    usart_init_38400();
    
    DDRA = 0x0f;
    PORTA = 0x0f;

    usart_putstr_P(PSTR("Small Test\r\n"));
    while(1) {
        uint8_t i;
        for (i = 0; i < 4; i++) {
            _delay_ms(200);
            PINA = 1 << i;
        }
    }
}
