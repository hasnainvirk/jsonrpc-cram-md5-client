/*
 * JsonOverNetstring.cpp
 *
 *  Created on: Jan 28, 2017
 *      Author: hasnain
 */

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <iterator>
#include <iostream>
#include <fstream>
#include "JsonOverNetstring.h"
#include "base64Helper.h"
#include "md5.h"

#ifdef TEST
#define TESTING_ENABLED 1
#endif

using namespace std;

JsonOverNetstring::JsonOverNetstring()
{
	_internal_buffer = new char [BUFFER_SIZE];
}

JsonOverNetstring::~JsonOverNetstring()
{
	delete [] _internal_buffer;
}

/* Example Format:
 *  Netstring-length:{"jsonrpc": "2.0", "method": "auth_request", "id": 1}*/
const char *JsonOverNetstring::auth_request(clt_msg_s msg)
{
	char msg_buf[BUFFER_SIZE];
	char appended[BUFFER_SIZE];

	sprintf(msg_buf, "{\"jsonrpc\": \"%.1f\", \"method\": \"%s\", \"id\": %d}",
			msg.version, msg.method_type, msg.id);

	sprintf(appended, "%zu:%s,", strlen(msg_buf), msg_buf);
	printf("Plain Request: %s\n", appended);
	memcpy(_internal_buffer, appended, strlen(appended));

	return _internal_buffer;
}

const char *JsonOverNetstring::EncryptedAuthMsg(clt_msg_s msg, const char *challenge) {

	/* Lame way of accessing list parameters from a lame list.
	 * We kind of know what was added to the list and in what order.
	 * Should have come up with a better linked list.
	 * xxx May be an ID driven list or something.   */
	const char *pwd = msg.params->param;
	const char *uname = msg.params->next->param;

	/* Length of the password/key/secret*/
	int key_length = strlen(pwd);

	/*Inner and outer padding for key (secret/password), needed
	 * for HMAC algorithm*/
	unsigned char k_ipad[65];
	unsigned char k_opad[65];

#if TESTING_ENABLED
	pwd = "tanstaaftanstaaf";
	key_length = strlen(pwd);
	uname = "tim";
	challenge = "PDE4OTYuNjk3MTcwOTUyQHBvc3RvZmZpY2UucmVzdG9uLm1jaS5uZXQ+";
	char *test_digest = "b913a602c7eda7a495b4e6e7334d3890";
	printf("Password length %d\n", key_length);
#endif

	/* For HMAC algorithm, we first need to decode the Base64 encoded Challenge
	 * received from the server */
	std::string encoded_str = static_cast<std::string>(challenge);
	std::string decoded_str = base64_decode(encoded_str);
	printf("Base64-DECODED: %s\n", decoded_str.c_str());

	/* test */
#if TESTING_ENABLED
	if (strcmp(decoded_str.c_str(),
			"<1896.697170952@postoffice.reston.mci.net>") == 0) {
		printf("Testing: Base64 Decoding works.\n");
	} else {
		fprintf(stderr, "Testing: Shame, Base64 Decoding doesn't work.\n");
	}
#endif


	/* Initialize cryptographic engine.
	 * We need 3 contexts as HMAC algorithm, converges in two or three distinct steps,
	 * namely:
	 * 1) A Hash of key itself, if its length is larger than 64 bytes
	 * 2) A Hash of challenge and (key xored ipad)
	 * 3) A Hash of the digest from 2) and (key zored opad)
	 * We actually know that our key is 10 bytes long, so we will need contexts however,
	 * just for better flexible design, let's implement the algorithm correctly and
	 * have 3 contexts.*/
	MD5 md5_engine_ctx1;
	MD5 md5_engine_ctx2;
	MD5 md5_engine_ctx3;

	/* For CRAM-Md5 Protocol, Step 1: Base64 decode the challenge message
	 * Following RFC's were consulted. Although the Md5 implementation is not
	 * our original work. It's a copy of Frank Thilo's work (which is essentially
	 * a C++ port to C reference code provided in the RFC.)*/
	/*https://tools.ietf.org/html/rfc2095, CRAM */
	/*https://tools.ietf.org/html/rfc2104, HMAC*/
	/*https://www.ietf.org/rfc/rfc1321.txt, MD5*/

	/* Following code block will append zeros to the password if its length is less
	 * than block size of hash engine (which is 64 Bytes).
	 * As of step 1 defined in rfc2104, i the secret is larger than block size, we
	 * hash the secret which reset the length to 16.
	 * However, if the length is smaller than or equal to block length, we continue
	 * as normal
	 *
	 *    the HMAC_MD5 transform looks like:
	 *
	 * MD5(K XOR opad, MD5(K XOR ipad, text))
	 *
	 * where K is an n byte key
	 * ipad is the byte 0x36 repeated 64 times
	 * opad is the byte 0x5c repeated 64 times and
	 * text is the data being protected */
	int BLOCK_SIZE = 64;
	unsigned char *K = new unsigned char [BLOCK_SIZE];
	memset(K, 0, BLOCK_SIZE);
	if (key_length > 64) {
		md5_engine_ctx1.update(pwd, key_length);
		md5_engine_ctx1.finalize();
		std::string hash = md5_engine_ctx1.hexdigest();
		memcpy(K, (unsigned char *)hash.c_str(), 16);
		key_length = 16;
	} else {
		memcpy(K, (unsigned char *)pwd, key_length);
		key_length = BLOCK_SIZE;
	}

	/* We start out by filling in the key in pads,
	 * arguably faster than memset and memcpy.  */
	bzero(k_ipad, sizeof k_ipad);
	bzero(k_opad, sizeof k_opad);
	bcopy(K, k_ipad, key_length);
	bcopy(K, k_opad, key_length);

	/* Step 2 as per rfc2401, XOR the key with ipad.
	 * Just for sake of ease we do step 4 here also which is to XOR
	 * key with opad */
	for (int i=0; i<64; i++) {
		k_ipad[i] ^= 0x36;
		k_opad[i] ^= 0x5c;
	}

	/* Step 3, perform inner md5*/
	md5_engine_ctx2.update(k_ipad, 64);
	md5_engine_ctx2.update(decoded_str.c_str(), strlen(decoded_str.c_str()));

	unsigned char digest[16];
	memcpy(digest, md5_engine_ctx2.digest, 16);
	md5_engine_ctx2.finalize();

	/* step 5, perform outer md5 */
	md5_engine_ctx3.update(k_opad, 64);
	md5_engine_ctx3.update(md5_engine_ctx2.digest, 16);
	md5_engine_ctx3.finalize();
	std::string hash = md5_engine_ctx3.hexdigest();

#if TESTING_ENABLED
	if (strcmp(test_digest, hash.c_str()) == 0) {

		printf("Testing: Hash works.\n");

	} else {
		fprintf(stderr, "Testing: lame, Hash doesn't work.\n");
		fprintf(stderr, "Testing: Expected, %s\nGot, %s\n", test_digest, hash.c_str());
	}
#endif

	/*Prepend username and a space character to the hash */
	hash.insert(0, " "); //space added
	hash.insert(0, (char *)uname); //username added

	printf("RESP: %s\n", hash.c_str());

	/* Base64 Encode */
	std::string encoded_hash = base64_encode((unsigned char *)hash.c_str(), strlen(hash.c_str()));

#if TESTING_ENABLED
	char *expected = "tim b913a602c7eda7a495b4e6e7334d3890";
	if (strcmp(hash.c_str(), expected) == 0) {
		printf("Testing: Un-encoded string matches.\n");
	} else {
		fprintf(stderr, "Testing: Prepended string is malformed.\n");
	}
	expected = "dGltIGI5MTNhNjAyYzdlZGE3YTQ5NWI0ZTZlNzMzNGQzODkw";
	if (strcmp(encoded_hash.c_str(), expected) == 0) {
		printf("Testing: Encoded string matches.\n");
	} else {
		fprintf(stderr, "Testing: Base64 encoding didn't work.\n");
	}
#endif
	printf("Encoded Response: %s\n", encoded_hash.c_str());

	char msg_buf[BUFFER_SIZE];
	char appended_buf[BUFFER_SIZE];

	sprintf(msg_buf, "{\"jsonrpc\": \"%.1f\", \"method\": \"%s\", \"params\": [\"%s\"], \"id\": %d}",
				msg.version, msg.method_type, encoded_hash.c_str(), msg.id);

	sprintf(appended_buf, "%zu:%s,", strlen(msg_buf), msg_buf);

	memset(_internal_buffer, 0, strlen(_internal_buffer));
	memcpy(_internal_buffer, appended_buf, strlen(appended_buf));

	return _internal_buffer;
}

/* Initilizes a linked list of parameters */
void JsonOverNetstring::param_list_init(parameter_list_s *list)
{
	/* Initialize a list */
	list = new parameter_list_s;
	list->param = NULL;
	list->next = NULL;

}

/* Adds an item to the list */
void JsonOverNetstring::add_to_param_list(parameter_list_s **list_head, const char *data, size_t len)
{
	/* Allocate space for the new entry to the list */
	parameter_list_s *cur = new parameter_list_s;

	/* Allocate memory for the data
	 * xxx weird. Add 1 byte for null termination*/
	cur->param = new char [len+1];
	memset(cur->param, 0, len);
	memcpy(cur->param, data, len);
	/*xxx weird. Null terminate the string. I thought when we convert from
	 * std::string to c_str(), it would give us a null terminated string. Seemingly this
	 * assumption is not correct. */
	cur->param[len] = '\0';

#if TESTING_ENABLED
	printf("Adding Param to list=%s, original=%s\n", cur->param, data);
#endif

	/*dereference head and make cur the new head*/
	cur->next = *list_head;
	*list_head = cur;
}

/* Removes an item from the list */
void JsonOverNetstring::remove_from_param_list(parameter_list_s **list_head, const char *data, size_t len)
{
	parameter_list_s *cur = *list_head;

	/* Do nothing if the list is empty*/
	if (cur == NULL) {
		return;
	}

	/*Iterate over the list, until the tail is reached.
	 * Find the item whose data matches your provided data string
	 * Delete it. Set the next pointer and continue */
	while (cur->next != NULL) {
		if (cur->next->param == data) {
			parameter_list *next;
			/*copy the pointer of the element which is next to the one we want
			 * to delete. */
			next = cur->next->next;

			/*axe the matched entry.
			 * We are one one leaf behind so its safe to axe . */
			delete cur->next->param;
			delete cur->next;

			/*Link the current pointer to the one after the axed one*/
			cur->next = next;
		} else {
			/* Move to the next pointer */
			cur = cur->next;
		}
	}
}

void JsonOverNetstring::delete_param_list(parameter_list_s **list_head)
{
	parameter_list_s *cur = *list_head;

	/* Do nothing if the list is empty*/
	if (cur == NULL) {
		return;
	}

	/*Iterate over the list, until the tail is reached. */
	while (cur != NULL) {

		parameter_list *next;
		/* copy the pointer to the next element */
		next = cur->next;
		/* axe current data pointer and then current pointer itself */
		delete cur->param;
		delete cur;
		/* move to the next element */
		cur = next;
	}

	/*dereference the head*/
	*list_head = NULL;

}

/* A non-member helper function to trim white space from the front or end
 * of strings */
char *trim(char *str)
{
    size_t len = 0;
    char *frontp = str;
    char *endp = NULL;

    if( str == NULL ) { return NULL; }
    if( str[0] == '\0' ) { return str; }

    len = strlen(str);
    endp = str + len;

    /* Move the front and back pointers to address the first non-whitespace
     * characters from each end.
     */
    while( isspace((unsigned char) *frontp) ) { ++frontp; }
    if( endp != frontp )
    {
        while( isspace((unsigned char) *(--endp)) && endp != frontp ) {}
    }

    if( str + len - 1 != endp )
            *(endp + 1) = '\0';
    else if( frontp != str &&  endp == frontp )
            *str = '\0';

    /* Shift the string so that it starts at str so that if it's dynamically
     * allocated, we can still free it on the returned pointer.  Note the reuse
     * of endp to mean the front of the string buffer now.
     */
    endp = str;
    if( frontp != str )
    {
            while( *frontp ) { *endp++ = *frontp++; }
            *endp = '\0';
    }


    return str;
}


/* Parses the server reponses */
ServerResponse *JsonOverNetstring::JsonResponsParser(char *data)
{
	/* keywords for JSON RPC 2.0
	 * We give them an order number which is specific to our implementation
	 * and completely heuristic*/
	char error[] = "error";
	char message[] = "message";
	char code[] = "code";
	char result[] = "result";
	char id[] = "id";

	/* Helper booleans */
	bool wasError = false;
	bool wasErrorMessage = false;
	bool wasErrorCode = false;
	bool wasResult = false;
	bool wasID = false;

	char *parsed_msg = strtok(data, ":{},\"");

	while (parsed_msg != NULL) {
		/* Problem: was that we want to retrieve full error message like "foo bar blah"
		 * with spaces from JSON. Our current logic is that we pick up interesting things
		 * along the way when we see something coming. For example if we see 'result:'
		 * keyword, we know that the next item is the result string.
		 * strtok() returns an emtpy character/characters for each delimiter when
		 * encountered. We don't want anything in between our keywords and logical pickup
		 * code.
		 * Coarse Solution: We duplicate the string and send it to a helper function
		 * who trims it for any white space. After that we check that if the string is
		 * empty. If empty we go out otherwise carry on. */

		char *new_str = strdup(parsed_msg);
		parsed_msg = trim(new_str);

		if(parsed_msg[0] == '\0') {
			goto out;
		}

		// This block searches for the json objects we are interested in.
		// If found, we set a boolean, get out and come back again

		/* Response ID, must be the same as the request */
		if (strcmp(id, parsed_msg) == 0) {
			wasID = true;
			goto out;
		}
		/* A success response will be method_type result*/
		if (strcmp(result, parsed_msg) == 0) {
			wasResult = true;
			goto out;
		}

		/* An error response is more complicated as compared to result
		 * It contains a message and an error code */
		if (strcmp(error, parsed_msg) == 0) {
			wasError = true;
			goto out;
		}
		/* Inside the error block, we search for message or code */
		if (wasError) {
			if (strcmp(message, parsed_msg) == 0) {
				wasErrorMessage = true;
				goto out;
			}
			if (strcmp(code, parsed_msg) == 0) {
				wasErrorCode = true;
				goto out;
			}
		}

		// Next block, we came back. Now handle values for nodes based
		//upon the booleans set in the previous block

		/* If we have found result */
		if (wasResult) {
			string temp = static_cast<string>(parsed_msg);
			_nodes.insert(make_pair(RESULT, temp));
			wasResult = false;
		}

		/* If we have found ID */
		if (wasID) {
			_nodes.insert(make_pair(ID, parsed_msg));
			wasID = false;
		}

		/* if we have found an error, get its message */
		if (wasErrorMessage) {
			_nodes.insert(make_pair(ERROR, parsed_msg));
			wasErrorMessage = false;
		}
		/* if we have found an error, get its code*/
		if (wasErrorCode) {
			_nodes.insert(make_pair(CODE, parsed_msg));
			wasErrorCode = false;
		}

out:
		parsed_msg = strtok(NULL, ":{},\"");
#if TESTING_ENABLED
		printf("PARSED: %s\n", parsed_msg);
#endif

	}

	// return the ServerResponse

	return &_nodes;
}

