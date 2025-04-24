#ifndef _TZ_SUPPORT_H
#define _TZ_SUPPORT_H
/*   
   TZ_support.h
 * (c)2025 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This generated code may be freely used for both private and commercial use
 * provided this copyright is maintained.
 */
 
/* See 
  https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv 
  for a list of time zones and their posix_tz strings
  that can be copied and pasted into the get_DefaultTZ or into the Set Time Zone webpage.
*/

#include <Arduino.h>
bool initTZsupport();
String getTZstr(); // get the current tz string
void resetDefaultTZstr(); // reset tz to default one
//void showTimeDebug();
void printTimeZoneConfig(struct timeZoneConfig_struct & timeZoneConfig, Print & out) ;
void setTZfromPOSIXstr(const char* tz_str); // sets flag to save config
bool saveTZconfigIfNeeded(); // saves any TZ config changes returns true if save happened
#endif
