#ifndef _DEBUG_OUT_H
#define _DEBUG_OUT_H
// call setDebugOut( ) to set output to send debug to
// NOTE: Still need to #define DEBUG in each file you want debug output from

/**
 * (c)2025 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This generated code may be freely used for both private and commercial use
 * provided this copyright is maintained.
*/

#include <Arduino.h>
Print* getDebugOut();
void setDebugOut(Print *outPtr);

#endif
