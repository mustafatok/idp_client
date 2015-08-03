#ifndef __GLOBAL_H
#define __GLOBAL_H

#include <stdint.h>

#define PROTOCOL_TYPE_INIT                      0x10
#define PROTOCOL_TYPE_CLOSE                     0x11

#define PROTOCOL_TYPE_HEADER                    0x20
#define PROTOCOL_TYPE_FRAME                     0x21
#define PROTOCOL_TYPE_CLIENT_INIT				0x22

#define IS_NALU(a)                              ((a&0xF0) == 0x20)

#define STATUS_INPUT_END                        0x50

#define MODE_VERTICALCONCAT                     0x60
#define MODE_LEFTRESIZED                        0x61
#define MODE_RIGHTRESIZED                       0x62
#define MODE_LEFTBLURRED                        0x63
#define MODE_RIGHTBLURRED                       0x64
#define MODE_INTERLEAVING                       0x65


#define LEFT 1
#define RIGHT 2

inline void initHeader(uint8_t* header, uint8_t type, uint32_t size)
{
        header[0] = type;
        uint32_t* payloadSize = reinterpret_cast<uint32_t*>(header + 1);
        *payloadSize = size;
}


#endif // __GLOBAL_H