#include <SPI.h>
#include <PN532_SPI.h>
#include <PN532Interface.h>
#include <PN532.h>
#include <EEPROM.h>
PN532_SPI pn532spi(SPI, 10);
PN532 nfc(pn532spi);

char GUID[0x20];//GUID DATA
void setup()
{    
    Serial.begin(9600);
    Serial.println("-------Peer to Peer HCE--------");
    
    nfc.begin();
    
    uint32_t versiondata = nfc.getFirmwareVersion();
    if (! versiondata) {
      Serial.print("Didn't find PN53x board");
      while (1); // halt
    }
    
    // Got ok data, print it out!
    Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
    Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
    Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
    
    // Set the max number of retry attempts to read from a card
    // This prevents us from waiting forever for a card, which is
    // the default behaviour of the PN532.
    nfc.setPassiveActivationRetries(0x05);
    
    // configure board to read RFID tags
    nfc.SAMConfig();
    for(int i=0;i<0x20;i++)
        GUID[i] = EEPROM.read(i);
}

void loop()
{
  bool success;
  
  uint8_t responseLength = 32;
  
  Serial.println("Waiting for an ISO14443A card");
  
  // set shield to inListPassiveTarget
  success = nfc.inListPassiveTarget();

  if(success) {
   
     Serial.println("Found something!");
                  
    uint8_t selectApdu[] = { 0x00, /* CLA */
                              0xA4, /* INS */
                              0x04, /* P1  */
                              0x00, /* P2  */
                              0x07, /* Length of AID  */
                              0x4A,0x49,0x4E,0x47,0x53,0x41,0x49, /* "JINGSAI" */
                              0x00  /* Le  */ };
                              
    uint8_t response[32];  
     
    success = nfc.inDataExchange(selectApdu, sizeof(selectApdu), response, &responseLength);
    
    if(success) {
      
      Serial.print("responseLength: "); Serial.println(responseLength);
       
      nfc.PrintHexChar(response, responseLength);
      
      do {
        uint8_t back[32];
        uint8_t length = 32; 

        success = nfc.inDataExchange(GUID, sizeof(GUID), back, &length);
        
        if(success) {
         
          //Serial.print("responseLength: "); Serial.println(length);
           
          //nfc.PrintHexChar(back, length);
          getSsidPwd(back);
          
        }
        else {
          
          Serial.println("Broken connection?"); 
        }
      }
      while(success);
    }
    else {
     
      Serial.println("Failed sending SELECT AID"); 
    }
  }
  else {
   
    Serial.println("Didn't find anything!");
  }

  delay(1000);
}
void getSsidPwd(char[]);
String respBuffer;
void printResponse(uint8_t *response, uint8_t responseLength) {
  
   

    for (int i = 0; i < responseLength; i++) {
      
      if (response[i] < 0x10) 
        respBuffer = respBuffer + "0"; //Adds leading zeros if hex value is smaller than 0x10
      
      respBuffer = respBuffer + String(response[i], HEX) + " ";                        
    }

    Serial.print("response: "); Serial.println(respBuffer);
    Serial.println("begin.");
    
}

void setupNFC() {
 
  nfc.begin();
    
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  
  // configure board to read RFID tags
  nfc.SAMConfig(); 
}
void getSsidPwd(char in[]){
    if(in[0]!=0x00){
        String buffer = String(in);
        int separator=buffer.indexOf(0xFE);
        Serial.println("SSID:");
        Serial.println(buffer.substring(0,separator));
        Serial.println("Password:");
        Serial.println(buffer.substring(separator+1));
    }
    else
        Serial.println("Not a vaild config!");
}
