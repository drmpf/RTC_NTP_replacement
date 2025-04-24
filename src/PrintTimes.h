#ifndef _PRINT_TIMES_H_
#define _PRINT_TIMES_H_
/*
   PrintTimes.h
 * (c)2025 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This generated code may be freely used for both private and commercial use
 * provided this copyright is maintained.
 */

/**
   Prints a tm struct in a human-readable format using Arduino Print

   @param tm_n Pointer to the struct tm to print
   @param outPtr Pointer to the Print object to use for output
*/
void print_tm(struct tm * tmPtr, Print * outPtr);

String get_ddMMYYYY(struct tm * tmPtr);
String getHHMMss(struct tm * tmPtr);
String getHHMM(struct tm * tmPtr);
String get_yyyymmdd(struct tm * tmPtr);



#endif
