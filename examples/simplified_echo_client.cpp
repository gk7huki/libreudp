/**
 * File: simplified_echo_client.cpp
 * 
 * A simplified example of how to use reudp library
 * to implement an UDP echo client.
 */
#include <list>
#include <string>
#include <utility>
#include <iostream>
#include <sstream>

#include <reudp/dgram.h>

#define UDP_BUFFER_SIZE 65536

reudp::dgram sock;
reudp::addr_inet_type host;

const char *usage = 
"Usage: reudp_client <port> [address]\n"
"  if address not specified, localhost assumed\n";

void do_client() {
	char buffer[UDP_BUFFER_SIZE];
	/* Note that this is by no means robust loop and
	 * can cause a hang because we assume the echo
	 * will be actually received. If it is lost in transmission
	 * we're blocking forever waiting for it to come.
	 */
	while (1) {
		ACE_DEBUG((LM_DEBUG, "Waiting user input\n"));
		std::cin.getline(buffer, UDP_BUFFER_SIZE);
		reudp::addr_inet_type from_addr;

		// Send the data if there is anything to send
		size_t buffer_len = strlen(buffer);
		if (buffer_len <= 0) continue;
		
		sock.send(buffer, buffer_len, host);
		
		// Wait for the echo
		while (1) {
			size_t n = sock.recv(buffer, UDP_BUFFER_SIZE, from_addr);

			if (n > 0) {
				std::string input(buffer, n);
				// If received data is 0 then it was likely an ACK from client
				// But since payload is received it should be the echo
				// for our data
				std::cout << "Received data:" 
				          << std::endl << input 
				          << std::endl;
				break;
			}
			// After each receive allow the possibility for sending
			// an ack back
			if (sock.needs_to_send())
				sock.send(NULL, 0, from_addr);			
		}
	}
}

void do_main(int argc, ACE_TCHAR *argv[]) {
	if (argc != 2 && argc != 3)
		throw "Invalid arguments";

	u_short port;
	std::string host_str("localhost");
	std::stringstream portstr;
	portstr << argv[1];
	portstr >> port;
	
	if (argc > 2)
		host_str = argv[2];

	if (host.set(port, host_str.c_str()) == -1)
		throw "Could not resolve address";
			
	ACE_DEBUG((LM_DEBUG, "Opening UDP echo client to server\n", 
	           host.get_host_addr(), host.get_port_number()));
	if (sock.open(reudp::addr_inet_type::sap_any) == -1)
		throw "Could not open UDP socket";
		
	do_client();
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
