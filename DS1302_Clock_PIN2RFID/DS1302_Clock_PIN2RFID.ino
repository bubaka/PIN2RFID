// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// DS1302RTC Serial Demo @PIN2RFID Board
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
#include <stdio.h>
#include <DS1302.h>

//Init DS1302RTC pins Connection
namespace {
const int kCePin = 10;   // Chip Enable
const int kIoPin = 7;   // Input/Output
const int kSclkPin = 6; // Serial Clock

// Create a DS1302 object.
DS1302 rtc(kCePin, kIoPin, kSclkPin);

String dayAsString(const Time::Day day) {
  switch (day) {
    case Time::kSunday: return "Minggu";
    case Time::kMonday: return "Senin";
    case Time::kTuesday: return "Selasa";
    case Time::kWednesday: return "Rabu";
    case Time::kThursday: return "Kamis";
    case Time::kFriday: return "Jumat";
    case Time::kSaturday: return "Sabtu";
  }
  return "(Harimau ?)";
}

void printTime() {
  // Get the current time and date from the chip.
  Time t = rtc.time();

  // Name the day of the week.
  const String day = dayAsString(t.day);

  // Format the time and date and insert into the temporary buffer.
  char buf[50];
  // Format: Hari, Tahun-Bulan-Tanggal Jam
  //snprintf(buf, sizeof(buf), "%s, %04d-%02d-%02d %02d:%02d:%02d",
  //         day.c_str(), t.yr, t.mon, t.date, t.hr, t.min, t.sec);
  
  // Format: Hari, Tanggal/Bulan/Tahun Jam
  snprintf(buf, sizeof(buf), "%s, %02d/%02d/%04d %02d:%02d:%02d",
           day.c_str(), t.date, t.mon, t.yr, t.hr, t.min, t.sec);

  // Print the formatted string to serial so we can see the time.
  Serial.println(buf);
  } // end of printTime
} // end of namespace

void setup() {
  Serial.begin(9600);
  
  Serial.println("DEMO DS1302 RTC ON SAUMPY328 DEVBOARD");
  Serial.println("====================================");

  // Initialize a new chip by turning off write protection and
  // clearing the clock halt flag. It's needn't always be called. 
  rtc.writeProtect(false);
  rtc.halt(false);

  // Remove the next define, after the right date and time are set.
  //#define SET_DATE_TIME_JUST_ONCE
  
  #ifdef SET_DATE_TIME_JUST_ONCE
    // Make a new time object to set the date and time.
    Time t(2014, 12, 22, 16, 48, 00, Time::kMonday);
    // Set the time and date on the chip.
    rtc.time(t);
  #endif
}

// Loop and print the time every second.
void loop() {
  printTime();
  delay(1000);
}

// =========== CREDIT ============
// Copyright (c) 2009, Matt Sparks
// All rights reserved.
//
// http://quadpoint.org/projects/arduino-ds1302
// Set the appropriate digital I/O pin connections. These are the pin
// assignments for the Arduino as well for as the DS1302 chip. 
// See the DS1302 datasheet:
// http://datasheets.maximintegrated.com/en/ds/DS1302.pdf
