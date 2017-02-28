/*
 * FibonacciParser.h
 *
 *  Created on: Jan 26, 2017
 *      Author: hasnain
 */

#ifndef FIBONACCIPARSER_H_
#define FIBONACCIPARSER_H_

class FibonacciParser {
public:
	/**
	 * @brief Constructor.
	 *
	 * Read contents of of a file
	 * Stores in _raw_contents
	 *
	 * @param File_Name name of the file */
	FibonacciParser(const char *File_Name);

	/**
	 * @brief  Destructor.
	 *
	 * Destroys class object, _raw_contents, processed contents */
	~FibonacciParser();

	/**
	 * @\brief Retrieves buried secret
	 *
	 * Processes the raw contents and returns pointer to the processed
	 * buffer. This buffer pointer points to _processed_content.
	 * User can access member variable directly too.
	 *
	 * @return a pointer to the processed data, i.e., the buried secret*/
	char *ProcessContent();

	/**
	 * Processed byte array containing hidden processed content
	 * */
	char *_processed_content;

private:

	/**
	 * Unprocessed byte array, read from file.
	 * */
	char *_raw_content;

	/**
	 * @brief Checks if the number belongs to Fibonacci progression or not.
	 * @param num, number to be checked
	 *
	 * @return true/false
	 */
	bool _isFibonacci(int num);

	/**
	 * Disallow copy constructor
	 */
	FibonacciParser(const FibonacciParser&);

	/**
	 * Disallow operator overload
	 */
	FibonacciParser& operator=(const FibonacciParser&);
};



#endif /* FIBONACCIPARSER_H_ */
