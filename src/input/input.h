#ifndef __INPUT_H
#define __INPUT_H

#include <stdlib.h>
#include "../observer/inputobserver.h"
#include "../global.h"

class Input {
protected:
        InputObserver* _observer;
        int _id;
public:

        void setInputObserver(int id, InputObserver* observer){
            this->_observer = observer; 
            this->_id = id;
        }

};

#endif // __INPUT_H