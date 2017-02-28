/*
 * SolveAssignment.cpp
 *
 *  Created on: Feb 3, 2017
 *      Author: hasnain
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include "SolveAssignment.h"
#include "FibonacciParser.h"
#include "TBClient.h"
#include "JsonOverNetstring.h"

using namespace std;

/**
 * @brief Delegates to lower layers for plain tex auth request message.
 * @param clt, pointer to client instance
 */
void SolveAssignment::PlainAuthRequest() {

	/*3.1:
	 * Prepare a JSON RPC  2.0 Over Netstring Transport Object with plain text
	 * authentication request configuration*/
	JsonOverNetstring json; //instantiate JSON framework
	clt_msg_s client_msg; //a client msg data structure
	client_msg.version = 2.0;			  //Fill up the data structure
	client_msg.id = 1;
	char method_type[] = "auth_request";
	client_msg.method_type = method_type;
	client_msg.params = NULL;
	/* JSON framework returns a JSON over Netstring compliant string */
	const char *toSend = json.auth_request(client_msg);

	/*3.2:
	 * Send this to server*/
	if (_clt->isConnected()) {
		_clt->SendMsg(toSend, strlen(toSend));
	} else {
		fprintf(stderr, "Connection down.");
		exit(1);
	}
}

/**
 * @brief Delegates to lower layers for encrypted auth response message.
 *
 * @param clt, pointer to client instance
 * @param u, username string
 * @param p, password string
 * @param c, challenge string
 */
void SolveAssignment::CryptAuthResp(const char *u, const char *p, const char *c)
{
	/*5.1:
	 * Prepare a JSON RPC  2.0 Over Netstring Transport Object with encrypted
	 * authentication response configuration. */
	JsonOverNetstring json; //instantiate JSON framework
	clt_msg_s client_msg; //a client msg data structure
	client_msg.version = 2.0;			  //Fill up the data structure
	client_msg.id = 1;
	char method_type[] = "auth_response";
	client_msg.method_type = method_type;
	parameter_list_s *list = NULL;
	json.param_list_init(list);
	json.add_to_param_list(&list, u, strlen(u));
	json.add_to_param_list(&list, p, strlen(p));
	client_msg.params = list;

	/* JSON framework returns a JSON over Netstring compliant string
	 * Job Summary in 5.1:
	 * - Base64 decode the server challenge
	 * - Undergo HMAC Algorithm
	 * - Prepare response to complete CRAM-Md5 protocol
	 * */
	const char *toSend = json.EncryptedAuthMsg(client_msg, c);

	/*5.2:
	 * Send response to the server
	 */
	_clt->SendMsg(toSend, strlen(toSend));
}

/* Delegates handling of server responses */
void SolveAssignment::ServerResponseHandle()
{
	/* First a little clean up. If we already have an instance of
	 * _reponse kicking around, we nail it down and free any memory allocated
	 * for result or error messagees from previous server responses.
	 * Then axe _reponse itself*/
	if (_response != NULL) {
		if (_response->handle.result) {
			delete _response->handle.result;
		} else if (_response->handle.error->message) {
			delete _response->handle.error->message;
		}

		delete _response;
	}

	/* Resurrect _reponse again.
	 * And fill it in with current server responses */
	_response = new serv_resp_msg_s;
	_clt->HandleResponse(_response);

}

SolveAssignment::SolveAssignment(const char *uname, const char *pwd)
: _response(NULL), _clt(NULL)
{
	/* STEP 1:
	 * We need to decipher a secret file, haystack.bin
	 * Hints were provided. Its pretty clear that the interesting characters
	 * which need our attention are located at indexes which are Fibonacci numbers.
	 * So, we process the file and dig out hidden secret. */
	FibonacciParser fibonacci("haystack.bin");
	fibonacci.ProcessContent();

	/*STEP 2:
	 * Establish a TCP connection with tosibox server.
	 * www.tosibox.com, port=0xBAD
	 * Fully qualified name was provided, port number converted from hex to decimal*/
	_clt = new TBClient("www.tosibox.com", "2989");
	_clt->Connect(); //Connect with the server

	/*STEP 3:
	 * Go through the Plain Text Authentication Request and Server Challenge Response
	 * procedure*/
	PlainAuthRequest();

	/*STEP 4:
	 * Handle Response of the request you made in the step 3 */
	ServerResponseHandle();

	/* Check State
	 * If we haven't received the Challenge from server, bail out.*/
	if (_clt->state != CHALENGE_RECVD) {
		fprintf(stderr, "Disconnecting.");
		_clt->Disconnect();
		exit(1);
	}

	/*Step 5:
	 * Go through the Encrypted Authentication Response and Server Authentication
	 * procedure. This step is ofcourse the heart of this assignment */
	CryptAuthResp(uname, pwd, _response->handle.result);

	/*STEP 6:
	 * Handle Response of the request you made in the step 5 */
	ServerResponseHandle();

	/* Check State
	 * */
	if (_clt->state == AUTHENTICATED) {
		printf("Assignment Solved\n");
		_clt->Disconnect();
		exit(0);
	}

	fprintf(stderr, "Not Authenticated\n");
	_clt->Disconnect();
	 exit(1);

}

SolveAssignment::~SolveAssignment() {
	/* last ditch effort to close the connection, I don't like my machine
	 * having to many stagnant connections still open */
	if(_clt->isConnected()) {
		_clt->Disconnect();
	}
}

