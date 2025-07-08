#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>

#include "headers/portpins.h"
#include "headers/USART.h"
#include "headers/pinDefines.h"

#define SCALEDINT(scale, number) ( (myFloat){scale, number} )

typedef struct {
    uint16_t scaleAsExponent;
    uint32_t number;
} myFloat;


void separateFloatComponents(myFloat input, uint32_t* integer, uint32_t* decimal);
void printMyFloat(myFloat in);
uint32_t findMaxPlaceValue(uint32_t number, uint8_t* placeNumber);
uint32_t getNumberToPlaceValue(uint32_t number, uint32_t maxPlaceValue);
int getDigitAtPlaceValue(uint32_t value, int digitNumber);
myFloat addition(myFloat number1, myFloat number2);
myFloat subtraction(myFloat number1, myFloat number2);
myFloat multiply(myFloat number1, myFloat number2);
myFloat divide(myFloat number1, myFloat number2);
uint32_t findGCD(uint32_t a, uint32_t b);
float calculateFraction(float decimal, uint32_t* numerator, uint32_t* denominator);
uint32_t exponentAsInt(uint32_t number, int power);
uint32_t findMaxPlaceValue(uint32_t number, uint8_t* placeNumber);
float exponent(float number, int power);
float calculatePercent(float numerator, float denominator);
float radical(float radicand, int index, int accuracy);
float CORDIC(float alpha, float* sin, float* cos);


int uart_put_char(char c, FILE *stream) { 
	if (c == '\n') uart_put_char('\r', stream);
   	loop_until_bit_is_set(UCSR0A, UDRE0); // wait for UDR to be clear 
   	UDR0 = c;
    return 0; 
}


void separateFloatComponents(myFloat input, uint32_t* integer, uint32_t* decimal) {
    *integer = input.number / exponentAsInt(10, input.scaleAsExponent);
    
    *decimal = input.number % exponentAsInt(10, input.scaleAsExponent);
}


void printMyFloat(myFloat in) {
    uint32_t integer, decimal;
    uint8_t placeValue;
    separateFloatComponents(in, &integer, &decimal);
    findMaxPlaceValue(decimal, &placeValue);

    printf("%ld.", integer);
    
    // start at -1 so if the scale is zero, no zeroes are printed
    for (int i = -1; i < in.scaleAsExponent - placeValue; i++) {
        printf("0");
    }
    printf("%ld", decimal);
}


uint32_t findMaxPlaceValue(uint32_t number, uint8_t* placeNumber) {
    uint32_t maxPlaceValue = 0, exponentSum = 1;
    int k = 1;

    while (k < 10) {
        exponentSum += (uint32_t)exponent(10, k);
        maxPlaceValue += 9 * exponentSum;

        if (number % (uint32_t)exponent(10, k) < 10) { break; }

        k++;
    }
    *placeNumber = k + 1;

    return maxPlaceValue;
}


uint32_t getNumberToPlaceValue(uint32_t number, uint32_t maxPlaceValue) {
    int k = 1;
    while (number < maxPlaceValue) {
        number *= 10;
        k++;
    }
    return number / 10;
}


int getDigitAtPlaceValue(uint32_t value, int digitNumber) {
    return ( value / (uint32_t)exponent( 10, digitNumber - 1 ) ) % 10;
}


myFloat addition(myFloat number1, myFloat number2) {
    myFloat sum;

    if (number1.scaleAsExponent > number2.scaleAsExponent) {
        number2.number *= exponentAsInt(10, (number1.scaleAsExponent - number2.scaleAsExponent));
    }
    else if (number1.scaleAsExponent < number2.scaleAsExponent) {
        number1.number *= exponentAsInt(10, (number2.scaleAsExponent - number1.scaleAsExponent) );
        number1.scaleAsExponent = number2.scaleAsExponent;
    }

    sum.number = number1.number + number2.number;
    sum.scaleAsExponent = number1.scaleAsExponent;

    return sum;
}


myFloat subtraction(myFloat number1, myFloat number2) {
    myFloat sum;

    if (number1.scaleAsExponent > number2.scaleAsExponent) {
        number2.number *= exponentAsInt(10, (number1.scaleAsExponent - number2.scaleAsExponent));
    }
    else if (number1.scaleAsExponent < number2.scaleAsExponent) {
        number1.number *= exponentAsInt(10, (number2.scaleAsExponent - number1.scaleAsExponent) );
        number1.scaleAsExponent = number2.scaleAsExponent;
    }

    sum.number = number1.number - number2.number;
    sum.scaleAsExponent = number1.scaleAsExponent;

    return sum;
}


myFloat multiply(myFloat number1, myFloat number2) {
    myFloat sum;

    if (number1.scaleAsExponent > number2.scaleAsExponent) {
        /* number2.number *= exponentAsInt(10, (number1.scaleAsExponent - number2.scaleAsExponent)); */
    }
    else if (number1.scaleAsExponent < number2.scaleAsExponent) {
        /* number1.number *= exponentAsInt(10, (number2.scaleAsExponent - number1.scaleAsExponent) ); */
        number1.scaleAsExponent = number2.scaleAsExponent;
    }

    sum.number = number1.number * number2.number;
    printf("HII: %ld, %d, %d\n", sum.number, number1.scaleAsExponent, number2.scaleAsExponent);
    sum.scaleAsExponent = number1.scaleAsExponent;

    return sum;
}


myFloat divide(myFloat number1, myFloat number2) {
    myFloat sum;

    if (number1.scaleAsExponent > number2.scaleAsExponent) {
        number2.number *= exponentAsInt(10, (number1.scaleAsExponent - number2.scaleAsExponent));
    }
    else if (number1.scaleAsExponent < number2.scaleAsExponent) {
        number1.number *= exponentAsInt(10, (number2.scaleAsExponent - number1.scaleAsExponent) );
        /* number1.scaleAsExponent = number2.scaleAsExponent; */
    }

    sum.number = number1.number / number2.number;

    sum.scaleAsExponent = number1.scaleAsExponent;

    return sum;
}


// Euclid's algorithm
// stolen from:
//      https://www.geeksforgeeks.org/dsa/euclidean-algorithms-basic-and-extended/#basic-euclidean-algorithm-for-gcd
uint32_t findGCD(uint32_t a, uint32_t b) {
    if (a == 0) { return b; }
    return findGCD(b % a, a);
}


float calculateFraction(float decimal, uint32_t* numerator, uint32_t* denominator) {
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
    else if (decimal - (int)decimal == 0) {
        *numerator = (int)decimal;
        *denominator = 1;
    }

    // floating points only have 7 digits of precision max,
    // so we multiply by 10 ^ 6 to get an integer
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

    uint32_t gcd = findGCD(comparison, *denominator);

    *numerator = comparison / gcd;
    *denominator /= gcd;

    return 1;
}


uint32_t exponentAsInt(uint32_t number, int power) {
    uint32_t returnVal = number;
    
    if (power == 0) {return 1.0;}
    /* else if (power < 0) { */
    /*     returnVal = 1 / exponent(number, power * -1); */
    /* } */

    for (int i = 0; i < power - 1; i++) {
        returnVal *= number;
    }

    return returnVal;
}


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


// Uses Newton's method
//      https://gmplib.org/manual/Nth-Root-Algorithm
float radical(float radicand, int index, int accuracy) {
    float guessArray[accuracy];

    for (int j = 1; j < radicand; j++) {
        float iteratorSquared = exponent(j, index);
        
        if (iteratorSquared == radicand) { return (float)j; }
        
        if ( calculatePercent(iteratorSquared, radicand) > 97.0) {
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

    float correctionFactor = 0.607259;
    float theta = 0.0;
    float x = 1.0;
    float y = 0.0;
    float exponent2to1 = 1;
    
    long exp;
    long temp2;
    // isolates the 1st to 9th bits, the exponent
    long bitMask = 0x7f800000;

    int rotationDirection;

    float tempX, tempY; 

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
    }
    
    x *= correctionFactor;
    y *= correctionFactor;
    
    *cos = x;
    *sin = y;

    return y / x;
}

int main(void) {
    initUSART();

    FILE mystdout = FDEV_SETUP_STREAM(uart_put_char, NULL, _FDEV_SETUP_WRITE);
    stdout = &mystdout;

    printf("Initialised!\n");
 
    float cosine, sine;
    float tangent = CORDIC(1.3, &cosine, &sine);

    float test[7] = {2.0, 2.6, 2.67, 2.674, 2.6747, 2.67476, 0.267476};

    /* addition(buildFloat(1, 327), buildFloat(2, 32)); */
    /* printMyFloat( subtraction( (myFloat){0, 67}, addition( (myFloat){1, 327}, (myFloat){0, 32} ) )); */
    /* printMyFloat( multiply( (myFloat){1, 5}, (myFloat){0, 2} ) ); */
    /* printf("\n"); */
    /* printMyFloat( divide( (myFloat){0, 5}, (myFloat){1, 2} ) ); */
    /* printf("\n"); */
    printMyFloat(SCALEDINT(3, 25));
    printf("\n");
    printMyFloat(SCALEDINT(0, 25));
    printf("\n");

    printf("Estimated sine, cosine and tangent of %f: %f, %f, %f\r\n" 
             "20th root of 9: %f\r\n"
             "GCD of 5 and 2: %ld\r\n",
             1.3, sine, cosine, tangent, radical(9, 20, 16), findGCD(5, 2)
             );

    for (int i = 0; i < 7; i++) {
        uint32_t numerator, denominator;
        
        if (!calculateFraction(test[i],
                               &numerator,
                               &denominator
                               )) {
            return 1;
        }

        
        printf("%f as fraction %lu / %lu\r\n", test[i], numerator, denominator);
    }

    return(0);
}
