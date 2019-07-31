#include <EEPROM.h>
void setup() {
    Serial.begin(9600);
  // put your setup code here, to run once:
    for(int i=0;i<=0x7F;i++)
    {
        if(!(i%8))
            Serial.println();
        Serial.print("0x");
        Serial.print(EEPROM.read(i),HEX);
        Serial.print(" ");
    }
}

void loop() {
  // put your main code here, to run repeatedly:

}
