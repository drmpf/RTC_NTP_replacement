/*
   RTC_support.cpp
   for DS3231 RTC  running on GMT / UTC time
   modifications by Matthew Ford,
 * (c)2025 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This generated code may be freely used for both private and commercial use
 * provided this copyright is maintained.
*/
// Code by JeeLabs http://news.jeelabs.org/code/
// Released to the public domain! Enjoy!

#include <Wire.h>
#include "RTC_support.h"
#include <Arduino.h>
#include "PrintTimes.h"
#include "DebugOut.h"

#define DEBUG

static Print* debugPtr = NULL;

static signed long MAX_DIFF_ms = 5; // if RTC and system time differ by less then this do not sync
static const long MAX_TIME_DIFF_ms = 5l * 60 * 1000; // max 5min diff between rtc and system time
// if diff > 5 min then just force system time to == rtc time
// if diff < 5 min then update system time 'smoothly"

class RTC_DS3231 {
  public:
    RTC_DS3231();
    bool begin(int sda, int scl, uint32_t frequency = 0);
    bool isDateTimeSet();
    void adjust(const struct tm * tmPtr);
    bool lostPower(void);
    bool now(struct tm * tmPtr);
    void enable_1Hz();
    /** true if the dateTime is set */
    bool dateTimeSet;
    /** true if RTC i2c has been initialized and the RTC found */
    bool hardwareInitialized;
};

/** the DS3231 instance */
// private to this file
static RTC_DS3231 rtc;

static volatile bool interruptTriggered_v = false;
static volatile unsigned long interruptMicros_v = 0;


static void IRAM_ATTR squareWaveInterrupt() {
  // Capture internal timer count instead of full timestamp
  if (interruptTriggered_v) {
    return; // still waiting to process last interrupt
  }
  interruptMicros_v = micros();
  interruptTriggered_v = true;
}

static void attach_1Hz_interrupt(int squareWavePin) {
  // 1Hz goes High 500ms after setting the seconds
  // Attach interrupt to pin 15, trigger on RISING edge (High to Low transition)
  attachInterrupt(digitalPinToInterrupt(squareWavePin), squareWaveInterrupt, FALLING);
  if (debugPtr) {
    debugPtr->print(" Monitoring 1Hz square wave on pin "); debugPtr->println(squareWavePin);
  }
}

// sets ESP32 unix time stamp from RTC running in GMT / UTC time
// called when user set time
static bool setTimeFromRTC();

/** returns false, if lost power
    then default time of 2020/10/10 10:10:10 UTC is set
*/
static bool setTimeFromRTC_Startup();

static bool RTC_isSet = false;
static time_t tm_to_unix_time(const struct tm *tm_in);
static void init_tm(struct tm* tmPtr, uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec);
static uint16_t date2days(uint16_t y, uint8_t m, uint8_t d);
static uint8_t dayOfTheWeek(uint16_t year, uint8_t month, uint8_t day);

/**
   Gets the current RTC time as Unix time
   Reads from RTC and converts struct tm to Unix time

   @return Unix timestamp from RTC
   used by webpages.cpp
*/
time_t get_rtc_unix_time() {
  time_t t = 0;
  struct tm tm_now;
  if (!rtc.now(&tm_now)) {
    return t;
  }
  t = tm_to_unix_time(&tm_now);
  return t;
}

// sets ESP32 unix time stamp from RTC running in GMT / UTC time
// called when user sets time
static bool setTimeFromRTC() {
  if (debugPtr) {
    debugPtr->print(" setTimeFromRTC() "); 
  }
  struct tm tm_now;
  if (!rtc.now(&tm_now)) {
    return false;
  }
  time_t t = tm_to_unix_time(&tm_now);
  if (debugPtr) {
    debugPtr->print(" unix time: "); debugPtr->print(t);
    debugPtr->print("  "); 
    print_tm(&tm_now, debugPtr);
  }
  struct timeval tv;
  tv.tv_sec = t;
  tv.tv_usec = 0;
  settimeofday(&tv, NULL); // will be adjusted later
  return true;
}

// return true if RTC started and no power loss
bool isRTCDateTimeSet() {
  return RTC_isSet;
}

void setRTC(time_t sec) {
  if (debugPtr) {
    debugPtr->print(" setRTC: ");
    debugPtr->println(sec);
  }
  struct tm rtc_tm;
  // Use gmtime_r which is thread-safe and converts to UTC time
  // ignoring the local time zone setting
  gmtime_r(&sec, &rtc_tm);
  rtc.adjust(&rtc_tm);
  RTC_isSet = true;
}


/**
  initialize the RTC date and time.
  if the RTC module has lost power (date/time not set)<br>
  sets a default time of 2010/10/10 10:10:10<br>
  otherwise leave as is.
*/
bool initRTC(int SDA_pin, int SCL_pin, int squareWavePin) {
#ifdef DEBUG
  debugPtr = getDebugOut();
#endif

  if (rtc.hardwareInitialized) {
    return true;
  }
  delay(2000); // give DS3231 time to startup
  //When  the  microcontroller  resets,  the  DS3231  I2C  interface  may  be
  //placed  into  a  known  state  by  tog-gling SCL until SDA is observed to be at a high level.
  //At that  point  the  microcontroller  should  pull  SDA  low  while  SCL is high, generating a START condition

  if (!rtc.begin(SDA_pin, SCL_pin)) { // returns false if bus cannot be cleared by ESP32 i2cInit code
    if (debugPtr) {
      debugPtr->println("Couldn't find RTC");
    }
    return false;
  }
  rtc.hardwareInitialized = true;
  setTimeFromRTC_Startup();
  // Configure input pin to read the square wave
  pinMode(squareWavePin, INPUT_PULLUP);
  rtc.enable_1Hz();
  if (debugPtr) {
    debugPtr->println("RTC 1Hz square wave enabled");
  }
  attach_1Hz_interrupt(squareWavePin);
  return true;
}


void syncFromRTC() {
  if (!interruptTriggered_v) {
    return; // nothing to do
  }
  static bool ahead = false; // if true then stop the system clock
  static time_t currentSec = 0; // for stopping system clock

  unsigned long interruptMicros = interruptMicros_v;
  interruptTriggered_v = false; // have picked up interruptMicros_v

  struct timeval tv;
  gettimeofday(&tv, NULL);
  unsigned long currentMicros = micros();
  time_t rtc_time = get_rtc_unix_time();

  // Calculate time offset from interrupt timestamp
  unsigned long elapsedMicros = currentMicros - interruptMicros;

  // Adjust current time by elapsed time since interrupt
  time_t secs = tv.tv_sec;
  time_t us = tv.tv_usec;

  unsigned long interruptDelay_us = elapsedMicros % 1000000; // us ignoring seconds

  // skip adjusting the seconds as both are adjusted by the same amount
  //  unsigned long interruptDelay_s = elapsedMicros / 1000000; // should normally be 0
  //  time_t adjusted_rtc_time = rtc_time - interruptDelay_s;
  //  secs -= interruptDelay_s;

  long adjusted_us = (((long)us) - ((long)interruptDelay_us)); // signed
  while (adjusted_us < 0) {
    adjusted_us += 1000000;
    secs -= 1;
  }
  us = adjusted_us;

  if (debugPtr) {
    debugPtr->print("1Hz processing RTC: ");  debugPtr->print(rtc_time);
    debugPtr->print(" system: "); debugPtr->print(secs); debugPtr->print(" sec ");
    debugPtr->print(adjusted_us / 1000.0); debugPtr->print(" ms  ");
  }

  // find difference in ms
  int64_t timeDiff_s = ((int64_t)secs - (int64_t)rtc_time);
  long timeDiff_ms = us / 1000;

  long ms_Diff = timeDiff_s * 1000 + timeDiff_ms;
  if (debugPtr) {
    debugPtr->print(" ms_Diff: ");  debugPtr->print(ms_Diff); debugPtr->print(" ms");  debugPtr->println();
  }

  if ((ms_Diff < -MAX_TIME_DIFF_ms) || (ms_Diff > MAX_TIME_DIFF_ms)) {
    // more than 5min out of sync just hard sync now
    if (debugPtr) {
      debugPtr->print("Out of Sync by more than "); debugPtr->print(MAX_TIME_DIFF_ms / 1000);
      debugPtr->println(" sec.  Hard Sync now");
    }
    struct timeval sync_tv;
    sync_tv.tv_sec = rtc_time; // now sec
    sync_tv.tv_usec = interruptDelay_us; // adjusted for seconds
    settimeofday(&sync_tv, NULL); // within 1 to 2 sec just set now adjust for interrupt delay
    return;
  }

  if ((ms_Diff < MAX_DIFF_ms) && (ms_Diff > -MAX_DIFF_ms)) {
    ahead = false;
    if (debugPtr) {
      debugPtr->print("Within ");  debugPtr->print(MAX_DIFF_ms); debugPtr->print(" ms, do no sync ");
      debugPtr->println();
    }
    return;
  }

  //else
  if (ms_Diff < 0) {
    ahead = false;
    if (ms_Diff > -2000) {
      if (debugPtr) {
        debugPtr->println("Within -2 sec");
      }
      struct timeval sync_tv;
      sync_tv.tv_sec = rtc_time; // now sec
      sync_tv.tv_usec = interruptDelay_us; // adjusted for seconds
      settimeofday(&sync_tv, NULL); // within 1 to 2 sec just set now adjust for interrupt delay
    } else { // > 2 sec offset // push forward 2sec at second so 10min in 5mins
      if (debugPtr) {
        debugPtr->println("More than -2 sec");
      }
      struct timeval sync_tv;
      sync_tv.tv_sec = tv.tv_sec + 2; // push it 2 sec
      sync_tv.tv_usec = interruptDelay_us; // adjusted for seconds
      settimeofday(&sync_tv, NULL); // within 1 to 2 sec just set now
    }
    return;
  }

  // else ahead
  if (!ahead) { // first ahead sync
    ahead = true;
    currentSec = secs;
  }
  // timeofday is ahead, need to stop it
  if (debugPtr) {
    debugPtr->print(" Head by "); debugPtr->print(ms_Diff); debugPtr->println(" ms");
  }
  struct timeval sync_tv;
  sync_tv.tv_sec = currentSec;
  sync_tv.tv_usec = interruptDelay_us; // adjusted for seconds
  settimeofday(&sync_tv, NULL);
}


static bool setTimeFromRTC_Startup() {
  if (rtc.lostPower()) {
    if (debugPtr) {
      debugPtr->println("RTC lost power, set the time to default value!");
    }
    struct tm dateTime;
    init_tm(&dateTime, 2020, 10, 10, 10, 10, 10);
    rtc.adjust(&dateTime); // set to default from above
    rtc.dateTimeSet = true;
    setTimeFromRTC();
    return false;
  }
  // else update current time
  rtc.dateTimeSet = true;
  RTC_isSet = true;
  setTimeFromRTC();
  return true; // OK
}

/**
   Initializes a struct tm with the given date and time values

   @param tmPtr Pointer to the struct tm to initialize
   @param year Full year (e.g., 2025)
   @param month Month (1-12)
   @param day Day of month (1-31)
   @param hour Hour (0-23)
   @param min Minute (0-59)
   @param sec Second (0-59)
*/
static void init_tm(struct tm* tmPtr, uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec) {
  if (debugPtr) {
    debugPtr->print(" init_tm()"); debugPtr->println();
  }
  if (!tmPtr) return;

  // Initialize the struct tm to all zeros
  memset(tmPtr, 0, sizeof(struct tm));

  // Set the fields with adjusted values
  tmPtr->tm_year = year - 1900;  // Years since 1900
  tmPtr->tm_mon = month - 1;     // Months since January (0-11)
  tmPtr->tm_mday = day;          // Day of the month (1-31)
  tmPtr->tm_hour = hour;         // Hours (0-23)
  tmPtr->tm_min = min;           // Minutes (0-59)
  tmPtr->tm_sec = sec;           // Seconds (0-59)

  tmPtr->tm_wday = dayOfTheWeek(year, month, day); //(day + 13 * (m + 1) / 5 + k + k / 4 + j / 4 + 5 * j) % 7;

  // Calculate day of year (0-365, 0 = Jan 1)
  tmPtr->tm_yday = date2days(year, month, day);

  // tm_isdst is set to -1 to indicate that daylight saving time information is not available
  tmPtr->tm_isdst = -1;
  print_tm(tmPtr, debugPtr);
}

/**
   Converts a struct tm (in GMT/UTC) to Unix timestamp

   @param tm_in Pointer to the struct tm to convert
   @return Unix timestamp in seconds since the epoch (1970-01-01 00:00:00 UTC)
*/
time_t tm_to_unix_time(const struct tm *tm_in) {
  if (!tm_in) {
    return 0;
  }

  // Create a copy of the input tm struct to avoid modifying the original
  struct tm tm_copy;
  memcpy(&tm_copy, tm_in, sizeof(struct tm));

  // The following calculates seconds since Jan 1, 1970, 00:00:00 GMT

  // First, calculate days before current month
  static const int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  int month = tm_copy.tm_mon;
  int year = tm_copy.tm_year + 1900;

  // Days since 1970
  int days_since_epoch = 0;

  // Add days from years
  for (int y = 1970; y < year; y++) {
    days_since_epoch += 365;
    // Add leap day for leap years
    if ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0)) {
      days_since_epoch += 1;
    }
  }

  // Add days from months in current year
  for (int m = 0; m < month; m++) {
    days_since_epoch += days_in_month[m];
    // Add leap day in February if current year is leap year
    if (m == 1 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))) {
      days_since_epoch += 1;
    }
  }

  // Add days from current month
  days_since_epoch += tm_copy.tm_mday - 1; // -1 because days are 1-based in tm

  // Convert days to seconds and add time components
  time_t seconds = days_since_epoch * 86400; // 86400 seconds in a day
  seconds += tm_copy.tm_hour * 3600;         // 3600 seconds in an hour
  seconds += tm_copy.tm_min * 60;            // 60 seconds in a minute
  seconds += tm_copy.tm_sec;                 // seconds

  return seconds;
}


/** read the register from the i2c address */
static uint8_t read_i2c_register(uint8_t addr, uint8_t reg) {
  Wire.beginTransmission(addr);
  Wire.write((byte)reg);
  Wire.endTransmission();

  Wire.requestFrom(addr, (byte)1);
  return Wire.read();
}

/** write the value to the register in the i2c address */
static void write_i2c_register(uint8_t addr, uint8_t reg, uint8_t val) {
  Wire.beginTransmission(addr);
  Wire.write((byte)reg);
  Wire.write((byte)val);
  Wire.endTransmission();
}
/** convert bcd number to binary */
static uint8_t bcd2bin (uint8_t val) {
  return val - 6 * (val >> 4);
}
/** convert binary to bcd number */
static uint8_t bin2bcd (uint8_t val) {
  return val + 6 * (val / 10);
}

////////////////////////////////////////////////////////////////////////////////
// RTC_DS3231 implementation

// for DS3231 RTC
#define DS3231_ADDRESS  0x68
#define DS3231_CONTROL  0x0E
#define DS3231_STATUSREG 0x0F


/** RTC_DS3231 class constructor */
RTC_DS3231::RTC_DS3231(): dateTimeSet(false), hardwareInitialized(false) {
}

/** start i2c for the RTC */
bool RTC_DS3231::begin(int sda, int scl, uint32_t frequency) {
  return Wire.begin(sda, scl, frequency);
}

/** returns true if the RTC has its time/date set */
bool RTC_DS3231::isDateTimeSet() {
  return dateTimeSet;
}

/** returns true if the RTC has lost power, battery flat and powered off */
bool RTC_DS3231::lostPower(void) {
  if (!hardwareInitialized) {
    return true;
  } // else
  return (read_i2c_register(DS3231_ADDRESS, DS3231_STATUSREG) >> 7);
}

//struct tm {
//  int tm_sec; //seconds after the minute 0-60
//  int tm_min; // minutes after the hour 0-59
//  int tm_hour; // hours since midnight 0-23
//  int tm_mday; // day of the month 1-31
//  int tm_mon; // months since January 0-11
//  int tm_year; // years since 1900
//  int tm_wday; // days since Sunday 0-6
//  int tm_yday; // days since January 1 0-365
//  int tm_isdst; //Daylight Saving Time flag, always -1, on information
//};


/** sets the RTC date and time from struct tm* */
void RTC_DS3231::adjust(const struct tm * tmPtr) {
  if ((!hardwareInitialized) || (tmPtr == NULL)) {
    if (debugPtr) {
      debugPtr->println("RTC_DS3231::adjust");
      debugPtr->println("!hardwareInitialized or NULL tmPtr");
    }
    return;
  } // else
  Wire.beginTransmission(DS3231_ADDRESS);  // takes lock
  Wire.write((byte)0); // start at location 0
  Wire.write(bin2bcd(tmPtr->tm_sec));
  Wire.write(bin2bcd(tmPtr->tm_min));
  Wire.write(bin2bcd(tmPtr->tm_hour));
  Wire.write(bin2bcd(0));
  Wire.write(bin2bcd(tmPtr->tm_mday));
  Wire.write(bin2bcd(tmPtr->tm_mon + 1));
  Wire.write(bin2bcd(tmPtr->tm_year + 1900 - 2000));
  Wire.endTransmission();

  uint8_t statreg = read_i2c_register(DS3231_ADDRESS, DS3231_STATUSREG);
  statreg &= ~0x80; // flip OSF bit
  write_i2c_register(DS3231_ADDRESS, DS3231_STATUSREG, statreg);
  dateTimeSet = true;
}


// Function to set up the DS3231 RTC 1Hz square wave output
// The control register (0x0E) bits:
// bit 7: EOSC - Enable oscillator (0 = enabled)
// bit 6: BBSQW - Battery-backed square wave enable (1 = enabled)
// bit 5: CONV - Convert temperature (0 = no force)
// bit 4: RS2 - Rate select bit 2
// bit 3: RS1 - Rate select bit 1
// bit 2: INTCN - Interrupt control (0 = square wave enabled)
// bit 1: A2IE - Alarm 2 interrupt enable (0 = disabled)
// bit 0: A1IE - Alarm 1 interrupt enable (0 = disabled)

// For 1Hz: RS2=0, RS1=0, INTCN=0
// Enable battery-backed square wave: BBSQW=1
// Final value: 01000000 = 0x40
// DS3231_SquareWave1Hz = 0x00,  /**<  1Hz square wave */
// #define DS3231_CONTROL 0x0E   ///< Control register

void RTC_DS3231::enable_1Hz() {
  //  DS3231_CONTROL 0x0E   ///< Control register
  //  uint8_t ctrl = read_register(DS3231_CONTROL);
  //  ctrl &= ~0x04; // turn off INTCON
  //  ctrl &= ~0x18; // set freq bits to 0
  //  write_register(DS3231_CONTROL, ctrl | mode);

  // first read in current ctrl value
  Wire.beginTransmission(DS3231_ADDRESS);
  Wire.write((byte)DS3231_CONTROL);
  Wire.endTransmission();
  uint8_t ctrl = Wire.read();
  // now update with 1Hz settings
  ctrl &= ~0x04; // turn off INTCON
  ctrl &= ~0x18; // set freq bits to 0

  // write out new ctrl value
  Wire.beginTransmission(DS3231_ADDRESS); // DS3231 I2C address
  Wire.write(DS3231_CONTROL);             // Control register
  Wire.write(ctrl);             // Set for 1Hz output freq bits all 0
  Wire.endTransmission();
}

/** get the current RTC date/time into the struct tm* arg */
bool RTC_DS3231::now(struct tm * tmPtr) {
  if ((!hardwareInitialized)  || (!dateTimeSet)) {
    init_tm(tmPtr, 2010, 10, 10, 10, 10, 10); // use default
    return false;
  } // else

  Wire.beginTransmission(DS3231_ADDRESS);
  Wire.write((byte)0);
  Wire.endTransmission();

  Wire.requestFrom(DS3231_ADDRESS, 7);
  tmPtr->tm_sec = bcd2bin(Wire.read() & 0x7F);
  tmPtr->tm_min = bcd2bin(Wire.read());
  tmPtr->tm_hour = bcd2bin(Wire.read());
  Wire.read();
  tmPtr->tm_mday = bcd2bin(Wire.read());
  tmPtr->tm_mon = bcd2bin(Wire.read()) - 1;
  tmPtr->tm_year = bcd2bin(Wire.read()) + 2000 - 1900;
  tmPtr->tm_wday = dayOfTheWeek(tmPtr->tm_year + 1900, tmPtr->tm_mon + 1, tmPtr->tm_mday);
  tmPtr->tm_yday = (date2days(tmPtr->tm_year + 1900, 1, 1) - date2days(tmPtr->tm_year + 1900, tmPtr->tm_mon + 1, tmPtr->tm_mday));
  tmPtr->tm_isdst = -1; // no data available
  return true;
}

/**
    @brief  Given a date, return number of days since 2000/01/01, valid for 2001..2099
    @param y Year
    @param m Month
    @param d Day
    @return Number of days
*/
static uint16_t date2days(uint16_t y, uint8_t m, uint8_t d) {
  /** Number of days in each month */
  static uint8_t daysInMonth [] PROGMEM = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  if (y >= 2000) {
    y -= 2000;
  }
  uint16_t days = d;
  for (uint8_t i = 1; i < m; ++i) {
    days += pgm_read_byte(daysInMonth + i - 1);
  }
  if (m > 2 && y % 4 == 0) {
    ++days;
  }
  return days + 365 * y + (y + 3) / 4 - 1;
}

/**
    returns the day of the week (Sunday == 0) for a given year,month,day
*/
static uint8_t dayOfTheWeek(uint16_t year, uint8_t month, uint8_t day) {
  // Calculate day of week (0-6, Sunday = 0)
  // Using Zeller's Congruence algorithm
  int y = (month < 3) ? (year - 1) : year;
  int m = (month < 3) ? (month + 12) : month;
  int k = y % 100;
  int j = y / 100;

  int wday = (day + 13 * (m + 1) / 5 + k + k / 4 + j / 4 + 5 * j) % 7;
  return wday;
}
