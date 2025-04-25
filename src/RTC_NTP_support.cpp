// RTC_NTP_support.cpp
#include <Arduino.h>
#include "RTC_NTP_support.h"
#include <time.h>
#include <sys/time.h>
#include "esp_sntp.h"
#include "millisDelay.h"
#include "RTC_support.h"
/**
 * (c)2025 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This generated code may be freely used for both private and commercial use
 * provided this copyright is maintained.
*/

#define DEBUG

extern Print* getDebugOut();
static Print* debugPtr = NULL;

extern void setRTC(time_t sec); // only used in this file

// this is called from the sntp client to set the time
// can use it to update the RTC module as well
// the RTC module has an error of ~0.3sec / day
// but ESP32 drift is greater so sync to RTC once a minute
// NOTE:  when this is called the tv is ALREADY in UNIX epoch NOT ntp epoch
static volatile int64_t sntp_time_sec_v;
static volatile unsigned long rtcSetDelay_ms_v = 0;

void sntp_sync_time(struct timeval *tvPtr) {
  if (debugPtr) {
    debugPtr->print(" NTP called sntp_sync_time: ");
    debugPtr->print(tvPtr->tv_sec); debugPtr->print(" sec ");
    debugPtr->print(tvPtr->tv_usec); debugPtr->print(" us ");
    debugPtr->println();
  }

  sntp_time_sec_v = tvPtr->tv_sec + 1;
  long delay_ms = ((long)1000000l - tvPtr->tv_usec) / 1000;
  if (delay_ms < 100) {
    delay_ms += 1000;
    sntp_time_sec_v = sntp_time_sec_v+1;
  }
  rtcSetDelay_ms_v = delay_ms;
}

static millisDelay rtcSetDelay;

void handleNTP() {
  if (rtcSetDelay_ms_v > 0) {
    rtcSetDelay.start(rtcSetDelay_ms_v);
    rtcSetDelay_ms_v = 0; // handled
  }
  if (rtcSetDelay.justFinished()) {
    time_t sntp_time_sec = sntp_time_sec_v;
    if (debugPtr) {
      debugPtr->print(" called handleSetRTCtime: ");
      debugPtr->print(sntp_time_sec); debugPtr->print(" sec ");
      debugPtr->println();
    }
    setRTC(sntp_time_sec);
  }
}

bool initNTP() {
#ifdef DEBUG
  debugPtr = getDebugOut();
#endif
  if (debugPtr) {
    debugPtr->println("Setting up NTP time");
  }
  // just for testing get NTP every 1 min (60000ms)
  sntp_set_sync_interval(60ul*60000); // once per hr
  configTime(0, 0, "pool.ntp.org");
  return true;
}
