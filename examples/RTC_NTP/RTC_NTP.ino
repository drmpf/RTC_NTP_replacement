/**
  RTC_NTP.ino
  Compiled on ESP32 with V3.0.7
 * (c)2025 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This generated code may be freely used for both private and commercial use
 * provided this copyright is maintained.
*/
#include "RTC_NTP_replacement.h"
#include "BufferedOutput.h"
#include "WiFi.h"


const int SQUARE_WAVE_PIN = 15; // the pin the RTC 1Hz output is connected to

//// WiFi credentials - replace with your own
const char* ssid = "SSID";
const char* password = "password";
IPAddress staticIP(10, 1, 1, 253); // static Ip

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


// ===================== DNS settings ==============================
// when WiFi config uses a static IP need need to manually set the dns servers
// these are the GOOGLE public DNS servers
static IPAddress dns1(8, 8, 8, 8);
static IPAddress dns2(8, 8, 8, 4);


bool initializedWiFi = false;
bool WiFi_connected = false;

bool connectToWiFi() {
  if (!initializedWiFi) {
    initializedWiFi = true;
    // Connect to WiFi
    IPAddress ip = staticIP;
    IPAddress gateway(ip[0], ip[1], ip[2], 1); // set gatway to ... 1
    IPAddress subnet_ip = IPAddress(255, 255, 255, 0);
    if (debugPtr) {
      debugPtr->print("Connecting to WiFi with static IP:"); debugPtr->print(ip); debugPtr->println();
    }
    if (!WiFi.config(ip, gateway, subnet_ip, dns1, dns2)) {
      if (debugPtr) {
        debugPtr->print("  WiFi.config failed");  debugPtr->println();
      }
      return false;
    }
    WiFi.begin(ssid, password);
  }
  if (WiFi.status() == WL_CONNECTED) {
    if (!WiFi_connected) {
      WiFi_connected = true;
      if (debugPtr) {
        debugPtr->println(" WiFi CONNECTED");
      }
    }
  } else {
    WiFi_connected = false;
  }
  return true;
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

  initNTP(); // NOTE!!! call this BEFORE calling initTZsupport() and initRTC()
  // because configTime( ) has 0 for the hr and min offset

  initTZsupport(); // starts FS by calling initializeFS()

  initRTC(SDA, SCL, SQUARE_WAVE_PIN );
  if (!isRTCDateTimeSet()) {
    Serial.println("RTC needs date set");
  }

}


void loop() {
  connectToWiFi(); // try to connect
  output.nextByteOut(); // send bytes to Serial
  syncFromRTC(); // keep system time in sync with RTC
  handleWebServer(); //starts web server on first call
  handleNTP();

}
