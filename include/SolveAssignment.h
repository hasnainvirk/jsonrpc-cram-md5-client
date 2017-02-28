/*
 * SolveAssignment.h
 *
 *  Created on: Feb 3, 2017
 *      Author: hasnain
 */

#ifndef INCLUDE_SOLVEASSIGNMENT_H_
#define INCLUDE_SOLVEASSIGNMENT_H_

#include "TBClient.h"
#include "JsonOverNetstring.h"
/**
 * \class Solves the Tosibox assignment.
 */
class SolveAssignment {
public:
	/**
	 * @brief Constructor
	 * @param uname username string
	 * @param pwd password string */
	SolveAssignment(const char *uname, const char *pwd);

	/**
	 * @brief Destructor
	 * */
	~SolveAssignment();

private:
	/**
	 * Server response data structure
	 */
	serv_resp_msg_s *_response;

	/**
	 * Instantiation of a Tosibox Client Class
	 */
	TBClient *_clt;

	/**
	 * @brief Delegates the Authentication Request sequence to the lower layers.
	 *
	 * i.e., Asks JSON frameork to prepare a valid Authenticatio Request message
	 *       with proprt JSON over Netstring configuration. Then asks the Client
	 *       to send it through and finally kicks the Client Response Handler
	 *       mechanism.
	 */
	void PlainAuthRequest();

	/**
	 * @brief Delegates Authentication Response sequence to the lower layers.
	 *
	 * i.e., Asks JSON framework to kick in CRAM-MD5 protocol and prepare
	 *       Authentication response. Then asks the Client class to send the
	 *       message through and later kick in the Client mechanism to handle
	 *       server response.
	 * @param u Username
	 * @param p password/key/secret
	 * @param c server challenge
	 */
	void CryptAuthResp(const char *u, const char *p, const char *c);

	/**
	 *@brief Delegates Handling of server responses to the lower layers.
	 */
	void ServerResponseHandle();

	/**
	 * Disallow copy constructor
	 */
	SolveAssignment(const SolveAssignment&);

	/**
	 * Disallow operator overload
	 */
	SolveAssignment& operator=(const SolveAssignment&);
};



#endif /* INCLUDE_SOLVEASSIGNMENT_H_ */
