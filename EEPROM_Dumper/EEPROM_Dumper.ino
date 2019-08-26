#include <EEPROM.h>
void setup() {
    Serial.begin(9600);
    byte a;
  // put your setup code here, to run once:
    for(int i=0;i<=0x7F;i++)
    {
        if(!(i%8))
            Serial.println();
        Serial.print("0x");
        a=EEPROM.read(i);
        if(a<0x0F)
            Serial.print(0);
        Serial.print(EEPROM.read(i),HEX);
        Serial.print(" ");
    }
}

void loop() {
  // put your main code here, to run repeatedly:

}
