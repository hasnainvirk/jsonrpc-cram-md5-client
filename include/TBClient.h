/*
 * TBClient.h
 *
 *  Created on: Jan 27, 2017
 *      Author: hasnain
 */

#ifndef TBCLIENT_H_
#define TBCLIENT_H_

#include <inttypes.h>
#include "JsonOverNetstring.h"
/* This class works on the assumption that we have a nix type
 * OS under the hood.*/

/**
 * Basically the state machine for the
 * tosibox client.
 */
typedef enum{
	NOT_CONNECTED=0,    //!< NOT_CONNECTED
	CONNECTED,          //!< CONNECTED
	AUTH_REQ_SENT,      //!< AUTH_REQ_SENT
	CHALENGE_RECVD,     //!< CHALENGE_RECVD
	ENCRYPTED_RESP_SENT,//!< ENCRYPTED_RESP_SENT
	AUTHENTICATED       //!< AUTHENTICATED
}connection_status;

/**
 * \class TBClient
 *
 * A Tosibox client
 *
 * Creates a TCP connection with tosibox server.
 * Handles a Plain Authentication request with the server.
 * Handles an Encrypted session with the server.
 * Provides HMAC algorithm.
 * Utilizes helper classes like Base64 encoding, JSON framework, and MD5 engine
 * to drive HMAC algorithm so as to implement CRAM-MD5 protocol.
 */
class TBClient
{
public:
	/**
	 * @brief Constructor
	 * @param server_address FQDN of tosibox server
	 * @param port port number of the server
	 */
	TBClient(const char *server_address, const char *port);

	/**
	 * @brief Destructor
	 */
	~TBClient();

	/**
	 * @brief Connects to the server
	 *
	 * Resolves the fqdn address so as to get the IP address.
	 * Opens up a TCP socket
	 */
	void Connect(void);

	/**
	 * @brief Disconnects from the server
	 *
	 * Closes the socket.
	 */
	void Disconnect(void);

	/**
	 * @brief Checks if the connection is open or not
	 * @return true or false depending upon the state of connection
	 */
	bool isConnected();

	/**
	 * @brief Writes a message to the server.
	 *
	 * @param data a JSON RPC 2.0 compliant string over Netstring transport
	 * @param length length of the string
	 * @return length of bytes written
	 */
	int SendMsg(const char *data, size_t length);

	/**
	 * Handles response of the server.
	 *
	 * This is a multi-faceted method. It handles reading from the socket,
	 * handles responses from the server as well as  handles/prepares/delegates
	 * responses to the server.
	 *
	 * @param serv_resp a server response data structure
	 */
	void HandleResponse(serv_resp_msg_s *serv_resp);

	/**
	 * State machine status
	 */
	uint8_t state;

private:
	const char *_address; //tosibox FQDN
	const char *_port; //port
	char *_remote_addr; //tosibox IP address
	int _sock_fd; // socket ID

	/**
	 * Handles errors.
	 * Exits as an error
	 * @param msg a helpful message string
	 */
	void _error(const char *msg);

	/**
	 * Disallow copy constructor.
	 */
	TBClient(const TBClient&);

	/**
	 * Disallow operator overload
	 */
	TBClient& operator=(const TBClient&);
};

#endif /* TBCLIENT_H_ */
