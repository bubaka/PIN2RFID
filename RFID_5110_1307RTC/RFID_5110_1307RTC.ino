#include
#include
int LED=7;
Adafruit_PCD8544 display = Adafruit_PCD8544(2, 3, 4, 6, 5);

#include
#include
#include
#include "RTClib.h"


RTC_DS1307 rtc;
#define RST_PIN  9  
#define SS_PIN  10  

MFRC522 mfrc522(SS_PIN, RST_PIN); 

void setup()
{
  Serial.begin(9600);

  #ifdef AVR
  Wire.begin();
#else
  Wire1.begin();
#endif
  rtc.begin();

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
     rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
     // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }


 pinMode(LED,OUTPUT);
 while (!Serial);  
 SPI.begin();   
 mfrc522.PCD_Init();  
 Serial.println("Test Display Oke");
  display.begin();
 display.setContrast(60);
   display.display();
  delay(2000);
  display.clearDisplay();

}

void loop()
{
  digitalWrite(LED,LOW);
  DateTime now = rtc.now();
    if ( ! mfrc522.PICC_IsNewCardPresent()) {
  
            display.setCursor(0, 0);
            display.setTextSize(1);
            display.print(now.year(), DEC);display.print('/');display.print(now.month(), DEC);display.print('/');display.println(now.day(), DEC);
            display.setCursor(0, 10);display.setTextSize(2);
            display.print(now.hour(), DEC);display.print(':');display.println(now.minute(), DEC);
            display.setCursor(0, 25);
            display.print(now.second(), DEC);
            display.display();
            display.clearDisplay();
             
            delay(100);
            digitalWrite(LED, LOW);
            display.clearDisplay();
            return;
 }
        if ( ! mfrc522.PICC_ReadCardSerial()) {
  return;
 }

String rfidUid = "";
for (byte i = 0; i < mfrc522.uid.size; i++) {
  rfidUid += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
  rfidUid += String(mfrc522.uid.uidByte[i], HEX);
}
if (!(rfidUid=="")){
    display.clearDisplay();
    rfidUid.toUpperCase();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.print("UID: ");
    display.println(rfidUid);
    display.print(now.year(), DEC);display.print('/');display.print(now.month(), DEC);display.print('/');display.println(now.day(), DEC);
    display.print(now.hour(), DEC);display.print(':');display.print(now.minute(), DEC);display.print(':');display.println(now.second(), DEC);
    display.println("USER :");
    FindID(rfidUid);
    display.display();
    digitalWrite(LED,HIGH);
    delay(2000);
}

}

void FindID( String NoUID){
  if (NoUID=="1109F44F"){
  display.print("JAKA BUDI");
  }
  else if (NoUID=="C1202C4F"){
  display.print("SRI RACMAWATI");
  }

  else if (NoUID=="61A7D74F"){
  display.print("A DIPTA S");
  }
  else if (NoUID=="C1CFD94F"){
  display.print("A DAMAR S");
  }
  else
  {
   display.print("TIDAK DIKENAL");
  }
}
