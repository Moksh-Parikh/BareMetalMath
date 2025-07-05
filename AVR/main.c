#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>

#include "headers/portpins.h"
#include "headers/USART.h"
#include "headers/pinDefines.h"

float exponent(float number, int power);
float calculatePercent(float numerator, float denominator);
float squareRoot(float square, int accuracy);

// TODO: add support for fractional indices
float exponent(float number, int power) {
    float returnVal = number;
    
    if (power == 0) {return 1.0;}
    else if (power < 0) {
        returnVal = 1 / exponent(number, power * -1);
    }

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

/*
    * x = Xin * cos(angle) - Yin * sin(angle)
    * y = Xin * sin(angle) + Yin * cos(angle)
    * if we set Xin to 1 and Yin to 0,
    * then the equations become:
    * x = cos(angle)
    * y = sin(angle)
    *
*/

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

    int rotationDirection;

    float tempX, tempY; 

    /* snprintf(buffer, 500, "%f\r\n", correctionFactor); */
    /* printString(buffer); */

    for (int i = 0; i < 16; i++) {
        rotationDirection = theta < alpha ? 1 : -1;
        theta += rotationDirection * thetaTable[i];

        tempX = x - rotationDirection * y * exponent2to1;
        tempY = y + rotationDirection * x * exponent2to1;

        x = tempX;
        y = tempY;

        /* snprintf(buffer, 500, "\e[38:5:23mSigma: %d, theta: %f\r\n\e[38:5:225mxOut: %f, yOut: %f\r\n\e[38:5:130mtan(): %f\r\n", rotationDirection, theta, x, y, y/x); */
        /* printString(buffer); */

        exponent2to1 /= 2;
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
 
    /* for (int i = 0; i <= 90; i++) { */
        char buffer[100];
        float cosine, sine;
        float tangent = CORDIC(1.570796, &cosine, &sine); // squareRoot(255, 6);
        snprintf(buffer, 100, "Estimated sine, cosine and tangent of %d: %f, %f, %f\r\n", 1.570796, sine, cosine, tangent);

        printString(buffer);
    /* } */

    return(0);
}
