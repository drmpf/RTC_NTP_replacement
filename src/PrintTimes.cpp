// PrintTimes.cpp
/**
 * (c)2025 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This generated code may be freely used for both private and commercial use
 * provided this copyright is maintained.
*/

#include <Arduino.h>
#include "PrintTimes.h"
#include "DebugOut.h"

static const char* const DAYS_OF_WEEK[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

/**
   Prints a tm struct in a human-readable format using Arduino Print

   @param tm_n Pointer to the struct tm to print
   @param outPtr Pointer to the Print object to use for output
*/
void print_tm(struct tm * tmPtr, Print * outPtr) {
  if ((outPtr == NULL) || (tmPtr == NULL)) {
    return;
  }

  // Format: YYYY/MM/DD (Day) HH:MM:SS
  outPtr->print(tmPtr->tm_year + 1900);
  outPtr->print("/");
  outPtr->print(tmPtr->tm_mon + 1);
  outPtr->print("/");
  outPtr->print(tmPtr->tm_mday);
  outPtr->print(" (");
  outPtr->print(DAYS_OF_WEEK[tmPtr->tm_wday]);
  outPtr->print(") ");

  // Add leading zeros for time components
  if (tmPtr->tm_hour < 10) outPtr->print("0");
  outPtr->print(tmPtr->tm_hour);
  outPtr->print(":");
  if (tmPtr->tm_min < 10) outPtr->print("0");
  outPtr->print(tmPtr->tm_min);
  outPtr->print(":");
  if (tmPtr->tm_sec < 10) outPtr->print("0");
  outPtr->print(tmPtr->tm_sec);
  outPtr->println();
}

// only handles +v numbers
static void print2digits(String & result, int num) {
  if ((num >=0) && (num < 10)) {
    result += '0';
  }
  result += num;
}


String get_ddMMYYYY(struct tm * tmPtr) {
  String result; // dd/mm/yyyy
  print2digits(result, tmPtr->tm_mday );
  result += '/';
  print2digits(result, tmPtr->tm_mon + 1);
  result += '/';
  print2digits(result, tmPtr->tm_year + 1900);
  return result;
}

String getHHMMss(struct tm * tmPtr) {
  String result; // hh:mm:ss
  print2digits(result, tmPtr->tm_hour);
  result += ':';
  print2digits(result, tmPtr->tm_min);
  result += ':';
  print2digits(result, tmPtr->tm_sec);
  return result;
}

String getHHMM(struct tm * tmPtr) {
  String result; // hh:mm:ss
  print2digits(result, tmPtr->tm_hour);
  result += ':';
  print2digits(result, tmPtr->tm_min);
  return result;
}

String get_yyyymmdd(struct tm * tmPtr) {
  String result; // yyyy-mm-dd
  print2digits(result, tmPtr->tm_year + 1900);
  result += '-';
  print2digits(result, tmPtr->tm_mon + 1);
  result += '-';
  print2digits(result, tmPtr->tm_mday );
  return result;
}
