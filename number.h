#pragma once

#ifndef NUMBERS_H_
#define NUMBERS_H_

#include <stdlib.h>

#include "object.h"


enum number_type {
    NUMBER_INTEGER,
    NUMBER_RATIONAL,
    NUMBER_REAL,
    NUMBER_COMPLEX
};


union number_value {
    int integer;
    
    struct {
	int numerator;
	unsigned int denominator;
    } rational;
    
    double real;
    
    struct {
	double real;
	double imaginary;
    } complex;
};


struct number {
    struct object head;
    enum number_type type;
    union number_value value;
};


extern struct object_type TYPE_NUMBER;



#endif
