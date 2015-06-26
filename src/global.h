#ifndef __GLOBAL_H
#define __GLOBAL_H

#include <stdint.h>

#define PROTOCOL_TYPE_INIT                      0x10
#define PROTOCOL_TYPE_CLOSE                     0x11

#define PROTOCOL_TYPE_HEADER                    0x20
#define PROTOCOL_TYPE_FRAME                     0x21

#define IS_NALU(a)                              ((a&0xF0) == 0x20)

#define STATUS_INPUT_END                        0x50

#define LEFT 1
#define RIGHT 2

inline void initHeader(uint8_t* header, uint8_t type, int size)
{
        header[0] = type;
        int32_t* payloadSize = reinterpret_cast<int32_t*>(header + 1);
        *payloadSize = size;
}


#endif // __GLOBAL_H