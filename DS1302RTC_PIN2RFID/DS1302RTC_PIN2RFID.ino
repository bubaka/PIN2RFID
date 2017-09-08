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
  // Format: Hari, Tanggal/Bulan/Tahun Jam
  snprintf(buf, sizeof(buf), "%s, %02d/%02d/%04d %02d:%02d:%02d",
           day.c_str(), t.date, t.mon, t.yr, t.hr, t.min, t.sec);
  // Print the formatted string to serial so we can see the time.
  Serial.println(buf);
  } // end of printTime
} // end of namespace

void setup() {
  Serial.begin(9600);
  
  Serial.println("DEMO DS1302 RTC ON PIN2RFID BOARD");
  Serial.println("=================================");

  // Initialize a new chip by turning off write protection and
  // clearing the clock halt flag. It's needn't always be called. 
  rtc.writeProtect(false);
  rtc.halt(false);
}

// Loop and print the time every second.
void loop() {
  int tgl = 0;  int jam = 0;
  int bln = 0;  int mnt = 0;
  int thn = 0;  int det = 0;
  int hari =0;
  
  printTime();
  //Format setting Time: #tgl,bln,thn,jam,menit,detik,hari
  if (Serial.find("#")) {
    tgl = Serial.parseInt(); bln = Serial.parseInt(); thn = Serial.parseInt(); 
    jam = Serial.parseInt(); mnt = Serial.parseInt(); det = Serial.parseInt(); 
    hari = Serial.parseInt();
    
    switch (hari) {
      case 1: {Time t(thn,bln,tgl,jam,mnt,det,Time::kSunday); rtc.time(t);} break;
      case 2: {Time t(thn,bln,tgl,jam,mnt,det,Time::kMonday); rtc.time(t);} break;
      case 3: {Time t(thn,bln,tgl,jam,mnt,det,Time::kTuesday); rtc.time(t);} break;
      case 4: {Time t(thn,bln,tgl,jam,mnt,det,Time::kWednesday); rtc.time(t);} break;
      case 5: {Time t(thn,bln,tgl,jam,mnt,det,Time::kThursday); rtc.time(t);} break;
      case 6: {Time t(thn,bln,tgl,jam,mnt,det,Time::kFriday); rtc.time(t);} break;
      case 7: {Time t(thn,bln,tgl,jam,mnt,det,Time::kSaturday); rtc.time(t);} break;
    }
/*
char *output= "xxxxxxxxx";
	Time t;
	t=getTime();
	switch (t.mon)
	{
		case 1: output="January"; break;
		case 2: output="February"; break;
		case 3: output="March"; break;
		case 4: output="April"; break;
		case 5: output="May"; break;
		case 6: output="June"; break;
		case 7: output="July"; break;
		case 8: output="August"; break;
		case 9: output="September"; break;
		case 10: output="October"; break;
		case 11: output="November"; break;
		case 12: output="December"; break;
	}     
*/
    Serial.println("Setting New Time... Done");
  }
  //delay(100);
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