  #include <EEPROM.h>
  /*
  EEPROM布局定义：
  GUID:00-0x1F
  Target IP:0x20~0x23
  Wifi Name/Pwd:0x24-0x7F
    Name(0xFE)Pwd(0xFE)
  */
  //Test GUID:2890014B-C5CD-42BA-879B-5F2C83E7A270
  //Test IP:47.103.1.240
void setup() {
  Serial.begin(9600);
  fdevopen( &serial_putc, 0 );
}
int serial_putc( char c, struct __file * )
{
  Serial.write( c );
  return c;
}
void write(){
  char* guid="2890014BC5CD42BA879B5F2C83E7A270";
  unsigned char* ip[]={47,103,1,240};
  for(int i=0;i<=0x1F;i+=1)
    EEPROM.write(i,*(guid+i));
  for(int i=0;i<=3;i+=1)
    EEPROM.write(i+0x20,*(ip+i));
  byte* usernamepwd="fhh 20000412fhh ";
  for(int i=0;i<strlen(usernamepwd);i++){
    if(usernamepwd[i]==' ')
        usernamepwd[i]=0xFE;
  }
    for(int i=0;i<strlen(usernamepwd);i+=1)
      EEPROM.write(i+0x24,*(usernamepwd+i));
    EEPROM.write(0x7F,0x01);
  Serial.println("Done writing");  
}
void loop() {
  write();
  while(!Serial);
  read();
  while(1);
}
void read(){
  Serial.print("Device GUID:\n");
  for(int i=0;i<=0x01F;i++)
  {
    Serial.print((char)EEPROM.read(i));
    if(i==7||i==11||i==15||i==19)
    Serial.print('-');
  }
  Serial.print("\n");
  Serial.print("Target IP:\n");
  printf("%d.%d.%d.%d\n",\
          EEPROM.read(0x20),
          EEPROM.read(0x21),
          EEPROM.read(0x22),
          EEPROM.read(0x23));
}
