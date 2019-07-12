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
SoftwareSerial mySerial(12, 11);//TX:12;RX:11
char* format="{\"GUID\":\"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\",\"DATA\":\"%2.2f\"}";
char ipdump[16];
void setup() {
    String ssid,pwd;
    int i;
    char ch;
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
    mySerial.begin(115200);
    mySerial.println("AT+RST");   // 初始化重启一次esp8266
    delay(1500);
    mySerial.println("AT");
    delay(500);
    mySerial.println("AT+CWMODE=3");  // 设置Wi-Fi模式
    i = 0x24;
    while(ch=EEPROM.read(i++)!=0xFE)
        ssid += ch;
    while(ch=EEPROM.read(i++)!=0xFE)
        pwd += ch;        
    mySerial.println("AT+CWJAP=\""+ssid+"\",\""+pwd+"\"");  // 连接Wi-Fi
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
            Serial.print(EEPROM.read(i));
        //接受相关信息
        char ch;
        int i;
        delay(100);
        while(Serial.available()>0){
            for(i=0x20;i<0x24;i++)//Target IP
                EEPROM.write(i,Serial.read());
            while(ch=Serial.read()!=0xFE)//SSID
                EEPROM.write(i++,ch);
            EEPROM.write(++i,0xFE);
            while(ch=Serial.read()!=0xFE)//PWD
                EEPROM.write(i++,ch);
            //给出配置完成flag
            EEPROM.write(0x7F,0x01);
            goto out;
        }
    }
    out:;
}
void uploadData(){
    /*JSON数据：
     * {"GUID":"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX","DATA":"XX.XX"}
     */
    char data[59];
    sprintf(data,format,0.00);
    mySerial.println("AT+CIPMODE=1");
    mySerial.print("AT+CIPSTART=\"TCP\",\"");  // 连接服务器的80端口
    mySerial.print(ipdump);
    mySerial.println("\",80");
    delay(1000);
    mySerial.println("AT+CIPSEND"); // 进入TCP透传模式，接下来发送的所有消息都会发送给服务器
    mySerial.print("POST /update.jsp"); // 开始发送post请求
    mySerial.print(" HTTP/1.1\r\nHost: "); // post请求的报文格式 
    mySerial.print(ipdump);
    mySerial.print("\r\nUser-Agent: arduino-ethernet\r\nConnection:close\r\nContent-Length:59\r\n\r\n");
    mySerial.println(data); // 结束post请求
    delay(3000);
    mySerial.print("+++"); 
}
