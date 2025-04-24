/**
  RTC_TZ.ino
 * (c)2025 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This generated code may be freely used for both private and commercial use
 * provided this copyright is maintained.
*/
#include "RTC_NTP_replacement.h"
#include "BufferedOutput.h"


const int SQUARE_WAVE_PIN = 15; // the pin the RTC 1Hz output is connected to

static Print* debugPtr = NULL; // local debug for this file only
// controls debug for this file ONLY
#define DEBUG

createBufferedOutput(output, 512, DROP_IF_FULL); // buffered out with variable name output with buffer size 512 and mode drop chars if buffer full

/**
  Set a compiled default TZ here. Can be overrided/edited later by webpage.
  get_DefaultTZ() weakly defined in TZ_support.cpp
  if const char *get_DefaultTZ() is not defined then GMT0 is used as the default.
  See 
  https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv 
  for a list of time zones and their posix_tz strings
  that can be copied and pasted into the get_DefaultTZ or into the Set Time Zone webpage.
*/
const char *get_DefaultTZ() { // magic name picked up by TZ_Support.cpp
  // TZ_Australia_Sydney
  static char _default_tz_[50] = "AEST-10AEDT,M10.1.0,M4.1.0/3";
  return _default_tz_;
}


void setup() {
  Serial.begin(115200);
  // if debugging delay output for 5sec to allow time to open Monitor
#ifdef DEBUG
  for (int i = 10; i > 0; i--) {
    Serial.print(i); Serial.print(' '); delay(500);
  }
  Serial.println();
#endif
  output.connect(Serial); // connect the buffered output

#ifdef DEBUG
  debugPtr = &output;
  setDebugOut(debugPtr); // set the debug output for the library, still need to uncomment #define DEBUG in library files
#endif

  initTZsupport();

  initRTC(SDA, SCL, SQUARE_WAVE_PIN );
  if (!isRTCDateTimeSet()) {
    Serial.println("RTC needs date set");
  }

  setupAP();
}


void loop() {
  output.nextByteOut(); // send bytes to Serial
  syncFromRTC(); // keep system time in sync with RTC
  handleWebServer(); //starts web server on first call
}
