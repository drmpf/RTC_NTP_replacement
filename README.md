# RTC_NTP_replacement
This library uses a DS3231 RTC as a replacement for ESP32 NTP client, including time zone, for complete off-line time keeping.  

This library provides web pages to set date/time and time zone, via either an ESP32 Access Point or the local network. 

User code just uses the standard ESP32 time/date methods and does not have access the RTC.  
The library keeps the ESP32 system time synced to the RTC to within +/-5ms.  

Includes examples of optional syncing of RTC from NTP and of using AsyncWebServer instead of the basic ESP32 webserver.  

# Features
This RTC_NTP_replacement library is for the ESP32 and DS3231 RTC and has the following features:-  
– Runs completely off-line, using the RTC (DS3231) to synchronize the ESP32's system clock to within a few milliseconds.  
– Uses the standard ESP32 time methods to get times and dates. The RTC is not accessed from user code.  
– Provides 'smooth' synchronization of the ESP32's system clock to the RTC's time for consistent time stamps.  
– Provides locally served web pages to set the time and the time zone. The web pages are formatted for easy display on a mobile phone.  
– Has a time zone parser to clean up the user inputted time zone to a valid one that the ESP32 can use.  
– Can optionally to use an NTP server to synchronize the RTC's date and time which then synchronizes the system time.  
– Can optionally to use AsyncWebServer instead of ESP32's built in web server  

# How-To
See [RTC_NTP_replacement Tutorial](https://www.forward.com.au/pfod/ArduinoProgramming/RTC_NTP_replacement/index.html)  


# Revisions
V1.0.0 initial release    

