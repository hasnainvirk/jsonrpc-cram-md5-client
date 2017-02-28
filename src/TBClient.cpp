/*
 * TBClient.cpp
 *
 *  Created on: Jan 27, 2017
 *      Author: hasnain
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "TBClient.h"
#include "JsonOverNetstring.h"

#define CLT_BUFFER_SIZE  500 //bytes
#define SA	(struct sockaddr *)

#ifdef TEST
#define TESTING_ENABLED 1
#endif


void TBClient::_error(const char *msg)
{
	perror(msg);
	exit(0);
}

TBClient::TBClient(const char *server_addr, const char *port):
		_address(server_addr),
		_port(port),
		_sock_fd(-1)
{
	_remote_addr = new char [256];
	state = NOT_CONNECTED;
}

bool TBClient::isConnected()
{
	if (state == CONNECTED) {
		return true;
	}

	return false;
}

TBClient::~TBClient()
{
	delete [] _remote_addr;
}

void TBClient::Connect(void) {

	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int retcode;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; //try getting any IPv4, tosibox does not seem to have IPv6 support
	hints.ai_socktype = SOCK_STREAM; // we want TCP
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = hints.ai_flags|AI_NUMERICSERV;

	int port_num = atoi(_port);

	if (port_num < 1 || port_num > 65535) {
		_error("Invalid port number");
	}

	/* Check for valid server address by trying to resolve it */
	retcode = getaddrinfo("www.tosibox.fi", _port, &hints, &result);
	if (retcode != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(retcode));
		exit(1);
	}

	/* Loop through the resultant addrinfo and try to connect.
	 * If none of the adresses retrieved works out, bail out. */
	for(rp=result; rp != NULL; rp=rp->ai_next) {
		/* Try opening the Socket */
		if ((_sock_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) < 0) {
			close(_sock_fd);
			_sock_fd = -1;
			continue;
		}

		/* we would like to reuse the socket port in case of failures, retries or
		 * whatever. If a  TCP socket is in TIME-WAIT state, linux machine will
		 * not let you open socket on that port which is still hanging around.
		 * This socket option would let us do that */
		int val = 1;
		setsockopt(_sock_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int));


		/* Try connecting to remote host */
		if (connect(_sock_fd, rp->ai_addr, rp->ai_addrlen) < 0) {
			/* A failed connect attempt is not fatal. As we are iterating over the
			 * address linked list retrieved from getaddrinfo. Its quite possible
			 * that some of the addresses found from the databases cannot be connected
			 * to.
			 * */
			close(_sock_fd);
			_sock_fd = -1;
			fprintf(stderr, "SOCKET: connect() - Attempt Failed\n");
			continue;
		}

		/* If we reached this point, then we are successfully connected.*/
		char service[256];
		char remote_addr[256];

		struct addrinfo *p;

		for (p = result; p != NULL; p = p->ai_next) {
			/* Its the reverse of getaddrinfo(). Handy shortcut to get
			 * human readable IP address */
			getnameinfo(rp->ai_addr, rp->ai_addrlen, remote_addr,
					sizeof(remote_addr), service, sizeof(service), NI_NUMERICHOST);
		}

		memcpy(_remote_addr, remote_addr, sizeof remote_addr);
		printf("CONNECT: IP Address: %s, Port: %s\n", _remote_addr, _port);
		printf("CONNECT: Connected.\n");
		state = CONNECTED;
		break;
	}

	if (rp == NULL) {
		/* This means that we looped off the list without a successful connection.
		 * Bail out.*/
		_error("Connection Failed.\n");
	}

	/* Alright, everything went well.
	 * Time for house cleaning*/
	freeaddrinfo(result);
}

void TBClient::Disconnect()
{
	/*Close the socket*/
	close(_sock_fd);
	_sock_fd = -1;
	state = NOT_CONNECTED;
}

/* The message is preformed, this method just send it through the socket
 * and kicks the state machine one step forward.*/
int TBClient::SendMsg(const char *msg, size_t length)
{
	printf("SendMsg: %s\n", msg);
	/* Blocking send */
	int retcode = send(_sock_fd, msg, length, 0);

	if (retcode < 0) {
		_error("SendMsg: Sending failed.");
	}

	if (state == CONNECTED) {
		state = AUTH_REQ_SENT;
	} else if (state == CHALENGE_RECVD) {
		state = ENCRYPTED_RESP_SENT;
	}

	printf("Sent %d bytes.\n", retcode);

	return retcode;
}

/* A little helper function to print out a decent error message.
 * These are actuall standard error codes for JSON RPC 2.0 alongwith one additional
 * error code that seems to be tosibox only. */
void print_error(signed short val) {
	switch (val) {
	case PARSE_ERROR:
		fprintf(stderr, "HandleResponse: Parse Error, %d\n", val);
		break;
	case INVALID_REQUEST:
		fprintf(stderr, "HandleResponse: Invalid Request, %d\n", val);
		break;
	case METHOD_NOT_FOUND:
		fprintf(stderr, "HandleResponse: Method not found, %d\n", val);
		break;
	case INVALID_PARAMETERS:
		fprintf(stderr, "HandleResponse: Invalid parameters, %d\n", val);
		break;
	case INTERNAL_ERROR:
		fprintf(stderr, "HandleResponse: Internal error, %d\n", val);
		break;
	case SERVER_ERROR:
		fprintf(stderr, "HandleResponse: Server error, %d\n", val);
		break;
	case TOSIBOX_CUSTOM:
		fprintf(stderr, "HandleResponse: Invalid Username or Password, %d\n", val);
		break;
	default:
		break;
	}
}

/* Important: We do not own resp_msg. Owner will do clean up
 *
 * Caller provides the client machinery with a pointer to a server response
 * message data structure. Its our job here to fill in that structure with a proper
 * valid response coming from the server when it eventually arrives.
 *
 * Job summary:
 * - Reads from the socket.
 * - Utilizes JSON framework to parse the incoming server response
 * - Sets up resp_msg for the caller according to the received server response
 * - Advances the state machine  */
void TBClient::HandleResponse(serv_resp_msg_s *resp_msg)
{
	char recvd[CLT_BUFFER_SIZE];
	JsonOverNetstring json;
	ServerResponse *serv_resp;

	printf("RECEIVING...\n");
	/*Blocking read*/
	int retcode = recv(_sock_fd, recvd, CLT_BUFFER_SIZE, 0);

	if (retcode < 0) {
		_error("Read: Reading failed.");
	}

	printf("Received, %d bytes.\n", retcode);

	serv_resp = json.JsonResponsParser(recvd);

	/* Transaction ID*/
	if (serv_resp->find(ID) != serv_resp->end()) {
		std::string id = serv_resp->find(ID)->second;
		printf("ServerResponse->ID: %s\n", id.c_str());
		resp_msg->id = atoi(id.c_str());
	}

	/* Success Result */
	if (serv_resp->find(RESULT) != serv_resp->end()) {
		std::string result = serv_resp->find(RESULT)->second;
		printf("HandleResponse->Result: %s\n", result.c_str());
		char method_type[] ="result";
		resp_msg->method_type = method_type;
		resp_msg->handle.result = new char [strlen(result.c_str())];
		memcpy(resp_msg->handle.result, result.c_str(), strlen(result.c_str()));
		if (state == AUTH_REQ_SENT) {
			/* then step the state machine */
			state = CHALENGE_RECVD;
		} else if (state == ENCRYPTED_RESP_SENT) {
			state = AUTHENTICATED;
		}
	}

	/* Failure msg and code*/
#if TESTING_ENABLED
	if (serv_resp->find(ERROR) != serv_resp->end()) {
		string error_msg = serv_resp->find(ERROR)->second;
		printf("ServerResponse->ErrorMessage: %s\n", error_msg.c_str());

	}
#endif

	if (serv_resp->find(CODE) != serv_resp->end()) {
		string error_code = serv_resp->find(CODE)->second;
		printf("ServerResponse->ErrorCode: %s\n", error_code.c_str());
		signed short code = atoi(error_code.c_str());
		print_error(code);
#if TESTING_ENABLED
		/* We should mark the test as passed.
		 * We had tampered with the username and password.
		 * Purpose of the test was to check if the base64 encoding, Md5 engine and
		 * CRAM/HMAC algorithms were working properly or not.*/
		if (code == TOSIBOX_CUSTOM) {
			printf("Test Passed.\n");
		}
#endif

	}
}
