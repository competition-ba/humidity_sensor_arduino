#include <SPI.h>
#include <PN532_SPI.h>
#include <PN532Interface.h>
#include <PN532.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <DHT.h>
/*
    EEPROM布局定义：
    GUID:00-0x1F
    Target IP:0x20~0x23
    Wifi Name/Pwd:0x24-0x7E
    Name(0xFE)Pwd(0xFE)
    配置完成flag:0x7F
    0x01:完成
 */
/*
    规定LED的颜色。
    LED显示颜色值按照“按位运算”的方式确定。
 */
#define LED_RED 1
#define LED_GREEN 2
#define LED_BLUE 4
//由于R/G/B针脚相连，只需定义R针脚即可
#define LED_R_PIN 6
/*
    规定LED状态指示灯的颜色。
    不同状态下的指示灯颜色不同。
 */
#define LED_NOT_CONFIGURED LED_RED
#define LED_CONFIGURE_DONE LED_RED+LED_GREEN
#define LED_CONNECT_FAILED LED_RED+LED_BLUE
#define LED_WIFI_CONNECTED LED_BLUE
#define LED_UPLOADING_DATA LED_GREEN
/*
     改变LED颜色
 */
void setStatus(char status){
    digitalWrite(LED_R_PIN  , (status&LED_RED  )   );
    digitalWrite(LED_R_PIN+1, (status&LED_GREEN)>>1);
    digitalWrite(LED_R_PIN+2, (status&LED_BLUE )>>2);
}
/*
    规定重置配置(RESET)键位的引脚值。
    当系统引导时，若为高电位（下拉），则触发"清除配置"动作
 */
#define CONFIG_RST 9
//定义湿度传感器针脚
#define DHTPIN 10 
//定义湿度传感器类型
#define DHTTYPE DHT22

//是否开启调试模式
#define DEBUG
SoftwareSerial mySerial(2,3);//TX:3;RX:2
char* format="{\"GUID\":\"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\",\"DATA\":\"";
char ipdump[16];
char GUID[0x20];
void longdelay(int m);//长时间暂停
void uploadData();//发送数据函数
PN532_SPI pn532spi(SPI, 10);
PN532 nfc(pn532spi);
DHT dht(DHTPIN, DHTTYPE);
void setup()
{   
    String ssid,pwd;
    int i;
    byte ch;
    pinMode(LED_R_PIN, OUTPUT);
    pinMode(LED_R_PIN+1, OUTPUT);
    pinMode(LED_R_PIN+2, OUTPUT);
    pinMode(CONFIG_RST, INPUT);
    mySerial.begin(9600); 
    setStatus(LED_CONFIGURE_DONE);
    #ifdef DEBUG
    Serial.begin(9600);
    Serial.println("-------Peer to Peer HCE--------");
    #endif
    nfc.begin();
    #ifdef DEBUG 
    uint32_t versiondata = nfc.getFirmwareVersion();
    if (! versiondata) {
      Serial.print("Didn't find PN53x board");
      while (1); // halt
    }
    // Got ok data, print it out!
    Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
    Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
    Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
    #endif
    // Set the max number of retry attempts to read from a card
    // This prevents us from waiting forever for a card, which is
    // the default behaviour of the PN532.
    nfc.setPassiveActivationRetries(0x05);
    
    // configure board to read RFID tags
    nfc.SAMConfig();
    for (int i=0x00;i<0x20;i++){
        format[i+9] = (char)EEPROM.read(i);
        GUID[i] = (char)EEPROM.read(i);
    }
    //如果触发了“重置”，则在此抹除“配置完成”标签，强迫进入配置模式
    if(digitalRead(CONFIG_RST))
        EEPROM.write(0x7F,0x00);
    //如果没有初始化配置，则进入初始化模式
    if(!(EEPROM.read(0x7F)==0x01))
        initVal();
    //读取IP地址
    sprintf(ipdump,"%d.%d.%d.%d",EEPROM.read(0x20),EEPROM.read(0x21),EEPROM.read(0x22),EEPROM.read(0x23));
    //初始化WIFI模块
    mySerial.print("AT+RST\r\n");   // 初始化重启一次esp8266
    delay(5000);
    #ifdef DEBUG
    Serial.print("AT+RST\r\n");
    while(mySerial.available()>0)
        Serial.print((char)mySerial.read());
    #endif
    mySerial.print("AT\r\n");
    delay(500);
    #ifdef DEBUG
    Serial.print("AT\r\n");
    while(mySerial.available()>0)
        Serial.print((char)mySerial.read());
    #endif
    mySerial.print("AT+CWMODE=3\r\n");  // 设置Wi-Fi模式
    delay(300);
    #ifdef DEBUG
    Serial.print("AT+CWMODE=3\r\n");
    delay(300);
    while(mySerial.available()>0)
        Serial.print((char)mySerial.read());
    #endif
    i = 0x24;
    while((ch=EEPROM.read(i++))!=0xFE)
    ssid += (char)ch;
    i = i;
    while((ch=EEPROM.read(i++))!=0xFE)
    pwd += (char)ch;    
    mySerial.print("AT+CWJAP=\""+ssid+"\",\""+pwd+"\"\r\n");// 连接Wi-Fi
    delay(10000);
    #ifdef DEBUG
    Serial.print("AT+CWJAP=\""+ssid+"\",\""+pwd+"\"\r\n");
    delay(300);
    while(mySerial.available()>0)
        Serial.print((char)mySerial.read());
    #endif
    Serial.print("Done Init.\r\n");
    setStatus(LED_WIFI_CONNECTED);
}

void loop()
{
    setStatus(LED_UPLOADING_DATA);
    uploadData();
    setStatus(LED_WIFI_CONNECTED);
    Serial.print("Done send.\r\n");
    longdelay(1);//1min更新一次
}
void getSsidPwd(char[]);
String respBuffer;
void initVal(){
    setStatus(LED_NOT_CONFIGURED);
    bool success;
  uint8_t responseLength = 128;
  #ifdef DEBUG
  Serial.println("Waiting for an ISO14443A card");
  #endif
  while(1){
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
                              
    uint8_t response[128];  
    success = nfc.inDataExchange(selectApdu, sizeof(selectApdu), response, &responseLength);
    if(success) {
      #ifdef DEBUG
      Serial.print("responseLength: "); Serial.println(responseLength);
      nfc.PrintHexChar(response, responseLength);
      #endif
       char* resp=GUID;
       int len=sizeof(GUID);
       char ch;
       int i;
      do {
        success = nfc.inDataExchange(resp,len,response, responseLength);
        if(success) {
          #ifdef DEBUG  
          nfc.PrintHexChar(response,responseLength);
          #endif
          //首位为0x00--->用户尚未准备好，跳过该轮
          if(response[0]==0x00)
                continue;
          //开始处理数据
          //-----------------------------------------------------------
            i = 0x24;
            int k=0;
            byte ch=0x01;
            while(ch){
                ch = response[k++];
                EEPROM.write(i++,ch);
            }
            EEPROM.write(i-1,0xFE);
            ch=0x01;
            while(ch){
                ch = response[k++];
                EEPROM.write(i++,ch);
            }
            EEPROM.write(i-1,0xFE);
            i=0x20;
            for(i;i<0x24;i++)
                EEPROM.write(i,response[k++]);
            //给出配置完成flag
            EEPROM.write(0x7F,0x01);
          //-----------------------------------------------------------
          //此时，返回消息，标识“我们已经好了”
          resp = "OK";
          len = 3;
          setStatus(LED_CONFIGURE_DONE);
          return;
         
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
}
void uploadData(){
    /*JSON数据：
     * {"GUID":"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX","DATA":"XX.XX"}
     */
    float result;
    mySerial.print("AT+CIPMODE=1\r\n");
    #ifdef DEBUG
    delay(300);
    Serial.print("AT+CIPMODE=1\r\n");
    while(mySerial.available()>0)
        Serial.print((char)mySerial.read());
    #endif
    delay(300);
    //清空缓冲区
    while(mySerial.available()>0)
       mySerial.read();
    mySerial.print("AT+CIPSTART=\"TCP\",\"");  // 连接服务器的8080端口
    mySerial.print(ipdump);
    mySerial.print("\",80\r\n");
    delay(2000);
    #ifdef DEBUG
    Serial.print("AT+CIPSTART=\"TCP\",\"");
    delay(300);
    while(mySerial.available()>0)
        Serial.print((char)mySerial.read());
    #endif
    mySerial.print("AT+CIPSEND\r\n"); // 进入TCP透传模式，接下来发送的所有消息都会发送给服务器
    delay(300);
    mySerial.print("POST /Arduino/update.jsp"); // 开始发送post请求
    delay(300);
    mySerial.print(" HTTP/1.1\r\nContent-Type: application/json;charset=utf-8\r\nHost: "); // post请求的报文格式 
    delay(300);
    mySerial.print(ipdump);
    delay(300);
    mySerial.print("\r\nUser-Agent: arduino-ethernet\r\nConnection:close\r\nContent-Length:58\r\n\r\n");
    delay(300);
    mySerial.print(format);
    delay(300);
    result=dht.readHumidity();
    mySerial.print(result,2);
    delay(300);
    mySerial.print("\"}");// 结束post请求
    delay(3000);
    mySerial.print("+++"); 
    #ifdef DEBUG
    Serial.print("+++"); 
    delay(300);
    while(mySerial.available()>0)
        Serial.print((char)mySerial.read());
    #endif
}   
void longdelay(int m){
    int s=m*60;
    for(int i=0;i<s;i++)
        delay(989);
}
