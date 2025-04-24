#ifndef _RTC_NTP_SUPPORT_H
#define _RTC_NTP_SUPPORT_H

/*
 * (c)2025 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This generated code may be freely used for both private and commercial use
 * provided this copyright is maintained.
 */

bool initNTP(); // called from setup

void handleNTP(); // called every loop()

#endif
