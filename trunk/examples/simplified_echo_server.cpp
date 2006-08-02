/**
 * File: simplified_echo_server.cpp
 * 
 * A simplified example of how to use reudp library
 * to implement an UDP echo server.
 * 
 * Useful mainly to see the basic usage in a quick
 * glance.
 */
#include <list>
#include <string>
#include <iostream>
#include <sstream>

#include <reudp/dgram.h>

#define UDP_BUFFER_SIZE 65536

reudp::dgram sock;

const char *usage = 
"Usage: reudp_server <port>";

void do_server() {
	ACE_DEBUG((LM_DEBUG, "Waiting datagrams\n"));
	char buffer[UDP_BUFFER_SIZE];
	while (1) {
		reudp::addr_inet_type from_addr;
		size_t n = sock.recv(buffer, UDP_BUFFER_SIZE, from_addr);
		ACE_DEBUG((LM_DEBUG, "Received data of size %d\n", n));

		if (n > 0) {
			// If received data is 0 then it was likely an ACK from client
			// Echo the data back to sender
			sock.send(buffer, n, from_addr);
		} else if (n == 0) {
			// In this case the received data was something that
			// the ReUDP library consumed so no payload data is
			// returned to the application.
			ACE_DEBUG((LM_DEBUG, "No payload data received"));
		} else {
			ACE_ERROR((LM_ERROR, "TODO some error happened, return value %d\n",
			           n));
			// TODO ACE functions can be used to get the error
		}

		// After each receive allow the possibility for sending
		// acks back.
		if (sock.needs_to_send())		
			sock.send(NULL, 0, from_addr);			
	}
}

void do_main(int argc, ACE_TCHAR *argv[]) {
	if (argc != 2)
		throw "Invalid arguments";

	unsigned short port;
	std::stringstream portstr;
	portstr << argv[1];
	portstr >> port;
	
	// Open the server port
	ACE_DEBUG((LM_DEBUG, "Opening UDP echo server port %d\n", port));
	if (sock.open(reudp::addr_inet_type(port)) == -1)
		throw "Could not open UDP server port";
		
	do_server();
}

int
ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{
	if (argc <= 1) {
		std::cerr << usage << std::endl;
		return -1;
	}
	
	try {
		do_main(argc, argv);
	} catch (std::exception &e) {
		ACE_ERROR((LM_ERROR, "Exception caught:\n"));
		ACE_ERROR((LM_ERROR, "%s\n", e.what()));
		ACE_ERROR((LM_ERROR, usage));
		return -1;
	} catch (const char *err) {
		std::cerr << "Error: " << err << std::endl;
		std::cerr << usage << std::endl;
		return -1;
	}

	return 0;
}
