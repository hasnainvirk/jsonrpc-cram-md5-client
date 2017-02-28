/*
 * JsonOverNetstring.h
 *
 *  Created on: Jan 28, 2017
 *      Author: hasnain
 */

#ifndef JSONOVERNETSTRING_H_
#define JSONOVERNETSTRING_H_

#include <map>
#include <string>

using namespace std;

/**
 * Fault codes as per JSON RPC 2.0
 * */

typedef enum {
	PARSE_ERROR = -32700,
	INVALID_REQUEST = -32600,
	METHOD_NOT_FOUND = -32601,
	INVALID_PARAMETERS  =-32602,
	INTERNAL_ERROR = -32603,
	SERVER_ERROR = -32000,
	TOSIBOX_CUSTOM = 20
}fault_codes_e;


/** Buffer size for JSON RPC 2.0 Call*/
#define BUFFER_SIZE 500 //100 bytes

/**
 * A simple linked list of parameters
 * */
typedef struct parameter_list {
	/*@{*/
	char *param; /**< parameter string to be added */
	parameter_list *next; /**< pointer to the next item in the list */
	/*@}*/
} parameter_list_s;

/**
 * A message structure for client to fill in
 * for generating JSON RPC 2.0 objects with Netstrings format
 * */
typedef struct {
	/*@{*/
	double version; /**< JSON RPC ver #, e.g., 2.0 tosibox supports JSON RPC 2.0 */
	const char *method_type; /**<  defined method type, e.g., auth_request */
	parameter_list_s *params;  /**< A linked list of parameters */
	int id; /**< RPC Call ID */
	/*@}*/
} clt_msg_s;

/**
 * JSON RPC Error Object
 * */
typedef struct {
	/*@{*/
	signed short code /**< Error codes, defined in fault_codes_e*/;
	char *message; /**< Error message */
	char *data; /**< Optional. Can be omitted */
	/*@}*/
} err_obj_s;

/**
 * A union for handling either result or error response
 * */
typedef union {
	/*@{*/
	char *result; /**< value returned from server in case of result */
	err_obj_s *error; /**< errors will be handled in error object */
	/*@}*/
} value_handle;

/**
 * A data structure to handle Server responses.
 * */
typedef struct {
	/*@{*/
	int id; /* Transaction ID*/
	char *method_type; /**< could be result or error */
	value_handle handle; /**< value parsed, check union value_handle */
} serv_resp_msg_s;

/**
 * Server response order number.
 * This order is strictly local. Have nothing to do with standard
 * specs. Helps in aligning map of key value pairs utilized for parsing server
 * reponses.
 */
typedef enum {
	RESULT=1,//!< RESULT
	ID,      //!< ID
	ERROR,   //!< ERROR
	CODE     //!< CODE
} serv_json_order;

//Nothing special, just a bit of convenience
typedef map<int, string> ServerResponse;

/**
 * \class JsonOverNetstring
 * Provides a minimal framework to create/handle
 * JSON RPC 2.0 objects over Netstrings.
 * */
class JsonOverNetstring
{
public:
	/** constructor */
	JsonOverNetstring();

	/** Destructor  */
	~JsonOverNetstring();

	/**
	 * @brief Prepares an Authentication Request message.
	 *
	 * Builds this request message based upon JSON RPC 2.0
	 * and writes it in Netstring Transport format.
	 *
	 * @param msg instructions that are needed to build the message.
	 * @return a pointer to the  buffer where the message is stored
	 *  */
	const char *auth_request(clt_msg_s msg);

	/**
	 * @brief  Prepares an authentication response message.
	 *
	 * The message is based upon JSON RPC 2.0 and writes it in
	 * Netstring Transport format.
	 *
	 * Prior to writing the message in JSON format, this method
	 * performs HMAC algorithm over the challenge and the secret key
	 * provided in the arguments. After encryption the resultant response
	 * is Base64 encoded and is added to the message (JSON format)
	 *
	 * @param msg instructions that are needed to build the message.
	 * @param challenge a pointer to the challenge string received
	 * 		  from the server.
	 *
	 * @return pointer to buffer where the message is written
	 *
	 *  */
	const char *EncryptedAuthMsg(clt_msg_s msg, const char *challenge);

	/**
	 * @brief Initializes a list of parameters
	 *
	 * @param list pointer to list of parameters
	 * */
	void param_list_init(parameter_list_s *list);

	/**
	 * @brief Add an item to an existing list.
	 *
	 * Push to the list. New arrival become the new Head.
	 *
	 * @param list_head pointer to the head of the list
	 * @param data pointer to the data
	 * @param len length of data
	 * */
	void add_to_param_list(parameter_list_s **list_head, const char *data, size_t len);

	/**
	 * @brief Remove an item from the list.
	 *
	 * Iterates over the list and tries to match the data with an existing entry.
	 * Deletes the entry if matched and re-links the stuff again.
	 * Not the world's most efficient implementation. We don't need one right now
	 * probably.
	 *
	 * @param list_head pointer to the head of the list
	 * @param data pointer to the data
	 * @param len length of data
	 * */
	void remove_from_param_list(parameter_list_s **list_head, const char *data, size_t len);

	/**
	 * @brief Drop the whole list.
	 *
	 * Iterates over the list and deletes every entry.
	 * Self destructs.
	 *
	 *  @param list_head pointer to the head of the list*/
	void delete_param_list(parameter_list_s **list_head);

	/**
	 * @brief Parses the server JSON response.
	 *
	 * Prepares a ServerReponse data structure.
	 *
	 * @param data pointer to the received data
	 * @return a pointer to the ServerResponse.
	 */
	ServerResponse *JsonResponsParser(char *data);

private:

	/**
	 * Internal buffer space for the framework.
	 */
	char *_internal_buffer;

	/**
	 * key-value pair of order number and its string value
	 * see for order no. reference serv_json_order
	 */
	ServerResponse _nodes;

	/**
	 * Disallow copy constructor
	 */
	JsonOverNetstring(const JsonOverNetstring&);

	/**
	 * Disallow operator overload
	 */
	JsonOverNetstring& operator=(const JsonOverNetstring&);

};



#endif /* JSONOVERNETSTRING_H_ */
