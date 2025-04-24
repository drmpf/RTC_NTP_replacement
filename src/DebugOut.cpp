// DebugOut.cpp

// call setDebugOut( ) to set output to send debug to
// NOTE: Still need to #define DEBUG in each file you want debug output from

/*
 * (c)2025 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This generated code may be freely used for both private and commercial use
 * provided this copyright is maintained.
 */

#include "DebugOut.h"

static Print* debugOutPtr = NULL;
Print* getDebugOut() {
    return debugOutPtr;
}

void setDebugOut(Print *outPtr) {
    debugOutPtr = outPtr;
}
    
