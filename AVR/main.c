#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "headers/portpins.h"
#include "headers/USART.h"
#include "headers/pinDefines.h"


ISR(TIMER1_COMPA_vect) {
    PORTB ^= (1 << PB5);
    printString("toggled!\r\n");
}


static inline void initTimer1() {
    TCCR1A = 0;
    TCCR1B = 0;
    // 16 000 000 (16 million) / 1024 (clock prescaler set below)
    // = 15624 ticks per second
    OCR1A = 15624;

    // put the clock in ctc mode and set the prescaler to 1024
    TCCR1B |= (1 << CS10) | (1 << CS12) | (1 << WGM12);

    TIMSK1 |= (1 << OCIE1A);

    sei();
}

int main(void) {
    DDRB |= (1 << PB5);
    initUSART();
    initTimer1();

    while (1) {}

    return(0);
}
