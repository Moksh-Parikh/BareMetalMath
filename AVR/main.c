#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>

#include "headers/portpins.h"
#include "headers/USART.h"
#include "headers/pinDefines.h"

/* ISR(TIMER1_COMPA_vect) { */
/*     PORTB ^= (1 << PB5); */
/*     printString("toggled!\r\n"); */
/* } */


/* static inline void initTimer1() { */
/*     TCCR1A = 0; */
/*     TCCR1B = 0; */
/*     // 16 000 000 (16 million) / 1024 (clock prescaler set below) */
/*     // = 15624 ticks per second */
/*     OCR1A = 15624; */

/*     // put the clock in ctc mode and set the prescaler to 1024 */
/*     TCCR1B |= (1 << CS10) | (1 << CS12) | (1 << WGM12); */

/*     TIMSK1 |= (1 << OCIE1A); */

/*     sei(); */
/* } */

float exponent(float number, int power) {
    float returnVal = number;
    for (int i = 0; i < power - 1; i++) {
        returnVal *= number;
    }

    return returnVal;
}

float squareRoot(float square) {
    float guessArray[6];

    printString("in squareRoot\r\n");

    for (int j = 1; j < square; j++) {
        float iteratorSquared = exponent(j, 2);

        printString("j and j^2:\r\n");
        printByte(j);
        transmitByte('\r');
        transmitByte('\n');
        
        printByte(iteratorSquared);
        transmitByte('\r');
        transmitByte('\n');
        
        if (iteratorSquared == square) { return (float)j; }
        
        if ( iteratorSquared - square < 3 || iteratorSquared - square > -3 ) {
            guessArray[0] = (float)j;
            break;
        }
    }


    printByte(guessArray[0]);
    printString("\r\n");

    for(int i = 1; i < 6; i++) {
        float correctionFactor = square / guessArray[i - 1];

        guessArray[i] = (guessArray[i - 1] + correctionFactor) / 2;
        /* guessArray[i] = (1 / 2) * (guessArray[i - 1] + (square / guessArray[i - 1]) ); */

        printString("guess\r\n");
        for (int k = 0; k < 4; k++) {
            printBinaryByte( *( &guessArray[i] + k) );
        }

        printString("\r\n");
    }

    return guessArray[5];
}


int main(void) {
    DDRB |= (1 << PB5);
    initUSART();
    /* initTimer1(); */
    printString("Initialised!\r\n");
    
    char buffer[100];
    float result = squareRoot(36);

    snprintf(buffer, 100, "Hi: %05f\r\n", result);

    printString("\r\nSquare Root:\r\n");
    printString(buffer);
    printString("\r\n");
    /* while (1) {} */

    return(0);
}
