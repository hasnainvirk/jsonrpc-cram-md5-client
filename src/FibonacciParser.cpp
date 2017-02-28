/*
 * FibonacciParser.cpp
 *
 *  Created on: Jan 26, 2017
 *      Author: hasnain
 */

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <math.h>

#include "FibonacciParser.h"

using namespace std;

/* Checks if the num is Fibonacci or not */
bool FibonacciParser::_isFibonacci(int num) {
	/*https://en.wikipedia.org/wiki/Fibonacci_number#Recognizing_Fibonacci_numbers*/
	/* Proof of concept given above */

	int i = sqrt((5*num*num)+4);
	int j = sqrt((5*num*num)-4);

	return (i*i == ((5*num*num)+4)) || (j*j == ((5*num*num)-4));
}

/* Processes the raw contents and provides us with the hidden secret */
char *FibonacciParser::ProcessContent()
{
	size_t size = strlen(_raw_content);
	char *bufptr = _processed_content;

	/* Iterate over raw contents and copy every character
	 * which resides at Fibonacci indexes. */
	for (unsigned int i = 0; i < size; i++) {
		if (_isFibonacci(i)) {
			if (i == 1) {
				memcpy(bufptr, _raw_content + i, 1);
				bufptr++;
			}
			memcpy(bufptr, _raw_content + i, 1);
			bufptr++;
		}
	}
	printf("Processed: %s\n", _processed_content);

	return _processed_content;
}

/* Construtor reads the contents of the file into memory */
FibonacciParser::FibonacciParser(const char *File_Name)
{
	streampos size; //stream position variable

	_raw_content = NULL;
	_processed_content = NULL;

	/* File stream (opens a file, attaches stream to it),
	 * current position at the end of the file */
	ifstream file(File_Name, ios::binary|ios::in|ios::ate);

	if (file.is_open()) {
		size = file.tellg(); //current position, which is end of the file
		_raw_content = new char [size]; //allocate memory for the raw contents
		_processed_content = new char [size]; //allocate memory for the processed contents
		file.seekg(0,ios::beg); //go back to the beginning of the file
		file.read(_raw_content, size); //read the contents into memory
		file.close(); //close the file
		printf("Raw: %s\n", _raw_content);

	} else {
		printf("File Open Failed.\n");
	}
}

FibonacciParser::~FibonacciParser()
{
	delete [] _raw_content;
	delete [] _processed_content;
}


