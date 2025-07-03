#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>

#include "headers/portpins.h"
#include "headers/USART.h"
#include "headers/pinDefines.h"

// TODO: add support for fractional indices
float exponent(float number, int power) {
    float returnVal = number;
    for (int i = 0; i < power - 1; i++) {
        returnVal *= number;
    }

    return returnVal;
}

float calculatePercent(float numerator, float denominator) {
    return (100 * numerator) / denominator;
}


// uses Heron's Method to calculate the square root
// https://gregorygundersen.com/blog/2023/02/01/estimating-square-roots/
float squareRoot(float square, int accuracy) {
    float guessArray[accuracy];

    for (int j = 1; j < square; j++) {
        float iteratorSquared = exponent(j, 2);
        
        if (iteratorSquared == square) { return (float)j; }
        
        if ( calculatePercent(iteratorSquared, square) > 97.0) {// || iteratorSquared - square > -3 ) {
            guessArray[0] = (float)j;
            break;
        }
    }

    for(int i = 1; i < accuracy; i++) {
        float correctionFactor = square / guessArray[i - 1];

        guessArray[i] = (guessArray[i - 1] + correctionFactor) / 2;
    }

    return guessArray[accuracy - 1];
}


int main(void) {
    initUSART();
    printString("Initialised!\r\n");
    
    char buffer[100];
    float result = squareRoot(255, 6);
    snprintf(buffer, 100, "Estimated square root of %d: %f\r\n", 255, result);

    printString(buffer);

    return(0);
}
