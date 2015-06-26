#ifndef __INPUTOBSERVER_H
#define __INPUTOBSERVER_H
#include <stdint.h>

class InputObserver {
public:
    virtual void onEncodedDataReceived(int id, uint8_t type, uint8_t* data, int size) = 0;
};


#endif // __INPUTOBSERVER_H
