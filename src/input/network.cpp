#include "network.h"

#include <iostream>

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include "../global.h"

using namespace std;

inline int posix_close(int fd)
{
	cout << "called close" << endl;
	return close(fd);
}

UdpSocket::UdpSocket()
{
}

UdpSocket::~UdpSocket()
{
	close();
}

bool UdpSocket::initClient(std::string address, uint16_t port)
{
	// create socket
	if ((_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		cerr << "Could not initialize udp socket." << endl;
		return false;
	}

	// initialize local interface and port number
	memset(static_cast<void*>(&localAddress), 0, sizeof(struct sockaddr_in));
	localAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	localAddress.sin_family = AF_INET;
	localAddress.sin_port = 0;

	// bind socket to local interfaces and some random port number
	if (bind(_socket, reinterpret_cast<struct sockaddr*>(&localAddress), sizeof(struct sockaddr_in)) < 0) {
		cerr << "Could not bind socket." << endl;
		return false;
	}

	// initialize remote address
	memset(static_cast<void*>(&remoteAddress), 0, sizeof(struct sockaddr_in));
	remoteAddress.sin_port = htons(port);
	remoteAddress.sin_family = AF_INET;

	if (inet_aton(address.c_str(), &remoteAddress.sin_addr) == 0) {
		cerr << "Could not parse ip address!" << endl;
		return false;
	}

	readerThread = new thread([&](){
		(*this)();
	});

	return true;
}

bool UdpSocket::initServer(uint16_t port)
{
	// create socket
	if ((_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		cerr << "Could not initialize udp socket." << endl;
		return false;
	}

	// initialize local interfaces and port
	memset(static_cast<void*>(&localAddress), 0, sizeof(struct sockaddr_in));
	localAddress.sin_family = AF_INET;
	localAddress.sin_port = htons(port);
	localAddress.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(_socket, reinterpret_cast<struct sockaddr*>(&localAddress), sizeof(struct sockaddr_in)) < 0) {
		cerr << "Bind to socket failed!" << endl;
		return false;
	}

	readerThread = new thread([&](){
		(*this)();
	});

	return true;
}

void UdpSocket::close()
{
	if (_socket != -1) {
		int sd = _socket;
		_socket = -1;
		shutdown(sd, SHUT_RDWR);
		readerThread->join();
		delete readerThread;
	}
}

void UdpSocket::operator()()
{
	struct sockaddr_in incomming;
	struct sockaddr* in = reinterpret_cast<struct sockaddr*>(&incomming);
	socklen_t inlen = sizeof(struct sockaddr_in);
	// todo catch EINTR signal
	while (_socket != -1) {
		ssize_t result = 0;

		if (!headerValid) {
			uint8_t header[HEADER_SIZE];

			result = recvfrom(_socket, header, HEADER_SIZE, MSG_WAITALL, in, &inlen);
			if (result == 0) { // orderly shutdown
				break;
			} else if (result != HEADER_SIZE) {
				cerr << "Reading protocol-header failed!, Error: "  << strerror(errno) << endl;
				continue;
			}


			headerValid = true;
			payloadType = header[0];
			uint8_t* sizePtr = &header[1];
			payloadSize = *(reinterpret_cast<uint32_t*>(sizePtr));
			cout << "Reading header succeeded, size: " << payloadSize;
			cout << "Payload Type: " << payloadType << endl;

			if (payloadSize > 0) {
				payload = new uint8_t[payloadSize + 8]; // +8 because of decoder, otherwise we would have to copy everything!!
			} else {
				payload = nullptr;
			}

			headerValid = true;
				
		} else {
			if (payloadSize > 0) {
				result = recvfrom(_socket, payload + payloadPosition, payloadSize - payloadPosition, MSG_WAITALL, in, &inlen);
				if (result == 0) {
					break;
				} else if (result < 0) {
					cerr << "Reading payload failed! " << strerror(errno)<< ", recvsize: "<< result  << endl;
					continue;
				} else if (result + payloadPosition == payloadSize) {
					cout << "Reading payload succeeded, size: " << payloadSize << endl;
					payloadPosition = 0;

					if(payloadType == (int)PROTOCOL_TYPE_CLIENT_INIT){
						cout << "PROTOCOL_TYPE_CLIENT_INIT" << endl;
						int32_t* tmp = reinterpret_cast<int32_t*>(payload);

						if(payload == nullptr){
							cerr << "Error initilizing client" << endl;
							exit(1);
						}
						initClientCallback(tmp[0], tmp[1], tmp[2],tmp[3], tmp[4]);
						delete[] tmp;
					}else{
    					_observer->onEncodedDataReceived(_id, payloadType, payload, payloadSize);
					}

				} else {
					payloadPosition += result;
					continue;
				}
			} else if (payloadType == PROTOCOL_TYPE_INIT) {
				cout << "PROTOCOL_TYPE_INIT" << endl;
				remoteAddress = incomming;
				connectionCallback(&incomming, inlen);
			} else if (payloadType == PROTOCOL_TYPE_CLOSE) {
				closeConnectionCallback(&incomming, inlen);
			}

			headerValid = false;
		}
	}
}

void UdpSocket::send(uint8_t* data, int size)
{
	if (sendto(_socket, data, size, 0, reinterpret_cast<struct sockaddr*>(&remoteAddress), sizeof(struct sockaddr_in)) < 0) {
		cerr << "Could not send package with size: " << size << endl;
	}
}

void UdpSocket::send(uint8_t type, uint8_t* data, int size)
{
	uint8_t packetHeader[HEADER_SIZE];
	initHeader(packetHeader, type, size);
	send(packetHeader, HEADER_SIZE); // transmit header
	if (data != nullptr && size > 0) {
		send(data, size); // transmit payload
	}
}


