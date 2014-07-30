#ifndef __NETWORK_H
#define __NETWORK_H

#include <functional>
#include <string>
#include <thread>
#include <netinet/in.h>
#include <stdint.h>

#define HEADER_SIZE 5

/*
 * Protocol:
 * 1 Byte (uint8) for Packages Type -> 0x0 HeaderNAL, 0x1 FrameNAL (see global.h for more type information)
 * 4 Byte (int32) for NAL-Size (x264_nal_t::i_payload), Little Endian(!)
 * Payload (uint8*) x264_nal_t::p_payload
 */

typedef void connection_callback_t (struct sockaddr_in*, int);

class UdpSocket {
public:
        explicit UdpSocket();
        virtual ~UdpSocket();

        /**
         * Start reading loop!
         */
        void operator()();

        bool initClient(std::string address, uint16_t port);
        bool initServer(uint16_t port);
        void close();
        void send(uint8_t* data, int size);
        void send(uint8_t type, uint8_t* data, int size);

        template <typename ObjectType>
        void setReadCallback(ObjectType *instance, void (ObjectType::*callback)(uint8_t, uint8_t*, int))
        {
                readCallback = [=](uint8_t t, uint8_t* d, int s) {
                        (instance->*callback)(t, d, s);
                };
        }

        void setConnectionCallback(connection_callback_t* callback)
        {
                connectionCallback = callback;
        }

        template <typename ObjectType>
        void setConnectionCallback(ObjectType *instance, void (ObjectType::*callback)(struct sockaddr_in*, int))
        {
                connectionCallback = [=](struct sockaddr_in* saddr, int size) {
                        (instance->*callback)(saddr, size);
                };
        }

        void setCloseConnectionCallback(connection_callback_t* callback)
        {
                closeConnectionCallback = callback;
        }

        template <typename ObjectType>
        void setCloseConnectionCallback(ObjectType *instance, void (ObjectType::*callback)(struct sockaddr_in*, int))
        {
                closeConnectionCallback = [=](struct sockaddr_in* saddr, int size) {
                        (instance->*callback)(saddr, size);
                };
        }

private:
        int _socket = -1;
        struct sockaddr_in localAddress;
        struct sockaddr_in remoteAddress;

        std::thread *readerThread;
        std::function<void (uint8_t, uint8_t*, int)> readCallback;
        std::function<void (struct sockaddr_in*, int)> connectionCallback;
        std::function<void (struct sockaddr_in*, int)> closeConnectionCallback;

        bool headerValid = false;
        uint8_t payloadType = 0;
        int32_t payloadSize = 0;
        int32_t payloadPosition = 0;
        uint8_t *payload = nullptr;

};

#endif // __NETWORK_H
