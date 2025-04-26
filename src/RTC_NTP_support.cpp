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
static volatile unsigned long ntp_ms_v = 0;

void sntp_sync_time(struct timeval *tvPtr) {
  if (debugPtr) {
    debugPtr->print(" NTP called sntp_sync_time: ");
    debugPtr->print(tvPtr->tv_sec); debugPtr->print(" sec ");
    debugPtr->print(tvPtr->tv_usec); debugPtr->print(" us ");
    debugPtr->println();
  }

  sntp_time_sec_v = tvPtr->tv_sec + 1;
  long delay_ms = (((long)1000000) - tvPtr->tv_usec) / 1000;
  if (delay_ms < 50) {
    delay_ms += 1000;
    sntp_time_sec_v = sntp_time_sec_v+1;
  }
  ntp_ms_v = millis();
  rtcSetDelay_ms_v = delay_ms;
}

static millisDelay rtcSetDelay;

void handleNTP() {
  if (rtcSetDelay_ms_v > 0) {
    // adjust for delay in calling this
    // usually delay due to startup() processing
    unsigned long elapsed_ms = millis() - ntp_ms_v;
    if (debugPtr) {
        debugPtr->print(" elapsed_ms: ");debugPtr->print(elapsed_ms);
        debugPtr->print(" rtcSetDelay_ms_v: ");debugPtr->print(rtcSetDelay_ms_v);
        debugPtr->print(" sntp_time_sec_v: ");debugPtr->println(sntp_time_sec_v);
    }
    if (elapsed_ms > rtcSetDelay_ms_v) {
       // adjust delay and sntp_time_sec_v
       unsigned long dly_s = elapsed_ms/1000 + 1;
       unsigned long dly_ms = ((dly_s*1000)-elapsed_ms) + rtcSetDelay_ms_v;
    if (debugPtr) {
        debugPtr->print(" dly_s: ");debugPtr->print(dly_s);
        debugPtr->print(" dly_ms: ");debugPtr->print(dly_ms);
        debugPtr->print(" sntp_time_sec_v: ");debugPtr->println(sntp_time_sec_v);
    }
       sntp_time_sec_v = sntp_time_sec_v + (dly_ms/1000) + dly_s; // extra secs
       rtcSetDelay_ms_v = dly_ms % 1000;
    } else {
      rtcSetDelay_ms_v = rtcSetDelay_ms_v - elapsed_ms;
    }
    if (debugPtr) {
        debugPtr->print(" adjusted ");
        debugPtr->print(" rtcSetDelay_ms_v: ");debugPtr->print(rtcSetDelay_ms_v);
        debugPtr->print(" sntp_time_sec_v: ");debugPtr->println(sntp_time_sec_v);
    }
    rtcSetDelay.start(rtcSetDelay_ms_v);
    rtcSetDelay_ms_v = 0; // handled
  }
  if (rtcSetDelay.justFinished()) {
    time_t sntp_time_sec = sntp_time_sec_v;
    if (debugPtr) {
      debugPtr->print(" handleNTP calling setRTC: ");
      debugPtr->print(sntp_time_sec); debugPtr->print(" sec ");
      debugPtr->println();
    }
    if (!isRTCDateTimeSet()) {
      if (debugPtr) {
        debugPtr->print(" RTC not set. Do hard sync now.");
        debugPtr->println();
      }
      struct timeval tv;
      tv.tv_sec = sntp_time_sec;
      tv.tv_usec = 0;
      settimeofday(&tv, NULL); // set now if RTC not set
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
  sntp_set_sync_interval(3600000); // 60ul*60000 once per hr
  configTime(0, 0, "pool.ntp.org");
  return true;
}
