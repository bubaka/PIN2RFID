//======================== RFID PIN2 ============================

#include <LiquidCrystal.h>
#include <EEPROM.h>  // We are going to read and write PICC's UIDs from/to EEPROM
#include <SPI.h>     // RC522 Module uses SPI protocol
#include <MFRC522.h> // Library for Mifare RC522 Devices
#include <DS1302.h>

//#define COMMON_ANODE //Define LED Control
#define COMMON_CATHODE

#ifdef COMMON_ANODE
#define LED_ON LOW
#define LED_OFF HIGH
#else
#define LED_ON HIGH
#define LED_OFF LOW
#endif

#define ReadyLED 2    // Ready / RedLED
#define LockLED 3     // Unlock /GreenLED
#define Buzzer 4      // Process Tag /BlueLED
#define Backlight 5   // LCD BackLight
#define WipeBtn 8     // Button pin for WipeMode
#define Relay 9       // Relay Actuator

boolean match = false; // initialize card match to false
boolean programMode = false; // initialize programming mode to false

int successRead; // Variable integer to keep if we have Successful Read from Reader
int counterflip; // Counter for flip LED
int posCur;

byte storedCard[4]; // Stores an ID read from EEPROM
byte readCard[4]; // Stores scanned ID read from RFID Module
byte masterCard[4]; // Stores master card's ID read from EEPROM

/* We need to define MFRC522's pins and create instance
* Pin layout should be as follows (on Arduino Uno):
* MOSI: Pin 11 / ICSP-4
* MISO: Pin 12 / ICSP-1
* SCK : Pin 13 / ICSP-3
* SS : Pin 10 (Configurable)
* RST : Pin 9 (Configurable)
* look MFRC522 Library for
* pin configuration for other Arduinos.
*/

#define SS_PIN 6 //NC
#define RST_PIN 7
MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance.

/*
Initialize the library with the numbers of the interface pins
LCD Connection :
 * LCD RS pin to digital pin A5
 * LCD EN pin to digital pin A4
 * LCD D4 pin to digital pin A3
 * LCD D5 pin to digital pin A2
 * LCD D6 pin to digital pin A1
 * LCD D7 pin to digital pin A0
 * LCD RW pin to ground
 * 10K trimpot:
   - ends to +5V and ground
   - wiper to LCD VO pin (pin 3)
*/   
LiquidCrystal lcd(A5, A4, A3, A2, A1, A0);

// Init DS1302 RTC pins connection
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
  return "(Hari?)";
}
 

///////////////////////////////////////// Setup ///////////////////////////////////
void setup() {
  //Arduino Pin Configuration
  pinMode(ReadyLED, OUTPUT);
  pinMode(LockLED, OUTPUT);
  pinMode(Backlight, OUTPUT);
  pinMode(Buzzer, OUTPUT);
  pinMode(Relay, OUTPUT);
  pinMode(WipeBtn, INPUT);

  digitalWrite(ReadyLED, LED_OFF); // Make sure led is off
  digitalWrite(LockLED, LED_OFF); // Make sure led is off
  digitalWrite(Buzzer, LOW); // Make sure Buzzer is off
  digitalWrite(Relay, HIGH); // Make sure door is locked
    
  lcd.begin(16,2); // set up the LCD's number of columns and rows: 

  //Protocol Configuration
  Serial.begin(9600); // Initialize serial communications with PC
  SPI.begin(); // MFRC522 Hardware uses SPI protocol
  
  // Initialize MFRC522 Hardware
  mfrc522.PCD_Init(); 
  //Set Antenna Gain to Max- this will increase reading distance
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max); 

  //Define DS 1302 pin
  DS1302 rtc(kCePin, kIoPin, kSclkPin);
  
  Serial.println("===|[ PIN2RFID Door Acces Control v1.0 ]|==="); //For debug purposes
  Serial.println("");
  printTime2Serial(); //print format time to serial
  wipe();   //Wipe Code if Button Pressed while setup run (powered on) it wipes EEPROM
  
 
  digitalWrite(Backlight, LED_ON); // Turn On Backlight LCD
  lcd.setCursor(0,0); lcd.print("-|[ PIN2RFID ]|-");    
  lcd.setCursor(0,1); lcd.print("Door Access Ctrl");
  delay(1000); 
  BuzzerBeep();
  homedisplay();
}

///////////////////////////////////////// Main Loop ///////////////////////////////////
void loop () {
  do {
    // sets successRead to 1 when we get read from reader otherwise 0
    successRead = getID(); 
    if (programMode) {
      programModeOn(); // Program Mode cycles through RGB waiting to read a new card
    }
    else {
      normalModeOn(); // Normal mode, blue Power LED is on, all others are off      
    }
  }
  while (!successRead); //the program will not go further while you not get a successful read
  if (programMode) {
    //If master card scanned again exit Program Mode
    if ( isMaster(readCard) ) { 
      Serial.println("This is Master Card");
      Serial.println("Exiting Program Mode");
      Serial.println("-----------------------------");
      lcd.setCursor(0,0); lcd.print(" Master Card OK ");    
      lcd.setCursor(0,1); lcd.print("Exit ProgramMode");
      BuzzerBeep();      
      programMode = false;
      delay(1000); homedisplay();
      return;
    }
    else {
      //If scanned card is known delete it
      if ( findID(readCard) ) { 
        Serial.println("I know this Card, so removing");
        lcd.setCursor(0,0); lcd.print("Erase Card ID  ");    
        lcd.setCursor(0,1); lcd.print("from dBase...! ");
        deleteID(readCard);
        BuzzerBeep();
        delay(1000);
        lcd.clear(); 
        lcd.setCursor(0,0); lcd.print("[ PROGRAM MODE ]");
        lcd.setCursor(0,1); lcd.print(" (( ScanCard )) ");
      }
      else { 
        // If scanned card is not known add it
        Serial.println("I do not know this Card, adding...");
        lcd.setCursor(0,0); lcd.print("New Card detect!");    
        lcd.setCursor(0,1); lcd.print("Add to dBase... ");
        writeID(readCard);
        BuzzerBeep();
        delay(1000);
        lcd.setCursor(0,0); lcd.print("[ PROGRAM MODE ]");
        lcd.setCursor(0,1); lcd.print(" (( ScanCard )) ");
      }
    }
  }
  else {
    if ( isMaster(readCard) ) { // If scanned card's ID matches Master Card's ID enter program mode
      programMode = true;
      BuzzerBeep();
      digitalWrite(Backlight, LED_ON); // Turn On Backlight LCD
      Serial.println("===================================");
      Serial.println("Hello Master - Entered PROGRAM MODE");
      lcd.setCursor(0,0); lcd.print("[ PROGRAM MODE ]");    
      lcd.setCursor(0,1); lcd.print(" Hello Master!! ");
      delay(1000); 
      
      int count = EEPROM.read(0); // Read the first Byte of EEPROM that
      Serial.print("I have "); // stores the number of ID's in EEPROM
      Serial.print(count);
      Serial.print(" record(s) on EEPROM");
      Serial.println("");
      
      lcd.clear(); 
      lcd.setCursor(0,0); lcd.print("I have "); lcd.print(count); 
      lcd.print(" record(s) on EEPROM");    
      delay(1000); scroll(20); delay(500);
      
      Serial.println("Scan a Card to ADD or REMOVE");
      //Serial.println("");
      lcd.clear();
      lcd.print("Please Scan a Card to ADD or REMOVE");    
      delay(500); scroll(20); delay(500);
      
      Serial.println("or scan Master Card to exit PROGRAM MODE");
      Serial.println("---------- [ PROGRAM MODE ] ------------");
      lcd.clear();
      lcd.setCursor(0,1); lcd.print("Scan MasterCard to exit PROGRAM MODE ");    
      delay(500); scroll(40); delay(500);
      lcd.clear();
      lcd.setCursor(0,0); lcd.print("[ PROGRAM MODE ]");
      lcd.setCursor(0,1); lcd.print(" (( ScanCard )) ");
          
    }
    else {
      if ( findID(readCard) ) { // If not, see if the card is in the EEPROM
        digitalWrite(Backlight, LED_ON); // Turn On Backlight LCD
        Serial.println("Welcome, Access Granted");
        Serial.println("-----------------------------");
        lcd.setCursor(0,0); lcd.print(" Access Granted ");    
        lcd.setCursor(0,1); lcd.print("    Welcome!!   ");
        openDoor(1000); // Open the door lock for 1000 ms
        homedisplay();
      }
      else { // If not, show that the ID was not valid
        digitalWrite(Backlight, LED_ON); // Turn On Backlight LCD
        Serial.println("Access Denied");
        Serial.println("-----------------------------");
        lcd.setCursor(0,0); lcd.print(" Access Denied! ");    
        lcd.setCursor(0,1); lcd.print(" ID:            ");
        posCur=5;
        for (int i = 0; i < 4; i++) { 
          lcd.setCursor(posCur+i,1);
          lcd.print(readCard[i] < 0x10 ? "0" : ""); //solve HEX = 0 
          lcd.print(readCard[i],HEX);
          posCur++;
        }
        failed(); delay(1000);
        lcd.setCursor(0,1); lcd.print("Card Unregister ");
        delay(1000);
        homedisplay();
      }
    }
  }
}

///////////////////////////////////////// Check if WIPE MODE ///////////////////////////////////
void wipe() {
  //Wipe Code if Button Pressed while setup run (powered on) it wipes EEPROM
  pinMode(WipeBtn, INPUT_PULLUP); // Enable pin's pull up resistor
  // when button pressed pin should get low, button connected to ground
  if (digitalRead(WipeBtn) == LOW) {  
    digitalWrite(Backlight, LED_ON); // Turn On Backlight LCD
    digitalWrite(ReadyLED, LED_ON); // Ready Led stays on to inform user we are going to wipe
    Serial.println("!!! Wipe Button Pressed !!!");
    Serial.println("You have 5 seconds to Cancel");
    Serial.println("This will be remove all records and cannot be undone");
    lcd.setCursor(0,0); lcd.print("   WARNING !!!  ");
    lcd.setCursor(0,1); lcd.print("WipeBtn Pressed!");
    BeepError();
    delay(2000);
    lcd.setCursor(0,0); lcd.print("You've 5 seconds");
    lcd.setCursor(0,1); lcd.print("to cancel WIPE!!");
    BeepError();
    delay(5000); // Give user enough time to cancel operation
    if (digitalRead(WipeBtn) == LOW) { // If button still be pressed, wipe EEPROM
      Serial.println("!!! Starting Wiping EEPROM !!!");
      lcd.setCursor(0,0); lcd.print("   Starting     ");    
      lcd.setCursor(0,1); lcd.print(" Wiping EEPROM!!");
      delay(1000); 
      for (int x=0; x<1024; x=x+1){ //Loop end of EEPROM address
        if (EEPROM.read(x) == 0){ //If EEPROM address 0
          // do nothing, already clear, go to the next address in order to save time and reduce writes to EEPROM
        }
        else{
          EEPROM.write(x, 0); // if not write 0, it takes 3.3mS
        }
      }
      Serial.println("!!! Wiped !!!");          
      lcd.clear(); lcd.setCursor(0,0); lcd.print(" !!! Wiped !!!  ");
      BeepSuccess();
    }
    else {
      Serial.println("!!! Wiping Cancelled !!!");
      lcd.clear(); lcd.setCursor(0,0); lcd.print("Wiping Canceled!");
      digitalWrite(ReadyLED, LED_OFF);
    }
    digitalWrite(Backlight, LED_OFF); // Turn Off Backlight LCD
  }
  //Check if master card defined, if not let user choose a master card
  //This also useful to just redefine Master Card
  //You can keep other EEPROM records just write other than 1 to EEPROM address 1
  if (EEPROM.read(1) != 1) { 
    // Look EEPROM if Master Card defined, EEPROM address 1 holds if defined
    Serial.println("No Master Card Defined");
    Serial.println("Scan Card to Define as Master Card");
    digitalWrite(Backlight, LED_ON); // Turn On Backlight LCD
    lcd.setCursor(0,0); lcd.print("No Master Card  ");
    lcd.setCursor(0,1); lcd.print("Scan to Define!!");
    //delay(1000); //homedisplay();
    do {
      // sets successRead to 1 when we get read from reader otherwise 0
      successRead = getID(); 
      delay(500); 
      BuzzerBeep();
    }
    while (!successRead); //the program will not go further while you not get a successful read
    for ( int j = 0; j < 4; j++ ) { // Loop 4 times
      EEPROM.write( 2 +j, readCard[j] ); // Write scanned PICC's UID to EEPROM, start from address 3
    }
    EEPROM.write(1,1); //Write to EEPROM we defined Master Card.
    Serial.println("Master Card Defined");
    lcd.setCursor(0,0); lcd.print("  Master Card   ");
    lcd.setCursor(0,1); lcd.print("  Defined !!    ");
    BuzzerBeep(); 
    delay(500); 
  }
  //Information UID MasterCard
  Serial.println("Master Card ID:");
  for ( int i = 0; i < 4; i++ ) { // Read Master Card's UID from EEPROM
    masterCard[i] = EEPROM.read(2+i); //Write it to masterCard
    Serial.print(masterCard[i] < 0x10 ? " 0" : " ");
    Serial.print(masterCard[i], HEX);
  }
  Serial.println(""); Serial.println("-----------------------------");
}

///////////////////////////////////////// Get PICC Card ID ///////////////////////////////////
int getID() {
  // Getting ready for Reading PICCs
  if ( ! mfrc522.PICC_IsNewCardPresent()) { //If a new PICC placed to RFID reader continue
    return 0;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) { //Since a PICC placed get Serial and continue
    return 0;
  }
  // There are Mifare PICCs which have 4 byte or 7 byte UID care if you use 7 byte PICC
  // I think we should assume every PICC as they have 4 byte UID
  // Until we support 7 byte PICCs
  Serial.println("Scanned Card ID:");
  for (int i = 0; i < 4; i++) { //
    readCard[i] = mfrc522.uid.uidByte[i];
    Serial.print(readCard[i] < 0x10 ? " 0" : " "); //Solve HEX = 0
    Serial.print(readCard[i], HEX);
  }
  Serial.println("");
  mfrc522.PICC_HaltA(); // Stop reading
  return 1;
}

///////////////////////////////////////// Program Mode Leds ///////////////////////////////////
void programModeOn() {
  digitalWrite(ReadyLED, LED_ON); delay(200);
  digitalWrite(ReadyLED, LED_OFF);  delay(200);
  digitalWrite(LockLED, LED_ON);  delay(200);
  digitalWrite(LockLED, LED_OFF);   delay(200);
  BuzzerBeep(); delay(200);     // Beep !!!
}

//////////////////////////////////////// Normal Mode Leds ///////////////////////////////////
void normalModeOn () {
  counterflip++;
  if (counterflip > 10) {
    digitalWrite(ReadyLED, LED_OFF);
    if (counterflip == 20) { counterflip = 0; }     
  }
  else {
    digitalWrite(ReadyLED, LED_ON);
  }
  digitalWrite(LockLED, LED_ON); // Make sure Green LED is off
  digitalWrite(Relay, HIGH); // Make sure Door is Locked
}

//////////////////////////////////////// Read an ID from EEPROM //////////////////////////////
void readID( int number ) {
  int start = (number * 4 ) + 2; // Figure out starting position
  for ( int i = 0; i < 4; i++ ) { // Loop 4 times to get the 4 Bytes
    storedCard[i] = EEPROM.read(start+i); // Assign values read from EEPROM to array
  }
}

///////////////////////////////////////// Add ID to EEPROM ///////////////////////////////////
void writeID( byte a[] ) {
  if ( !findID( a ) ) { // Before we write to the EEPROM, check to see if we have seen this card before!
    int num = EEPROM.read(0); // Get the numer of used spaces, position 0 stores the number of ID cards
    int start = ( num * 4 ) + 6; // Figure out where the next slot starts
    num++; // Increment the counter by one
    EEPROM.write( 0, num ); // Write the new count to the counter
    for ( int j = 0; j < 4; j++ ) { // Loop 4 times
      EEPROM.write( start+j, a[j] ); // Write the array values to EEPROM in the right position
    }
    successWrite();
  }
  else {
    failedWrite();
  }
}

///////////////////////////////////////// Remove ID from EEPROM ///////////////////////////////////
void deleteID( byte a[] ) {
  if ( !findID( a ) ) { // Before we delete from the EEPROM, check to see if we have this card!
    failedWrite(); // If not
  }
  else {
    int num = EEPROM.read(0); // Get the numer of used spaces, position 0 stores the number of ID cards
    int slot; // Figure out the slot number of the card
    int start;// = ( num * 4 ) + 6; // Figure out where the next slot starts
    int looping; // The number of times the loop repeats
    int j;
    int count = EEPROM.read(0); // Read the first Byte of EEPROM that stores number of cards
    slot = findIDSLOT( a ); //Figure out the slot number of the card to delete
    start = (slot * 4) + 2;
    looping = ((num - slot) * 4);
    num--; // Decrement the counter by one
    EEPROM.write( 0, num ); // Write the new count to the counter
    for ( j = 0; j < looping; j++ ) { // Loop the card shift times
      EEPROM.write( start+j, EEPROM.read(start+4+j)); // Shift the array values to 4 places earlier in the EEPROM
    }
    for ( int k = 0; k < 4; k++ ) { //Shifting loop
      EEPROM.write( start+j+k, 0);
    }
    successDelete();
  }
}

///////////////////////////////////////// Check Bytes ///////////////////////////////////
boolean checkTwo ( byte a[], byte b[] ) {
  if ( a[0] != NULL ) // Make sure there is something in the array first
    match = true; // Assume they match at first
  for ( int k = 0; k < 4; k++ ) { // Loop 4 times
    if ( a[k] != b[k] ) // IF a != b then set match = false, one fails, all fail
      match = false;
  }
  if ( match ) { // Check to see if if match is still true
    return true; // Return true
  }
  else {
    return false; // Return false
  }
}

///////////////////////////////////////// Find Slot ///////////////////////////////////
int findIDSLOT( byte find[] ) {
  int count = EEPROM.read(0); // Read the first Byte of EEPROM that
  for ( int i = 1; i <= count; i++ ) { // Loop once for each EEPROM entry
    readID(i); // Read an ID from EEPROM, it is stored in storedCard[4]
    if( checkTwo( find, storedCard ) ) { // Check to see if the storedCard read from EEPROM
      // is the same as the find[] ID card passed
      return i; // The slot number of the card
      break; // Stop looking we found it
    }
  }
}

///////////////////////////////////////// Find ID From EEPROM ///////////////////////////////////
boolean findID( byte find[] ) {
  int count = EEPROM.read(0); // Read the first Byte of EEPROM that
  for ( int i = 1; i <= count; i++ ) { // Loop once for each EEPROM entry
    readID(i); // Read an ID from EEPROM, it is stored in storedCard[4]
    if( checkTwo( find, storedCard ) ) { // Check to see if the storedCard read from EEPROM
      return true;
      break; // Stop looking we found it
    }
    else { // If not, return false
    }
  }
  return false;
}

///////////////////////////////////////// Write Success to EEPROM ///////////////////////////////////
// Flashes the green LED 3 times to indicate a successful write to EEPROM
void successWrite() {
  BeepSuccess();
  Serial.println("Succesfully added ID record to EEPROM");
  lcd.setCursor(0,0); lcd.print("Adding ID record");    
  lcd.setCursor(0,1); lcd.print("      Done!!    ");
  delay(1000);      
}

///////////////////////////////////////// Write Failed to EEPROM ///////////////////////////////////
// Flashes the Ready (red) LED 3 times to indicate a failed write to EEPROM
void failedWrite() {
  BeepError();
  Serial.println("Failed! There is something wrong with ID or bad EEPROM");
  lcd.setCursor(0,0); lcd.print("Adding ID record");    
  lcd.setCursor(0,1); lcd.print("     Failed!!   ");
  delay(1000);
}

///////////////////////////////////////// Success Remove UID From EEPROM ///////////////////////////////////
// Flashes the blue LED 3 times to indicate a success delete to EEPROM
void successDelete() {
  BeepSuccess();
  Serial.println("Succesfully removed ID record from EEPROM");
  lcd.setCursor(0,0); lcd.print("Remove ID record");    
  lcd.setCursor(0,1); lcd.print("      Done!!    ");
  delay(1000);
}

////////////////////// Check readCard IF is masterCard ///////////////////////////////////
// Check to see if the ID passed is the master programing card
boolean isMaster( byte test[] ) {
  if ( checkTwo( test, masterCard ) )
    return true;
  else
    return false;
}

///////////////////////////////////////// Unlock Door ///////////////////////////////////
void openDoor( int setDelay ) {
  BuzzerBeep(); 
  delay(200); 
  BuzzerBeep();
  digitalWrite(LockLED, LED_OFF); // Turn on green LED
  digitalWrite(Relay, LOW); // Unlock door!
  delay(setDelay); // Hold door lock open for given seconds
  digitalWrite(Relay, HIGH); // Relock door
}

///////////////////////////////////////// Failed Access ///////////////////////////////////
void failed() {
  digitalWrite(LockLED, LED_ON); // Make sure green LED is off
  digitalWrite(ReadyLED, LED_OFF); // Turn on Ready LED
  BuzzerBeep();
  digitalWrite(ReadyLED, LED_ON); // Turn on Ready LED
  delay(500);
}

/////////////////////////////////////// Success Indicator ////////////////////////////////
void BeepSuccess() {
  BuzzerBeep(); 
  digitalWrite(ReadyLED, LED_OFF); delay(200);
  digitalWrite(ReadyLED, LED_ON);  delay(200);
  BuzzerBeep(); 
  digitalWrite(ReadyLED, LED_OFF); delay(200);
  digitalWrite(ReadyLED, LED_ON);  delay(200);
}

/////////////////////////////////////// Error Indicator ////////////////////////////////
void BeepError() {
  BuzzerBeep(); 
  digitalWrite(LockLED, LED_OFF); delay(200);
  digitalWrite(LockLED, LED_ON);  delay(200);
  BuzzerBeep(); 
  digitalWrite(LockLED, LED_OFF); delay(200);
  digitalWrite(LockLED, LED_ON);  delay(200);
  BuzzerBeep(); 
  digitalWrite(LockLED, LED_OFF); delay(200);
  digitalWrite(LockLED, LED_ON);  delay(200);
}

///////////////////////////////////////// Scroll Display ///////////////////////////////////
void scroll(int ujung) {
  for (int positionCounter = 0; positionCounter < ujung; positionCounter++) {
    lcd.scrollDisplayLeft(); 
    delay(200); // wait a bit:
  }
}

//====================================== Home Display =======================================
void homedisplay() {
  Serial.println("");
  Serial.println("PIN2RFID Ready ('_')");
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("-|[ PIN2RFID ]|-");    
  lcd.setCursor(0,1); lcd.print("12/12/2014 13:05");
  delay(2000);
  digitalWrite(Backlight, LOW); // Turn Off Backlight LCD
}

//====================================== Buzzer Beep =======================================
void BuzzerBeep() {
  digitalWrite(Buzzer, HIGH);
  delay(200);
  digitalWrite(Buzzer, LOW);
}      

void printTime2Serial() {
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

//======================== CHANGES NOTE ========================
/*
>>20141126
  - Start coding ( I mean modify code... :D )
  - Add Buzzer pin at D5
>>20141127
  - settting delay and LED indicator
  - add LCD library
>>20141201
  - Move hardware to Matrix Board
  - re-config pin 
>>2014124
  - change indicator mode
  - re-structure code 
>>20141215
  - print ID card
  - Error: can't print HEX 0x00 on serial and LCD --> SOLVED
>>20141222
  - Add RTC Function
================================================================
*/

//======================= CREDIT ==============================
/* Arduino RC522 RFID Door Unlocker
* July/2014 Omer Siar Baysal
*
* Unlocks a Door (controls a Relay actually)
* using a RC522 RFID reader with SPI interface on your Arduino
* You define a Master Card which is act as Programmer
* then you can able to choose card holders who able to unlock
* the door or not.
*
* Easy User Interface
*
* Just one RFID tag needed whether Delete or Add Tags
* You can choose to use Leds for output or
* Serial LCD module to inform users. Or you can use both
*
* Stores Information on EEPROM
*
* Information stored on non volatile Arduino's EEPROM
* memory to preserve Users' tag and Master Card
* No Information lost if power lost.
* EEPROM has unlimited Read cycle but 100,000 limited Write cycle.
*
* Security
*
* To keep it simple we are going to use Tag's Unique IDs
* It's simple, a bit secure, but not hacker proof.
*
* MFRC522 Library also lets us to use some authentication
* mechanism, writing blocks and reading back
* and there is great example piece of code
* about reading and writing PICCs
* here > http://makecourse.weebly.com/week10segment1.html
*
* If you rely on heavy security, figure it out how RFID system
* can be secure yourself (personally very curious about it)
*
* Credits
*
* Omer Siar Baysal who put together this project
*
* Idea and most of code from Brett Martin's project
* http://www.instructables.com/id/Arduino-RFID-Door-Lock/
* www.pcmofo.com
*
* MFRC522 Library
* https://github.com/miguelbalboa/rfid
* Based on code Dr.Leong ( WWW.B2CQSHOP.COM )
* Created by Miguel Balboa (circuitito.com), Jan, 2012.
* Rewritten by Søren Thing Andersen (access.thing.dk), fall of 2013
* (Translation to English, refactored, comments, anti collision, cascade levels.)
*
* Arduino Forum Member luisilva for His Massive Code Correction
* http://forum.arduino.cc/index.php?topic=257036.0
* http://forum.arduino.cc/index.php?action=profile;u=198897
*
* License
*
* You are FREE what to do with this code
* Just give credits who put effort on this code
*
* "PICC" short for Proximity Integrated Circuit Card (RFID Tags)
*/

/* Instead of a Relay maybe you want to use a servo
* Servos can lock and unlock door locks too
* There are examples out there.
*/

// #include <Servo.h>

/* For visualizing whats going on hardware
* we need some leds and
* to control door lock a Relay and a wipe button
* (or some other hardware)
* Used common anode led,digitalWriting HIGH turns OFF led
* Mind that if you are going to use common cathode led or
* just seperate leds, simply comment out #define COMMON_ANODE,
*/


