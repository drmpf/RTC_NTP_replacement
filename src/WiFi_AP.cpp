#include <WiFi.h>
#include "WiFi_AP.h"
#include "DebugOut.h"
/**
 * (c)2025 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This generated code may be freely used for both private and commercial use
 * provided this copyright is maintained.
*/

#define DEBUG

// =================== ESP Access Point settings ===================
// this is the ip of the WiFi setup Access Point
static  IPAddress accessPoint_ip = IPAddress(192, 168, 1, 1);
// other choices
//static  IPAddress accessPoint_ip = IPAddress(10, 1, 1, 1);
//static  IPAddress accessPoint_ip = IPAddress(172, 16, 1, 1);

static char wifiWebConfigAP[] = "RTCwithTZ";
static char wifiWebConfigPASSWORD[] = "12345678";

static Print* debugPtr = NULL;


/**
   sets up AP
*/
void setupAP() {
#ifdef DEBUG
  debugPtr = getDebugOut();
#endif

  WiFi.softAPConfig(accessPoint_ip, accessPoint_ip, IPAddress(255, 255, 255, 0)); // call this first!!
  WiFi.softAP(wifiWebConfigAP, wifiWebConfigPASSWORD);
  if (debugPtr) {
    debugPtr->print("Access Point: ");
    debugPtr->print(wifiWebConfigAP);
    debugPtr->print(" pw: ");
    debugPtr->print(wifiWebConfigPASSWORD);
    debugPtr->print(" setup with ip: ");
    debugPtr->print(accessPoint_ip);
    debugPtr->println();
  }
}
