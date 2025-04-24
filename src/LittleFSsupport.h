#ifndef _LITTLE_FS_SUPPORT_H
#define _LITTLE_FS_SUPPORT_H
/*
   LittleFSsupport.cpp
 * (c)2025 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This generated code may be freely used for both private and commercial use
 * provided this copyright is maintained.
 */

#include <FS.h>
#include <LittleFS.h>

/* ===================
r   Open a file for reading. If a file is in reading mode, then no data is deleted if a file is already present on a system.
r+  open for reading and writing from beginning

w   Open a file for writing. If a file is in writing mode, then a new file is created if a file doesn’t exist at all. 
    If a file is already present on a system, then all the data inside the file is truncated, and it is opened for writing purposes.
w+  open for reading and writing, overwriting a file

a   Open a file in append mode. If a file is in append mode, then the file is opened. The content within the file doesn’t change.
a+  open for reading and writing, appending to file
============== */

bool initializeFS(); // returns false if fails
void showLittleFS_size(Print *outPtr);
void listDir(const char * dirname); // list to debugOut NOT recursive!!
void listDir(const char * dirname, Print& out); // list to out   NOT recursive!!
void listDir(const char * dirname, Print* outPtr); // list to out   NOT recursive!!


#endif
