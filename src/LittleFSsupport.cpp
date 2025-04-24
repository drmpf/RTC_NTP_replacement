/*
   LittleFSsupport.cpp
   by Matthew Ford,  2021/12/06
 * (c)2025 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This generated code may be freely used for both private and commercial use
 * provided this copyright is maintained.
*/

// need https://github.com/lorol/arduino-esp32littlefs-plugin  to upload

#include "LittleFSsupport.h"
#include "esp_err.h"
#include "esp_littlefs.h"
#include "DebugOut.h"

//#define DEBUG

static Print* debugPtr = NULL;  // local to this file
static bool FS_initialized = false;

/* ===================
  r   Open a file for reading. If a file is in reading mode, then no data is deleted if a file is already present on a system.
  r+  open for reading and writing from beginning

  w   Open a file for writing. If a file is in writing mode, then a new file is created if a file doesn’t exist at all.
    If a file is already present on a system, then all the data inside the file is truncated, and it is opened for writing purposes.
  w+  open for reading and writing, overwriting a file

  a   Open a file in append mode. If a file is in append mode, then the file is opened. The content within the file doesn’t change.
  a+  open for reading and writing, appending to file
  ============== */

bool initializeFS() {
  if (FS_initialized) {
    return FS_initialized;
  }

#ifdef DEBUG
  debugPtr = getDebugOut();
#endif

  if (debugPtr) {
    debugPtr->println("Mount LittleFS");
  }
  if (!LittleFS.begin()) {
    if (debugPtr) {
      debugPtr->println("LittleFS mount failed");
    }
    return FS_initialized;
  }
  if (debugPtr) {
    showLittleFS_size(debugPtr);
  }
  // else
  FS_initialized = true;
  listDir("/");
  return FS_initialized;
}

void listDir(const char * dirname) {
  if (!debugPtr) {
    return; // no where to send the output
  }
  listDir(dirname, *debugPtr);
}

void showLittleFS_size(Print *outPtr) {
  if (!outPtr) {
    return;
  }
  size_t total = 0, used = 0;
  esp_err_t ret = esp_littlefs_info("spiffs", &total, &used);
  if (ret != ESP_OK)   {
    outPtr->print(" Failed to get LittleFS partition information "); outPtr->println(esp_err_to_name(ret));
  }  else   {
    outPtr->print(" LittleFS size:"); outPtr->print(total / 1000); outPtr->print(" kB   used:"); outPtr->print(used / 1000.0,2); outPtr->println(" kB");
  }
}
void listDir(const char * dirname, Print& out) {
  listDir(dirname, &out);
}

void listDir(const char * dirname, Print* outPtr) {
  if (!outPtr) {
    return;
  }
  if (!FS_initialized) {
    outPtr->println("FS not initialized yet");
    return;
  }

  outPtr->printf("Listing directory: %s\r\n", dirname);
  File root = LittleFS.open(dirname);
  if (!root) {
    outPtr->println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    outPtr->println(" - not a directory");
    return;
  }
  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      outPtr->print("  DIR: ");
    } else {
      outPtr->print("  FILE: ");
    }
    outPtr->print(file.name());
    outPtr->print("  SIZE: ");
    outPtr->print(file.size());
    outPtr->println(" B");
    file = root.openNextFile();
  }
  outPtr->println();
}
