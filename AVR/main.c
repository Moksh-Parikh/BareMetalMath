#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>

#include "headers/portpins.h"
#include "headers/USART.h"
#include "headers/pinDefines.h"

typedef struct {
    uint8_t decimalPointLocation;
    uint32_t number;
} myFloat;


uint32_t findGCD(uint32_t a, uint32_t b);
float calculateFraction(float decimal, uint32_t* numerator, uint32_t* denominator);
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
    *integer = input.number >> input.decimalPointLocation;
    
    // apply a bitmask to only read the fractional part
    *decimal = input.number & (0xffffffff >> (32 - input.decimalPointLocation));
}


void printMyFloat(myFloat in) {
    uint32_t integer, decimal;
    separateFloatComponents(in, &integer, &decimal);

    printf("%ld.%ld", integer, decimal);
}


myFloat addition(myFloat number1, myFloat number2) {
    printf("number1 dp: %d, number2 dp: %d\n", number1.decimalPointLocation, number2.decimalPointLocation);
    printMyFloat(number1);
    printf(", ");
    printMyFloat(number2);
    printf("\n");

    if (number1.decimalPointLocation >= number2.decimalPointLocation) {
        number2.number <<= number1.decimalPointLocation;
        number2.decimalPointLocation = number1.decimalPointLocation;
    }
    else {
        number1.number <<= (number2.decimalPointLocation - number1.decimalPointLocation);
        number1.decimalPointLocation = number2.decimalPointLocation;
    }

    printf("After shift:\n\tnumber1 dp: %d, number2 dp: %d\n", number1.decimalPointLocation, number2.decimalPointLocation);
    printMyFloat(number1);
    printf(", ");
    printMyFloat(number2);
    printf("\n");

    myFloat output;
    uint32_t integer1, fraction1, integer2, fraction2;

    separateFloatComponents(number1, &integer1, &fraction1);
    separateFloatComponents(number2, &integer2, &fraction2);

    fraction1 += fraction2;
    integer1 += integer2;

    output.decimalPointLocation = 32 - __builtin_clzl(fraction1);
    output.number = fraction1 | (integer1 << output.decimalPointLocation);

    printf("Fraction: %ld\nInteger: %ld\nWhole: %ld\nDP: %d\n", fraction1, integer1 << output.decimalPointLocation, output.number, output.decimalPointLocation);

    printf("Output:\n");
    printMyFloat(output);
    printf("\n");

    return output;
}


myFloat buildFloat(uint8_t decimalPlace, uint32_t inputNumber) {
    myFloat output;
    uint32_t fractional = inputNumber % (uint32_t)exponent(10, decimalPlace);

    output.decimalPointLocation = 32 - __builtin_clzl(fractional);

    output.number = (uint32_t)(inputNumber / exponent(10, decimalPlace)) << output.decimalPointLocation;
    output.number |= fractional;

    return output;
}


myFloat multiply(myFloat number1, myFloat number2) {
    uint32_t integer1, decimal1, integer2, decimal2;
    uint32_t temp1, temp2;

    myFloat output;

    printMyFloat(number1);
    printf("\n");
    printMyFloat(number2);
    printf("\n");

    separateFloatComponents(number1, &integer1, &decimal1);
    separateFloatComponents(number2, &integer2, &decimal2);

    temp1 = integer1 * integer2;
    printf("%ld\n", temp1);

    temp2 = decimal1 * decimal2;
    printf("%ld\n", temp2);

    temp2 += decimal1 * integer2;
    printf("%ld\n", temp2);

    temp2 += integer1 * decimal2;
    printf("%ld\n", temp2);

    output.decimalPointLocation = number1.decimalPointLocation < number2.decimalPointLocation
                                  ? number2.decimalPointLocation :
                                  number1.decimalPointLocation;

    temp1 <<= output.decimalPointLocation;
    temp1 |= temp2;
    output.number = temp1;
    
    printf("l82\n");
    printMyFloat(output);
    printf("\n");

    return output;
}


myFloat divide(uint32_t dividend, uint32_t divisor) {
    myFloat quotient = {0, 0};
    uint32_t temp = dividend % divisor;
    uint32_t temp2 = dividend % divisor;

    if (dividend == divisor) {
        quotient.decimalPointLocation = 0;
        quotient.number = 1;
        
        printf("%ld\n%ld / %ld = %ld\n", quotient.number, dividend, divisor, dividend / divisor);
        return quotient;
    }

    /* if (temp == 0) { */
    /* } */
    
    /* temp = dividend % divisor; */
    
    uint32_t remainder = 0;
   
    int i = 1;
    char buffer[100];

    while (remainder) {
        temp *= 10;
        remainder = temp % dividend;

        i++;
    }

    // TODO: find alternative to the GCC builtin
    quotient.decimalPointLocation = 32 - __builtin_clzl(temp2);

    quotient.number = temp | ((dividend / divisor) << quotient.decimalPointLocation);

    printf("%ld\n%ld / %ld = %ld, dcp = %d\n", quotient.number, dividend, divisor, dividend / divisor, quotient.decimalPointLocation);
    printMyFloat(quotient);
    printf("\n");

    return quotient;
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

    addition(buildFloat(1, 327), buildFloat(2, 32));

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
