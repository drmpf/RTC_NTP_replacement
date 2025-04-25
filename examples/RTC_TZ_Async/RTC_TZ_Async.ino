/**
 RTC_TZ_Async.ino
   Compiled on ESP32 with V3.0.7
 * (c)2025 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This generated code may be freely used for both private and commercial use
 * provided this copyright is maintained.
*/
#include "RTC_NTP_replacement.h"
#include "BufferedOutput.h"
#include "ESPAsyncWebServer.h"


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
  
  initTZsupport(); // starts FS by calling initializeFS()

  initRTC(SDA, SCL, SQUARE_WAVE_PIN );
  if (!isRTCDateTimeSet()) {
    Serial.println("RTC needs date set");
  }

  setupAP();
  startAsyncWebServer(); 
}


void loop() {
  output.nextByteOut(); // send bytes to Serial
  syncFromRTC(); // keep system time in sync with RTC
}

// ================= AsyncWebServer ==============
static AsyncWebServer server(80);
static void handleSetRTCDateTime(AsyncWebServerRequest * request);
static void handleSetTZstr(AsyncWebServerRequest * request);
static String correctedTZstr;  // empty if no problems
static String correctedTZstrDsc;
static String userInputTZstr;

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

// link in RTC_webVariableProcessor to handle RTC pages
String processor(String var) {
  String rtn = "";
  rtn = RTC_webVariableProcessor(var);
  if (rtn.length() == 0) {
    // call other page processors here
  }
  return rtn;
}

void startAsyncWebServer() {
  if (!initializeFS()) {
    if (debugPtr) {
      debugPtr->println("LittleFS failed to start");
    }
    return;
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(LittleFS, "/index.html", String(), false, processor);
  });
  server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(LittleFS, "/index.html", String(), false, processor);
  });
  server.on("/setRTCtime.html", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(LittleFS, "/setRTCtime.html", String(), false, processor);
  });
  server.on("/settzRTC.html", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(LittleFS, "/settzRTC.html", String(), false, processor);
  });
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(LittleFS, "/style.css", "text/css");
  });
  server.on("/setRTCDateTime", HTTP_POST, [](AsyncWebServerRequest * request) {
    handleSetRTCDateTime(request);
  });
  server.on("/setTZstr", HTTP_GET, [](AsyncWebServerRequest * request) {
    handleSetTZstr(request);
  });
  server.on("/resetTZ", HTTP_GET, [](AsyncWebServerRequest * request) {
    resetRTC_TZ();
    request->redirect("/setRTCtime.html");
  });
  server.onNotFound(notFound);
  if (debugPtr) {
    debugPtr->println("Starting webserver");
  }
  server.begin();
}

void handleSetRTCDateTime(AsyncWebServerRequest * request) {
  const AsyncWebParameter *dateParam = request->getParam("DATE", true); // GET_POST
  const AsyncWebParameter *timeParam = request->getParam("TIME", true); // GET_POST
  if ((!dateParam) || (!timeParam)) {
    if (debugPtr) {
      debugPtr->print(" Error missing DATE or TIME arg");
      debugPtr->println();
    }
    return;
  }
  String dateArg = dateParam->value();
  String timeArg = timeParam->value();
  setRTC_DateTime(dateArg, timeArg);
  request->redirect("/index.html");
}

void handleSetTZstr(AsyncWebServerRequest * request) {
  const AsyncWebParameter *tzParam = request->getParam("TZ_INPUT_STR", false);
  if (!tzParam) {
    if (debugPtr) {
      debugPtr->print(" Error missing TZ_INPUT_STR");
    }
  }
  String tzArg = tzParam->value();
  // returns false for redirect("/settzRTC.html")
  // returns true for redirect("/setRTCtime.html");
  if (setRTC_TZstr(tzArg)) {
    request->redirect("/setRTCtime.html");
  } else {
    request->redirect("/settzRTC.html");
  }
}

// ================= END AsyncWebServer method==============
