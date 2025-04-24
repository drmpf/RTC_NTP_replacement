/**
   TZ_support.cpp
   by Matthew Ford, 
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

// also SEE https://randomnerdtutorials.com/esp32-ntp-timezones-daylight-saving/
/// AND https://randomnerdtutorials.com/esp32-date-time-ntp-client-server-arduino/

// timezone offsets range from  UTC−12:00 to UTC+14:00 in down to 15min segments
// so just use current time to set offset in range -12 to +12 this will be a day off for those tz that are +13 and +14
// +hhmm are SUBTRACTED!! fromo UTC to get local time so take UTC and subtract user's Current Time to get TZ envirmental variable tzoffset rounded to 15mins
// offsets in range -12 < offset <= +12  i.e. -11:45 is the smallest offset and +12:00 is the largest
// e.g. UTC  = 14:00,  LC (localTime) UTC=04:00  tzoffset = +10:00
//      UTC = 08:00  LC = 22:00  tzoffset =  -14 => <=-12 so add 24,  -14+24 = +10
//      UTC = 14:00  LC = 22:00  tzoffset = -8:00
//      UTC = 20:00  LC = 4:00   tzoffset = 16:00 => >12 so subtract 24,  16-24 = -8:00

#include "TZ_support.h"
#include "LittleFSsupport.h"
#include "tzPosix.h"
#include <time.h>                       // time() ctime()
#include <sys/time.h>                   // struct timeval
#include "DebugOut.h"

// normally DEBUG is commented out
//#define DEBUG
static Print* debugPtr = NULL;  // local to this file

//struct timeval {
//  time_t      tv_sec;
//  suseconds_t tv_usec;
//};
// time_t is an intergal type that holds number of seconds elapsed since 00:00 hours, Jan 1, 1970 UTC (i.e., a unix timestamp).
// nullptr is a C++ null pointer literal which you can use anywhere you need to pass a null pointer.

static bool TZ_initialized = false;

static struct timeZoneConfig_struct* loadTimeZoneConfig();
static bool saveTimeZoneConfig(struct timeZoneConfig_struct& timeZoneConfig);


//// define a weak getDefaultTZ method that can be defined elsewhere if you want to set a default TZ -- defined in .ino
const char* get_DefaultTZ() __attribute__((weak));
// if tz not set and no get_DefaultTZ() specified then GMT0 will be used


struct timeZoneConfig_struct {
  time_t utcTime; // sec since 1/1/1970  = if not yet set by SNTP or timezonedb.com
  // POSIX tz str
  char tzStr[50]; // eg AEST-10AEDT,M10.1.0,M4.1.0/3  if empty then skip setting tzStr and just use user set local time and sntp utc to calculate tz offset
};

static struct timeZoneConfig_struct timeZoneConfig;


//struct tm;
//Defined in header <time.h>
//Structure holding a calendar date and time broken down into its components.
//Member objects
//int tm_sec  seconds after the minute – [0, 61] (until C99)[0, 60] (since C99)[for leap second]
//int tm_min minutes after the hour – [0, 59]
//int tm_hour hours since midnight – [0, 23]
//int tm_mday day of the month – [1, 31]
//int tm_mon months since January – [0, 11]
//int tm_year years since 1900
//int tm_wday days since Sunday – [0, 6]
//int tm_yday days since January 1 – [0, 365]
//int tm_isdst Daylight Saving Time flag. The value is positive if DST is in effect, zero if not and negative if no information is available
//
//The Standard mandates only the presence of the aforementioned members in either order.
//The implementations usually add more data-members to this structure.

static bool needToSaveConfigFlag = false;

static const char timeZoneConfigFileName[] = "/timeZoneCfg.bin";  // binary file

void printTimeZoneConfig(struct timeZoneConfig_struct & timeZoneConfig, Print & out) {
  out.print("utcTime:");
  out.println(timeZoneConfig.utcTime);
  out.print("tzStr:");
  out.println(timeZoneConfig.tzStr);
}

bool isTZSet() {
  bool rtn = TZ_initialized;
  return rtn;
}


void setTimezone(String timezone) {
  if (debugPtr) {
    debugPtr->print("  Setting Timezone to "); debugPtr->println(timezone.c_str());
  }
  String longTZ = timezone;
  size_t len = longTZ.length();
  for (int i = 0; i < (60 - len); i++) {
    // 12/17/2023 ADDITION, Code to add spaces to TZ (timezone) environment variable due to a memory leak in the way these variables work. This fixes the issue. Before it was causing a memory leak and crashing the ESP every 5-10 hours or so, depending on how many times TZ was changed.
    longTZ += ' ';
  }
  setenv("TZ", timezone.c_str(), 1); //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
  tzset();
}


void setTime(int yr, int month, int mday, int hr, int minute, int sec, int isDst) {
  struct tm tm;

  tm.tm_year = yr - 1900;   // Set date
  tm.tm_mon = month - 1;
  tm.tm_mday = mday;
  tm.tm_hour = hr;      // Set time
  tm.tm_min = minute;
  tm.tm_sec = sec;
  tm.tm_isdst = isDst;  // 1 or 0
  time_t t = mktime(&tm);
  if (debugPtr) {
    debugPtr->print("Setting time:"); debugPtr->println(asctime(&tm));
  }
  struct timeval now = { .tv_sec = t };
  settimeofday(&now, NULL);
}

void printLocalTime(Print *outPtr) {
  if (!outPtr) {
    return;
  }
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    outPtr->println("Failed to obtain time 1");
    return;
  }
  outPtr->println(&timeinfo, "%A, %B %d %Y %H:%M:%S zone %Z %z ");
}


/** returns true if config saved */
// this may be called from Async webserver thread
// or webpage.cpp
// but not otherwise call so thread safe
bool saveTZconfigIfNeeded() { // saves any TZ config changes
  bool rtn = false;
  if (needToSaveConfigFlag) {
    saveTimeZoneConfig(timeZoneConfig);
    rtn = true;
    needToSaveConfigFlag = false;
  }
  return rtn;
}

void setUTCconfigTime() {
  time_t now = time(nullptr);
  timeZoneConfig.utcTime = now;
};

// call cleanUpfirst
// this may be called from Async webserver thread
// or webpage.cpp
// but not otherwise call so thread safe
void setTZfromPOSIXstr(const char* tz_str) {
  time_t now = time(nullptr);
  timeZoneConfig.utcTime = now;
  strlcpy(timeZoneConfig.tzStr, tz_str, sizeof(timeZoneConfig.tzStr));
  if (debugPtr) {
    debugPtr->print("setTZfromPOSIXstr:"); debugPtr->println(timeZoneConfig.tzStr);
  }
  setTimezone(tz_str);
  needToSaveConfigFlag = true;
}

// used when timeZoneConfigFileName file does not exist or is invalid
void setInitialTimeZoneConfig() {
  timeZoneConfig.utcTime = 0;
  timeZoneConfig.tzStr[0] = '\0'; // => GMT0 after cleanup
  if (get_DefaultTZ) {
    strlcpy(timeZoneConfig.tzStr, get_DefaultTZ(), sizeof(timeZoneConfig.tzStr));
  }
  // clean up
  cleanUpPosixTZStr(timeZoneConfig.tzStr, sizeof(timeZoneConfig.tzStr));
}

void resetDefaultTZstr() {
  char tzStr[sizeof(timeZoneConfig.tzStr)];
  tzStr[0] = '\0';
  if (get_DefaultTZ) {
    strlcpy(tzStr, get_DefaultTZ(), sizeof(tzStr));
  } else {
    strlcpy(tzStr, "GMT0", sizeof(tzStr));    
  }
  setTZfromPOSIXstr(tzStr); // cleans up and set save flag as well
}

String getTZstr() {
  String rtn;
  rtn = String(timeZoneConfig.tzStr);
  rtn.trim();
  if (rtn.length() == 0) {
    rtn = "GMT0";
  }
  return rtn;
}



bool initTZsupport() {
  if (TZ_initialized) {
    return true;
  }
#ifdef DEBUG
  debugPtr = getDebugOut();
#endif
  loadTimeZoneConfig(); // load timeZoneConfig global and cleans up tzStr
  // handle POSIX tz start
  cleanUpPosixTZStr(timeZoneConfig.tzStr, sizeof(timeZoneConfig.tzStr));
  if (debugPtr) {
    debugPtr->println("Setting up time");
  }
  // Now we can set the real timezone
  setTimezone(timeZoneConfig.tzStr);
  TZ_initialized = true;
  return true;
}

// load the last time saved before shutdown/reboot
// returns pointer to timeZoneConfig
static struct timeZoneConfig_struct* loadTimeZoneConfig() {
#ifdef DEBUG
  debugPtr = getDebugOut();
#endif
  setInitialTimeZoneConfig();
  if (!initializeFS()) {
    if (debugPtr) {
      debugPtr->println("FS failed to initialize");
    }
    return &timeZoneConfig; // returns default if cannot open FS
  }
  if (!LittleFS.exists(timeZoneConfigFileName)) {
    if (debugPtr) {
      debugPtr->print(timeZoneConfigFileName); debugPtr->println(" missing.");
    }
    saveTimeZoneConfig(timeZoneConfig);
    return &timeZoneConfig; // returns default if missing
  }
  // else load config
  File f = LittleFS.open(timeZoneConfigFileName, "r");
  if (!f) {
    if (debugPtr) {
      debugPtr->print(timeZoneConfigFileName); debugPtr->print(" did not open for read.");
    }
    LittleFS.remove(timeZoneConfigFileName);
    saveTimeZoneConfig(timeZoneConfig);
    return &timeZoneConfig; // returns default wrong size
  }
  if (f.size() != sizeof(timeZoneConfig)) {
    if (debugPtr) {
      debugPtr->print(timeZoneConfigFileName); debugPtr->print(" wrong size.");
    }
    f.close();
    saveTimeZoneConfig(timeZoneConfig);
    return &timeZoneConfig; // returns default wrong size
  }
  int bytesIn = f.read((uint8_t*)(&timeZoneConfig), sizeof(timeZoneConfig));
  if (bytesIn != sizeof(timeZoneConfig)) {
    if (debugPtr) {
      debugPtr->print(timeZoneConfigFileName); debugPtr->print(" wrong size read in.");
    }
    setInitialTimeZoneConfig(); // again
    f.close();
    saveTimeZoneConfig(timeZoneConfig);
    return &timeZoneConfig;
  }
  f.close();
  // else return settings
  // clean up tz and return
  cleanUpPosixTZStr(timeZoneConfig.tzStr, sizeof(timeZoneConfig.tzStr));
  if (debugPtr) {
    debugPtr->println("Loaded config");
    printTimeZoneConfig(timeZoneConfig, *debugPtr);
  }
  if (debugPtr) {
    String desc = timeZoneConfig.tzStr;
    struct posix_tz_data_struct posixTz;
    posixTZDataFromStr(desc, posixTz);
    buildPOSIXdescription(posixTz, desc);
    debugPtr->println("TZ description");
    debugPtr->println(desc);
  }
  return &timeZoneConfig;
}

// load the last time saved before shutdown/reboot
static bool saveTimeZoneConfig(struct timeZoneConfig_struct & timeZoneConfig) {
  if (!initializeFS()) {
    if (debugPtr) {
      debugPtr->println("FS failed to initialize");
    }
    return false;
  }
  // else save config
  File f = LittleFS.open(timeZoneConfigFileName, "w"); // create/overwrite
  if (!f) {
    if (debugPtr) {
      debugPtr->print(timeZoneConfigFileName); debugPtr->print(" did not open for write.");
    }
    return false; // returns default wrong size
  }
  setUTCconfigTime(); // update utc time
  int bytesOut = f.write((uint8_t*)(&timeZoneConfig), sizeof(struct timeZoneConfig_struct));
  if (bytesOut != sizeof(struct timeZoneConfig_struct)) {
    if (debugPtr) {
      debugPtr->print(timeZoneConfigFileName); debugPtr->print(" write failed.");
    }
    return false;
  }
  // else return settings
  f.close(); // no rturn
  if (debugPtr) {
    debugPtr->print(timeZoneConfigFileName); debugPtr->println(" config saved.");
    printTimeZoneConfig(timeZoneConfig, *debugPtr);
  }
  return true;
}
