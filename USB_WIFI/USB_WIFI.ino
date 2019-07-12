/*
    EEPROM布局定义：
    GUID:00-0x1F
    Target IP:0x20~0x23
    Wifi Name/Pwd:0x24-0x7E
    Name(0xFE)Pwd(0xFE)
    配置完成flag:0x7F
    0x01:完成
 */
#include<EEPROM.h>
#include <SoftwareSerial.h>
//是否开启调试模式
#define DEBUG
SoftwareSerial mySerial(12, 11);//TX:12;RX:11
char* format="{\"GUID\":\"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\",\"DATA\":\"";
char ipdump[16];
void setup() {
    String ssid,pwd;
    int i;
    char ch;
    //初始化ESP8266 WIFI模块通信串口
    Serial.begin(9600);
    mySerial.begin(115200);
    //将设备GUID填入data中
    for (int i=0x00;i<0x20;i++)
        format[i+9] = (char)EEPROM.read(i);
    //如果没有初始化配置，则进入初始化模式
    if(!(EEPROM.read(0x7F)==0x01))
        initVal();
    //读取IP地址
    sprintf(ipdump,"%d.%d.%d.%d",EEPROM.read(0x20),EEPROM.read(0x21),EEPROM.read(0x22),EEPROM.read(0x23));
    //初始化WIFI模块
    mySerial.println("AT+RST");   // 初始化重启一次esp8266
    delay(1500);
    mySerial.println("AT");
    delay(500);
    mySerial.println("AT+CWMODE=3");  // 设置Wi-Fi模式
    i = 0x24;
    while((ch=(char)EEPROM.read(i++))!=0xFE)
        ssid += ch;
    i = i;
    while((ch=(char)EEPROM.read(i++))!=0xFE)
        pwd += ch;        
    mySerial.print("AT+CWJAP=\"");
    mySerial.print(ssid);
    mySerial.print("\",\"");
    mySerial.print( pwd);
    mySerial.print( "\"");// 连接Wi-Fi
    while (mySerial.available()) {
        Serial.write(mySerial.read());
    }
    delay(10000);
}

void loop() {
    // put your main code here, to run repeatedly:
    uploadData();
    delay(1000*60*10);//10min更新一次
}
void initVal(){
    while (!Serial) {
        ; // wait for serial port to connect. Needed for native USB port only
    }
    while(1){
        //发送GUID
        for(int i=0x00;i<0x20;i++)
            Serial.print((char)EEPROM.read(i));
        Serial.println();
        //接受相关信息
        char ch;
        int i;
        delay(1000);
        #ifdef DEBUG
        Serial.print(Serial.available());
        #endif
        while(Serial.available()>0){
            for(i=0x20;i<0x24;i++)//Target IP
                EEPROM.write(i,(char)Serial.read());
            #ifdef DEBUG
            Serial.println("target IP OK,received:"+EEPROM.read(0x20)+EEPROM.read(0x21)+EEPROM.read(0x22)+EEPROM.read(0x23));
            #endif
           while((ch=(char)Serial.read())!=0xFE)//SSID
                EEPROM.write(i++,ch);
            EEPROM.write(i,0xFE);
            #ifdef DEBUG
            Serial.println("SSID OK,Waiting for PWD");
            #endif
            while((ch=(char)Serial.read())!=0xFE)//PWD
                EEPROM.write(i++,ch);
            EEPROM.write(i,0xFE);
            //给出配置完成flag
            EEPROM.write(0x7F,0x01);
            #ifdef DEBUG
            Serial.println("ALL ARE OK");
            #endif
            goto out;
        }
    }
    out:;
}
void uploadData(){
    /*JSON数据：
     * {"GUID":"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX","DATA":"XX.XX"}
     */
    mySerial.println("AT+CIPMODE=1");
    mySerial.print("AT+CIPSTART=\"TCP\",\"");  // 连接服务器的80端口
    mySerial.print(ipdump);
    mySerial.println("\",80");
    delay(1000);
    mySerial.println("AT+CIPSEND"); // 进入TCP透传模式，接下来发送的所有消息都会发送给服务器
    mySerial.print("POST /update.jsp"); // 开始发送post请求
    mySerial.print(" HTTP/1.1\r\nHost: "); // post请求的报文格式 
    mySerial.print(ipdump);
    mySerial.print("\r\nUser-Agent: arduino-ethernet\r\nConnection:close\r\nContent-Length:58\r\n\r\n");
    mySerial.print(format);
    mySerial.print(20.31231,2);
    mySerial.println("\"}");// 结束post请求
    delay(3000);
    mySerial.print("+++"); 
}
