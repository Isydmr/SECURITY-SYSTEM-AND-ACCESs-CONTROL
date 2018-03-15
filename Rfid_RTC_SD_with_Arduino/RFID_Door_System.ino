#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include "SdFat.h"
#include "RTClib.h"

#define RST_PIN 9 
#define SS_PIN 10 //RFID Slave PIN --> 10 
#define ledPin 7
#define CS_pin 4  //SD Card Module Slave PIN -->4

SdFat SD;
MFRC522 mfrc522(SS_PIN, RST_PIN);
byte uidByte[4];
int successRead;
int readSD(String UID);
String buffer;
MFRC522::MIFARE_Key key;
File myFile;
void card_register(String rfid);
RTC_DS1307 RTC;

void setup()
{
  Serial.begin(9600);
  SPI.begin();
  
  mfrc522.PCD_Init();
  
  SD.begin(CS_pin);
  while (!Serial) {;}
  Wire.begin();
  
  RTC.begin();
  // Check to see if the RTC is keeping time.
  if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
    RTC.adjust(DateTime(__DATE__, __TIME__)); // Obtain time info
  }
  
  pinMode(SS_PIN, OUTPUT);
  digitalWrite(CS_pin, LOW);
  pinMode(ledPin, OUTPUT);
  
  Serial.println("Rfid Access Control System");
  Serial.println("--------------------------");
  Serial.println();
  if (!SD.begin(CS_pin)) {
    Serial.println("initialization failed!");
    while (1);
  } else {
    //Nothing wrong with SD setup
    Serial.println("Wiring is correct and a card is present."); 
  }
}

void loop()
{
  String rfid;//String that keeps rfid info for every loop
  rfid = "";  //Reset rfid info in every loop

  if ( ! mfrc522.PICC_IsNewCardPresent())
  {
    return;
  }
  if ( ! mfrc522.PICC_ReadCardSerial())
  {
    return;
  }

  //Read Card's UID, save it to string named rfid.
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    rfid += mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ";
    rfid += String(mfrc522.uid.uidByte[i], HEX);
  }
  //Clean rfid info, it's kind of a standardization
  rfid.trim();
  rfid.toUpperCase();

  /*  Here, if Admin Card UID has read by RFID 
   *  go to card_register function
   *  this function saves the unknown card to SD
   *  in order to give access authorization to this card 
   */
  if (rfid == "94 06 C6 D5") {
    card_register(rfid);
    Serial.print("Kart Kayit Bitti-----\n");
  }
  Serial.print("Card UID:");
  // Another standardization to remove white-spaces
  rfid.replace(" ",""); 
  Serial.println(rfid);
  Serial.println();
  /* readSD() checks SD Card to see if this UID exists in data
   *  if it exists, return 0 !
   */
  if (readSD(rfid) == 0)
  {
    /*Blink the led, make some noise with buzzer, 
     * open the door etc.
     * Here, we proven this tag is authorised.
     */
    time_sd(rfid); // Write the time info to SD Card
    Serial.println("Welcome");
    digitalWrite(ledPin, HIGH);
    Serial.println("Access Granted");
    delay(2000);
    digitalWrite(ledPin, LOW);
  }
  rfid = "";
  Serial.println();
  delay(200);

}
int readSD(String UID)
{
  // Master, Slave config
  select_sd();
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  SD.begin(CS_pin);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  pinMode(SS_PIN, OUTPUT);
  digitalWrite(CS_pin, LOW);
  pinMode(ledPin, OUTPUT);
  
  int flag=1;
  //Read from Sd
  myFile = SD.open("test.txt", FILE_READ);
  if (!myFile) {
    // It failed, so try and make a new file.
    myFile = SD.open("test.txt", FILE_READ);
    if (!myFile) {
      // It failed too, so give up.
      Serial.println("Failed to open file.txt");
      myFile.close();
    }
  }
  // Only write to the file if the file is actually open.
  if (myFile) {
    while (myFile.available()) {
          //
          buffer = myFile.readStringUntil('\n');
          Serial.println(buffer);
    /* Remove white-spaces and make all letters uppercase */
    UID.trim();
    UID.toUpperCase();
    UID.replace(" ","");
    /* Remove white-spaces and make all letters uppercase */
    buffer.trim();
    buffer.toUpperCase();
    buffer.replace(" ","");
    /* Finally, if UID is same with buffer return 0 to our loop function */
      if ( UID.equals(buffer) ) {
        Serial.println("Card exists!");
        flag = 0;
      }
    }
    // Close file
    myFile.close();
  }
  return flag;
}
int getID() {
  //Return 0 if there's no new card
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return 0;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return 0;
  }
  Serial.print("Card's UID: ");
  //Read byte by byte
  for (int i = 0 ; i < mfrc522.uid.size; i++) {  //
    uidByte[i] = mfrc522.uid.uidByte[i];
  }
  Serial.println("");
  //Stop reading, if read is successful return 1
  mfrc522.PICC_HaltA();
  return 1;
}
/* Basically, it's for new cards.
 * Without interrupting system, 
 * saves new card's UID to SD.
 * After Admin Card read by system,
 * this function prints new Cards UID,
 * if it reads any card within 3 sec.
 */
void card_register(String rfid) {
  //Serial.println("Register Function");
  successRead = 0;
  /* Basic do-while, no need to register Admin Card to SD 
   * If a new PICC placed to RFID reader continue,
   * else try to catch another one within 3 sec.
   */
  do {
    Serial.println("");
    delay(3000);
    //get New Cards UID after 3 seconds.
    successRead = getID();
  }
  while (!successRead && rfid == "9406C6D5" && rfid == "94 06 C6 D5");
 
  
  if (successRead) {
    
    String content = "";

    for ( int i = 0; i < mfrc522.uid.size; i++ )
    {
      content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : ""));
      content.concat(String(mfrc522.uid.uidByte[i], HEX));
    }
    content.toUpperCase();
    content.trim();
    
    Serial.print("This UID will be written to SD: ");
    Serial.println(content);
    // Give this new Card UID to SD
    write_sd(content);
  }
  Serial.println("New Card Registered To SD Card.");
  Serial.println();
}

void write_sd(String content) {
  select_sd();
 
  myFile = SD.open("test.txt", FILE_WRITE);
    if (!myFile) {
      // Failed to open file, so give up.
      Serial.println("Failed to open file.txt");
    }
  if (myFile) {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  SD.begin(CS_pin);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  pinMode(SS_PIN, OUTPUT);
  digitalWrite(CS_pin, LOW);
  pinMode(ledPin, OUTPUT);
  /* After some master-slave config, 
   * Let's Write to SD.
   */
    myFile.println(content);
    delay(1000);
    myFile.close();
    
    Serial.println("This value has succesfully written to SD: ");
    Serial.println(content);
    Serial.println();
  }
  else {
    Serial.println("Couldn't Access File");
  }
  delay(3000);
  myFile.close();
}
/* Select_Slave utility function 1, USE SD ! */
void select_sd() {
  pinMode(SS_PIN, HIGH);
  delay(200);
  digitalWrite(CS_pin, LOW);
  delay(200);
}
/* Select_Slave utility function 2, USE RFID ! */

void select_rfid() {
  pinMode(SS_PIN, LOW);
  delay(200);
  digitalWrite(CS_pin, HIGH);
  delay(200);
}
void time_sd(String content){
  select_sd();
 // delay(1000);
  myFile = SD.open("logs.txt", FILE_WRITE);
  Serial.println(myFile);
  if (myFile) {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  SD.begin(CS_pin);
  while (!Serial) {
    ; // Waits for serial port to connect. Needed for native USB port only
  }
  pinMode(SS_PIN, OUTPUT);
  digitalWrite(CS_pin, LOW);
  pinMode(ledPin, OUTPUT);
  /* Now, it writes time info to sd*/
    myFile.println(content);
    DateTime now = RTC.now(); 
    myFile.print(now.month(), DEC);
    myFile.print('/');
    myFile.print(now.day(), DEC);
    myFile.print('/');
    myFile.print(now.year(), DEC);
    myFile.print(' ');
    myFile.print(now.hour(), DEC);
    myFile.print(':');
    myFile.print(now.minute(), DEC);
    myFile.print(':');
    myFile.print(now.second(), DEC);
    myFile.println();    
    delay(300);//Delay to save properly
    myFile.close();
//  Serial.println(content);
//  Might be useful for debug purposes
    Serial.println();
    Serial.println("Time info has succesfully written to SD Card.");
  }
  else {
    Serial.println("Couldn't Access File");
  }
  delay(500);
  myFile.close();
}

   
