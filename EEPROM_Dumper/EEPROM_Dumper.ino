#include <EEPROM.h>
void setup() {
    Serial.begin(9600);
  // put your setup code here, to run once:
    for(int i=0;i<=0x7F;i++)
    {
        Serial.print((char)EEPROM.read(i));
    }
}

void loop() {
  // put your main code here, to run repeatedly:

}
