/*
 * base64Helper.h
 *
 *  Created on: Jan 31, 2017
 *      Author: hasnain
 */

#include <string>
#ifndef LIB_BASE64HELPER_H_
#define LIB_BASE64HELPER_H_

/**
 * @brief Internal in-line function to if a character is base64
 * @param c character to be validated
 * @return true/false
 */
static inline bool is_base64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

/**
 * @brief Base64 Encodes a given byte array
 *
 * @param bytes_to_encode bytes to encode
 * @param in_len length of the bytes provided
 * @return a base64 encoded std::string
 */
std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len);

/**
 * @brief Decodes a base64 encoded string
 *
 * @param encoded_string a base64 encoded std::string
 * @return a decoded std::string
 */
std::string base64_decode(std::string const& encoded_string);

#endif /* LIB_BASE64HELPER_H_ */
