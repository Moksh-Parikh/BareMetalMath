#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>

#include "headers/portpins.h"
#include "headers/USART.h"
#include "headers/pinDefines.h"

float exponent(float number, int power);
float calculatePercent(float numerator, float denominator);
float radical(float radicand, int index, int accuracy);
float CORDIC(float alpha, float* sin, float* cos);


// TODO: add support for fractional indices
float exponent(float number, int power) {
    float returnVal = number;
    
    if (power == 0) {return 1.0;}
    else if (power < 0) {
        returnVal = 1 / exponent(number, power * -1);
    }
    /* else if (fabsf(roundf(power) ) <= 0.0001f) { */
    /*     returnVal = */ 
    /* } */

    for (int i = 0; i < power - 1; i++) {
        returnVal *= number;
    }

    return returnVal;
}


float calculatePercent(float numerator, float denominator) {
    return (100 * numerator) / denominator;
}

// Uses Newton's method
//      https://gmplib.org/manual/Nth-Root-Algorithm
float radical(float radicand, int index, int accuracy) {
    float guessArray[accuracy];

    for (int j = 1; j < radicand; j++) {
        float iteratorSquared = exponent(j, index);
        
        if (iteratorSquared == radicand) { return (float)j; }
        
        if ( calculatePercent(iteratorSquared, radicand) > 97.0) {// || iteratorSquared - square > -3 ) {
            guessArray[0] = (float)j;
            break;
        }
    }

    for(int i = 1; i < accuracy; i++) {
        guessArray[i] = 
            (radicand / exponent(guessArray[i - 1], (index - 1))
            + (guessArray[i - 1] * (index - 1)) ) / index;
    }

    return guessArray[accuracy - 1];
}


// translated from Python implementation at:
//      https://en.wikipedia.org/wiki/CORDIC
float CORDIC(float alpha, float* sin, float* cos) {
    float thetaTable[16] = {0.785398, 0.463647, 0.244978, 0.124354, 0.062418, 0.031239, 0.015623, 0.007812, 0.003906, 0.001953, 0.000976, 0.000488, 0.000244, 0.000122, 0.000061, 0.000030};

    /* char buffer[500]; */

    float correctionFactor = 0.607259;
    float theta = 0.0;
    float x = 1.0;
    float y = 0.0;
    float exponent2to1 = 1;
    
    long exp;
    long temp2;
    long bitMask = 0x7f800000; // isolates the 1st to 9th bits

    int rotationDirection;

    float tempX, tempY; 

    /* snprintf(buffer, 500, "%f\r\n", correctionFactor); */
    /* printString(buffer); */

    for (int i = 0; i < 16; i++) {
        exp = * (long *) &exponent2to1;
        temp2 = * (long *) &exponent2to1;
        
        exp &= bitMask;
        exp -= 1;
        
        temp2 &= 0x807fffff; // clear the 1st to 9th bits
        temp2 |= exp; // OR on the divided exponent
        exponent2to1 = * (float *) &temp2;

        rotationDirection = theta < alpha ? 1 : -1;
        theta += rotationDirection * thetaTable[i];

        tempX = x - rotationDirection * y * exponent2to1;
        tempY = y + rotationDirection * x * exponent2to1;

        x = tempX;
        y = tempY;

        /* snprintf(buffer, 500, "\e[38:5:23mExponent: %f\r\n", exponent2to1); */
        /* printString(buffer); */
    }
    
    x *= correctionFactor;
    y *= correctionFactor;

    /* snprintf(buffer, 500, "%f / %f = %f\r\n", x, y, y/x); */
    /* printString(buffer); */
    
    *cos = x;
    *sin = y;

    return y / x;
}

int main(void) {
    initUSART();
    printString("Initialised!\r\n");
 
    char buffer[500];
    float cosine, sine;
    float tangent = CORDIC(1.3, &cosine, &sine);

    snprintf(buffer, 500, "Estimated sine, cosine and tangent of %f: %f, %f, %f\r\n20th root of 9: %f\r\n", 1.3, sine, cosine, tangent, radical(9, 20, 16));

    printString(buffer);

    return(0);
}
