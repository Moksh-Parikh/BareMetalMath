#include <avr/io.h>
#include <avr/interrupt.h>

#include "headers/portpins.h"

ISR(TIMER1_COMPA_vect) {
    PORTB ^= (1 << PB5);
}

static inline void initTimer1() {
    TCCR1A = 0;
    TCCR1B = 0;
    OCR1A = 15624;

    TCCR1B |= (1 << CS10) | (1 << CS12) | (1 << WGM12);

    TIMSK1 |= (1 << OCIE1A);

    sei();
}

int main(void) {
    DDRB |= (1 << PB5);
    initTimer1();

    while (1) {}

    return(0);
}
