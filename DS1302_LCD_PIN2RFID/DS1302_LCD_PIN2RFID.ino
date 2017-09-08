// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// DS1302RTC Serial Demo @Saumpy328 Board
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
#include <stdio.h>
#include <Time.h>
#include <DS1302.h>
#include <LiquidCrystal.h>

// Remove the next define, after the right date and time are set.
//#define SET_DATE_TIME_JUST_ONCE
#define LCDBackLight 5

boolean setTime = false;

// Init the LCD
//   initialize the library with the numbers of the interface pins
//            lcd(RS,  E, d4, d5, d6, d7)
LiquidCrystal lcd(A5, A4, A3, A2, A1, A0);

namespace {
//Init DS1302RTC pins Connection
const int kCePin = 10;   // Chip Enable
const int kIoPin = 7;   // Input/Output
const int kSclkPin = 6; // Serial Clock

DS1302 rtc(kCePin, kIoPin, kSclkPin);

  String dayAsString(const Time::Day day) {
    switch (day) {
      case Time::kSunday: return "Minggu";
      case Time::kMonday: return "Senin ";
      case Time::kTuesday: return "Selasa";
      case Time::kWednesday: return "Rabu  ";
      case Time::kThursday: return "Kamis ";
      case Time::kFriday: return "Jum'at";
      case Time::kSaturday: return "Sabtu ";
    }
    return "(Harimau ?)";
  }
} // end of namespace

//=======================================================
void setup() {
  // Setup LCD to 16x2 characters
  pinMode(LCDBackLight, OUTPUT);
  digitalWrite(LCDBackLight, HIGH);
  lcd.begin(16, 2);
  
  // Serial Setup 
  Serial.begin(9600);
  
  Serial.println("DEMO DS1302 RTC ON PIN2RFID BOARD");
  Serial.println("====================================");
  
  lcd.setCursor(0,0); lcd.print(" DS1302RTC DEMO ");
  lcd.setCursor(0,1); lcd.print("[ Embedtronix ] ");
  delay(2000);

  // Initialize a new chip by turning off write protection and
  // clearing the clock halt flag. It's needn't always be called. 
  rtc.writeProtect(false);
  rtc.halt(false);
  
  #ifdef SET_DATE_TIME_JUST_ONCE
    // Make a new time object to set the date and time.
    Time t(2014, 12, 22, 20, 44, 00, Time::kMonday);
    // Set the time and date on the chip.
    rtc.time(t);
  #endif
  
  lcd.clear();
}

// Loop and print the time every second.
//======================================
void loop() {
  // ===== Print Time to serial =====
  // Get the current time and date from the chip.
  Time t = rtc.time();

  // Name the day of the week.
  const String day = dayAsString(t.day);

  // Format the time and date and insert into the temporary buffer.
  char buf[50];
  // Format: Hari, Tanggal/Bulan/Tahun Jam:Menit:Detik
  snprintf(buf, sizeof(buf), "%s, %02d/%02d/%04d %02d:%02d:%02d",
           day.c_str(), t.date, t.mon, t.yr, t.hr, t.min, t.sec);

  // Print the formatted string to serial so we can see the time.
  Serial.println(buf);

  // ===== Print Time to LCD =====
  lcd.setCursor(3,0);
  print2digits(t.hr);
  lcd.print("  ");
  print2digits(t.min);
  lcd.print("  ");
  print2digits(t.sec);

  // Display abbreviated Day-of-Week in the lower left corner
  lcd.setCursor(0, 1);
  lcd.print(day.c_str());

  // Display date in the lower right corner
  lcd.setCursor(6, 1);
  print2digits(t.date);
  lcd.print("/");
  print2digits(t.mon);
  lcd.print("/");
  lcd.print(t.yr);

  // Warning!
  //if(timeStatus() != timeSet) {
  //  lcd.setCursor(0, 1);
  //  lcd.print(F("RTC ERROR: SYNC!"));
  //}
  
  // Save day number
  //sday = day();
  if(t.sec > 40) { digitalWrite(LCDBackLight, LOW); }
  else { digitalWrite(LCDBackLight, HIGH); }
  delay(1000); // Wait approx 1 sec
}
  
void print2digits(int number) {
  // Output leading zero
  if (number >= 0 && number < 10) { lcd.write('0'); }
  lcd.print(number);
}

// =========== CREDIT ============
// Copyright (c) 2009, Matt Sparks
// Copyright (c) 2004, Timur Maksiomv
// All rights reserved.
//
// http://quadpoint.org/projects/arduino-ds1302
// Set the appropriate digital I/O pin connections. These are the pin
// assignments for the Arduino as well for as the DS1302 chip. 
// See the DS1302 datasheet:
// http://datasheets.maximintegrated.com/en/ds/DS1302.pdf
//
// A quick demo of how to use DS1302-library to make a quick
// clock using a DS1302 and a 16x2 LCD.
//
// I assume you know how to connect the DS1302 and LCD.
// DS1302:  CE pin    -> Arduino Digital 27
//          I/O pin   -> Arduino Digital 29
//          SCLK pin  -> Arduino Digital 31
//
// LCD:     DB7       -> Arduino Digital 7
//          DB6       -> Arduino Digital 6 
//          DB5       -> Arduino Digital 5
//          DB4       -> Arduino Digital 4
//          E         -> Arduino Digital 9
//          RS        -> Arduino Digital 8