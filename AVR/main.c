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

// Euclid's algorithm
/* uint32_t findGCD(uint32_t num1, uint32_t num2) { */
/*     while (num1 != num2) { */
/*         num1 /= num2; */
/*         num2 = num1 % num2; */

/*     } */
/*     return num1; */
/* } */

// Euclid's algorithm
// stolen from:
//      https://www.geeksforgeeks.org/dsa/euclidean-algorithms-basic-and-extended/#basic-euclidean-algorithm-for-gcd
uint32_t findGCD(uint32_t a, uint32_t b) {
    if (a == 0) { return b; }
    return findGCD(b % a, a);
}


float calculateFraction(float decimal, uint32_t* numerator, uint32_t* denominator) {
    char buffer[100];

    if (decimal == 0.0) { 
        *numerator = 0;
        *denominator = 0;

        return 0;
    }
    else if (decimal == 1.0) {
        *numerator = 1;
        *denominator = 1;

        return 1;
    }
    else if (decimal < 1.0) {
        float comparison = decimal;
        uint32_t digits;
        for (digits = 1; digits <= 6; digits++) {
            comparison *= 10;

            snprintf(buffer, 100, "fraction %f / %lu\r\n",
                comparison, (uint32_t)comparison
            );

            printString(buffer);
            if (comparison - (uint32_t)comparison <= 0) {
                break;
            }
        }
        *numerator = comparison;
        *denominator = exponent(10, digits - 1);
        
        uint32_t gcd = findGCD(*numerator, *denominator);

        *numerator /= gcd;
        *denominator /= gcd;

        return 1;
    }

    // if the last digit of the decimal is negative,
    // the only way to represent it will be
    // decimal / 10 ^ number of digits
    *denominator = 1000000;
    uint32_t comparison = decimal * (*denominator);

    while (comparison % 10 == 0) {
        comparison /= 10;
        *denominator /= 10;
    }
    
    if (comparison % 2 > 0) {
        while (comparison % 5 == 0) {
            comparison /= 5;
            *denominator /= 5;
        }

        *numerator = comparison;
        return 1;
    }
    
    snprintf(buffer, 100, "%lu\r\n",
        *denominator
    );

    printString(buffer);

    uint32_t decimalWholeNumber = (uint32_t)decimal;

    for (uint32_t i = decimalWholeNumber; i < 100000; i++) {
        /* account for rounding error */
        for (uint32_t j = 1; j < (i / decimal) + 1 ; j++) {
            if (j == i) { continue; }
            /* snprintf(buffer, 100, "fraction %lu / %lu\r\n", */
            /*     i, j */
            /* ); */

            /* printString(buffer); */
            if ((float)i / (float)j == decimal) {
                *numerator = i;
                *denominator = j;
                return 1;
            }
        }
    }

    return 1;
}


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

    uint32_t numerator, denominator;
    if (!calculateFraction(2.675, &numerator, &denominator)) {return 1;}

    snprintf(buffer, 500, "Estimated sine, cosine and tangent of %f: %f, %f, %f\r\n" 
             "20th root of 9: %f\r\n", 
             1.3, sine, cosine, tangent, radical(9, 20, 16)
             );

    printString(buffer);
    
    snprintf(buffer, 500, "4.6 as fraction %lu / %lu, %f\r\n",
             numerator, denominator, 23.0 / 5.0
    );

    printString(buffer);

    return(0);
}
