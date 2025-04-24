#ifndef _RTC_SUPPORT_H_
#define _RTC_SUPPORT_H_
/*
   RTC_support.h
   RTC based on the DS3231 chip connected via I2C and the Wire library
   modifications by Matthew Ford,  2025/04/23
 * (c)2025 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This generated code may be freely used for both private and commercial use
 * provided this copyright is maintained.
 */

 // Code by JeeLabs http://news.jeelabs.org/code/
// Released to the public domain! Enjoy!
#include <Arduino.h>
#include "time.h"


/** 
 returns false if rtc not found, else returns true
 if lost power then default time of 2020/10/10 10:10:10 UTC
 is set, and isRTCDateTimeSet() will return false
 until a valid date/time is set.
 Also enables 1Hz output and attaches interrupt to squareWavePin
*/
bool initRTC(int SDA_pin, int SCL_pin, int squareWavePin);

/**
 * Keeps system time in sync with RTC
 * should be called at least once each loop()
 */
void syncFromRTC();

/** return true if RTC started and no power loss 
   OR started and dateTime has been set to something
   (not just the default time)
*/
bool isRTCDateTimeSet();


#endif
