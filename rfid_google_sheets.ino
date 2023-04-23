/*Esther Ndegwa Security system with data logging
This code is designed to send registered users data to a google sheets file and captures unregistered users but doesnt grant them access*/
#include <SPI.h>
#include <MFRC522.h>      //rfid tag library
#include <Arduino.h>
#include <ESP8266WiFi.h>  //nodemcu library
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>   // wifi library
#include <WiFiClientSecureBearSSL.h>
#include <BlynkSimpleEsp8266.h>// blynk phone notification library
//-----------------------------------------
#define RST_PIN  D3 //RFID tag  reset pin declaration
#define SS_PIN   D4
#define RELAY   D2// Relay pin to controll solenoid lock
//-----------------------------------------
MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;  
MFRC522::StatusCode status;      
//-----------------------------------------
/* Be aware of Sector Trailer Blocks */
int blockNum = 2;  
/* Create another array to read data from Block */
/* Legth of buffer should be 2 Bytes more than the size of Block (16 Bytes) */
byte bufferLen = 18;
byte readBlockData[18];
//-----------------------------------------


// this the auth token to link project to blynk app which is used in remote monitoring
char auth[] ="HuKwaMGdtgpLM7COsT7G6WCemLu-zJwr";



// the widget terminal in blynk app is the terminal with which data is displayed on mobile devices
WidgetTerminal terminal(V5);
String card_holder_name;
// sheet url is the link to the google sheets slide where data or attendance is monitored
const String sheet_url = "https://script.google.com/macros/s/AKfycbzy8H8LHF00AoHD6i3JogCQHO4gEzjkOIC89m2tzFft6m7Kw0UNAzvbUDJ07nF3Hiwp/exec?name=";

// Fingerprint for demo URL, expires on ‎Monday, ‎May ‎29, ‎2023 7:20:58 AM, needs to be updated well before this date
const uint8_t fingerprint[20] = {0xAC, 0xC1, 0x30, 0x63, 0x91, 0x6B, 0xDC, 0x79, 0xD3, 0xC8, 0x51, 0x9E, 0x9A, 0xE7, 0x8F, 0xEF, 0xDC, 0xA6, 0xC5, 0xC6};
//AC C1 30 63 91 6B DC 79 D3 C8 51 9E 9A E7 8F EF DC A6 C5 C6
//-----------------------------------------


#define WIFI_SSID "TROJAN"//change this with your wifi name or ssid
#define WIFI_PASSWORD "Alex@7402"//change this with your wifi password
//-----------------------------------------




/****************************************************************************************************
 * setup() function code designed to run only once unless reset
 ****************************************************************************************************/
void setup()
{
  //--------------------------------------------------
  /* Initialize serial communications with the PC */
  Serial.begin(9600);
  //------ start connection with blynk app
   Blynk.begin(auth, WIFI_SSID, WIFI_PASSWORD);
  //Serial.setDebugOutput(true);
  //--------------------------------------------------
  //WiFi Connectivity
  Serial.println();
  Serial.print("Connecting to AP");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(200);
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  //--------------------------------------------------
  /* Set Relay/solenoid as OUTPUT */
  pinMode(RELAY, OUTPUT);
  //--------------------------------------------------
  /* Initialize SPI bus */
  SPI.begin();
  //--------------------------------------------------
}




/****************************************************************************************************
 * loop() function
 ****************************************************************************************************/
 void loop()
{
  Blynk.run();
  //--------------------------------------------------
  /* Initialize MFRC522 Module */
  mfrc522.PCD_Init();
  
  /* Look for new cards */
  /* Reset the loop if no new card is present on RC522 Reader */
  if ( ! mfrc522.PICC_IsNewCardPresent()) {return;}
  /* Select one of the cards */
  if ( ! mfrc522.PICC_ReadCardSerial()) {return;}
  /* Read data from the same block */
  //--------------------------------------------------
  Serial.println();
  Serial.println(F("Reading last data from RFID..."));
  ReadDataFromBlock(blockNum, readBlockData);
  /* If you want to print the full memory dump, uncomment the next line */
  //mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
  
  /* Print the data read from block */
  Serial.println();
  Serial.print(F("Last data in RFID:"));
  Serial.print(blockNum);
  Serial.print(F(" --> "));
  for (int j=0 ; j<16 ; j++)
  {
    Serial.write(readBlockData[j]);
  }
  Serial.println();
  //--------------------------------------------------
  digitalWrite(RELAY, HIGH);
 
  
  //--------------------------------------------------
  
  //---------------------------------------------------------------------------------
  if (WiFi.status() == WL_CONNECTED) {
    //-------------------------------------------------------------------------------
    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
    //-------------------------------------------------------------------------------
    client->setFingerprint(fingerprint);
    // Or, if you want to ignore the SSL certificate
    //then use the following line instead:
    // client->setInsecure();
    //-----------------------------------------------------------------
    card_holder_name = sheet_url + String((char*)readBlockData);
    card_holder_name.trim();
    Serial.println(card_holder_name);
    //-----------------------------------------------------------------
    HTTPClient https;
    Serial.print(F("[HTTPS] begin...\n"));
    //-----------------------------------------------------------------

    //NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
    if (https.begin(*client, (String)card_holder_name)){
      //-----------------------------------------------------------------
      // HTTP
      Serial.print(F("[HTTPS] GET...\n"));
      // start connection and send HTTP header
      int httpCode = https.GET();
      //-----------------------------------------------------------------
      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
        Serial.println("essy");   //Enter User1 Name
         Blynk.virtualWrite(V5, "Essy has accessed the premises a moment ago" );
          delay(1000);
          digitalWrite(RELAY, LOW);
          delay(5000);
          digitalWrite(RELAY, HIGH);
          delay(5000);
          digitalWrite(RELAY, LOW);
        // file found at server
      }
      //-----------------------------------------------------------------
      else 
      {Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());}
      //-----------------------------------------------------------------
      https.end();
      delay(1000);
    }
    //NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
    else {
      Serial.printf("[HTTPS} Unable to connect\n");
    }
    //NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
  }
  //MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
}




/****************************************************************************************************
 * ReadDataFromBlock() function
 ****************************************************************************************************/
void ReadDataFromBlock(int blockNum, byte readBlockData[]) 
{ 
  //----------------------------------------------------------------------------
  /* Prepare the ksy for authentication */
  /* All keys are set to FFFFFFFFFFFFh at chip delivery from the factory */
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  //----------------------------------------------------------------------------
  /* Authenticating the desired data block for Read access using Key A */
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
  //----------------------------------------------------------------------------s
  if (status != MFRC522::STATUS_OK){
     Serial.print("Authentication failed for Read: ");
     Serial.println(mfrc522.GetStatusCodeName(status));
     return;
  }
  //----------------------------------------------------------------------------
  else {
    Serial.println("Authentication success");
  }
  //----------------------------------------------------------------------------
  /* Reading data from the Block */
  status = mfrc522.MIFARE_Read(blockNum, readBlockData, &bufferLen);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Reading failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  //----------------------------------------------------------------------------
  else {
    Serial.println("Block was read successfully");  
  }
  //----------------------------------------------------------------------------
}
