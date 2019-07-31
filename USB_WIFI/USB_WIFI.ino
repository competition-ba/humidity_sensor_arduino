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
SoftwareSerial mySerial(2,3);//TX:3;RX:2
char* format="{\"GUID\":\"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\",\"DATA\":\"";
char ipdump[16];
void longdelay(int m);//长时间暂停
void setup() {
    
    String ssid,pwd;
    int i;
    byte ch;
    //初始化ESP8266 WIFI模块通信串口
    Serial.begin(9600);
    mySerial.begin(9600);
    //将设备GUID填入data中
    for (int i=0x00;i<0x20;i++)
    format[i+9] = (char)EEPROM.read(i);
    //如果没有初始化配置，则进入初始化模式
    if(!(EEPROM.read(0x7F)==0x01))
    initVal();
    //读取IP地址
    sprintf(ipdump,"%d.%d.%d.%d",EEPROM.read(0x20),EEPROM.read(0x21),EEPROM.read(0x22),EEPROM.read(0x23));
    //初始化WIFI模块
    mySerial.print("AT+RST\r\n");   // 初始化重启一次esp8266
    delay(5000);
    mySerial.print("AT\r\n");
    delay(500);
    mySerial.print("AT+CWMODE=3\r\n");  // 设置Wi-Fi模式
    i = 0x24;
    while((ch=EEPROM.read(i++))!=0xFE)
    ssid += (char)ch;
    i = i;
    while((ch=EEPROM.read(i++))!=0xFE)
    pwd += (char)ch;    
    mySerial.print("AT+CWJAP=\""+ssid+"\",\""+pwd+"\"\r\n");// 连接Wi-Fi
    delay(10000);
}

void loop() {
    // put your main code here, to run repeatedly:
    uploadData();
    longdelay(10);//10min更新一次
}
void initVal(){
    while(!Serial);
    //等待设备上线
    while(Serial.available()==0);   
    Serial.read();
    //发送GUID
    for(int i=0x00;i<0x20;i++)
    Serial.print((char)EEPROM.read(i));
    //Serial.println();
    //接受相关信息
    char ch;
    int i=0x20;
    //等待返回IP
    while(Serial.available()==0);
    for(i;i<0x24;i){
        while(Serial.available()!=0){
            EEPROM.write(i++,(char)Serial.read());
        }
    }
    #ifdef DEBUG
        Serial.println("target IP OK,received:");
        for(int j=0x20;j<0x24;j++)
            Serial.println(EEPROM.read(j));
    #endif
    i = 0x24;
    while(1){
        if(Serial.available()>0){
            ch = Serial.read();
            EEPROM.write(i++,ch);
        }
        if(ch==' ')
            break;
    }
    EEPROM.write(i-1,0xFE);
    #ifdef DEBUG
        Serial.println("SSID OK,Waiting for PWD");
    #endif
    i++;
    while(1){
        if(Serial.available()>0){
            ch = Serial.read();
            EEPROM.write(i++,ch);
        }
        if(ch==' ')
            break;
    }
    EEPROM.write(i-1,0xFE);
    //给出配置完成flag
    EEPROM.write(0x7F,0x01);
    #ifdef DEBUG
        Serial.println("ALL ARE OK");
    #endif
    out:;
}
void uploadData(){
    /*JSON数据：
     * {"GUID":"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX","DATA":"XX.XX"}
     */
    mySerial.print("AT+CIPMODE=1\r\n");
    mySerial.print("AT+CIPSTART=\"TCP\",\"");  // 连接服务器的8080端口
    mySerial.print(ipdump);
    mySerial.print("\",8080\r\n");
    delay(1000);
    mySerial.print("AT+CIPSEND\r\n"); // 进入TCP透传模式，接下来发送的所有消息都会发送给服务器
    mySerial.print("POST /Arduino/update.jsp"); // 开始发送post请求
    mySerial.print(" HTTP/1.1\r\nContent-Type: application/json;charset=utf-8\r\nHost: "); // post请求的报文格式 
    mySerial.print(ipdump);
    mySerial.print("\r\nUser-Agent: arduino-ethernet\r\nConnection:close\r\nContent-Length:58\r\n\r\n");
    mySerial.print(format);
    mySerial.print(20.31231,2);
    mySerial.print("\"}");// 结束post请求
    delay(3000);
    mySerial.print("+++"); 
}
void longdelay(int m){
    int s=m*60;
    for(int i=0;i<s;i++)
        delay(989);
}
