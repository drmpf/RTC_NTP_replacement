#ifndef _RTC_WEBPAGES_H
#define _RTC_WEBPAGES_H

#include <Arduino.h>
/*   
   RTC_webpages.h
 * (c)2025 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This generated code may be freely used for both private and commercial use
 * provided this copyright is maintained.
 */

void startWebServer(); // call this from startup()
void handleWebServer();  // call this each loop()

// the processor for webpage variables
// this processor can also used with the ESPAysncWebServer library
String RTC_webVariableProcessor(const String& varName);

void setRTC_DateTime(String &dateArg, String &timeArg);

// returns false for redirect("/settzRTC.html")
// returns true for redirect("/setRTCtime.html");
bool setRTC_TZstr(String &tzArg);

void resetRTC_TZ(); // public for Async server to use

#endif
