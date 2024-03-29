#include <DHT.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>
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
    当系统引导时，若为低电位（下拉），则触发"清除配置"动作
 */
#define CONFIG_RST 13
//定义湿度传感器针脚
#define DHTPIN 10 
//定义湿度传感器类型
#define DHTTYPE DHT22
/*
 *规定气体传感器的针脚位置。
 *气体传感器通过模拟（Analog）方式与Arduino交换数据。
 */
#define MQSENSOR A2
//是否开启调试模式
#define DEBUG
SoftwareSerial mySerial(2,3);//TX:3;RX:2
char* format="{\"GUID\":\"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\",\"DATA\":\"";
char ipdump[16];
void longdelay(int m);//长时间暂停
DHT dht(DHTPIN, DHTTYPE);
void setup() {
    String ssid,pwd;
    int i;
    byte ch;
    pinMode(LED_R_PIN, OUTPUT);
    pinMode(LED_R_PIN+1, OUTPUT);
    pinMode(LED_R_PIN+2, OUTPUT);
    pinMode(CONFIG_RST, INPUT);
    //因为我们现在还不清楚是否完成了配置，假设其已经配置完成
    setStatus(LED_CONFIGURE_DONE);
    //初始化ESP8266 WIFI模块通信串口
    Serial.begin(9600);
    mySerial.begin(9600);
    dht.begin();
    //将设备GUID填入data中
    for (int i=0x00;i<0x20;i++)
    format[i+9] = (char)EEPROM.read(i);
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

void loop() {
    setStatus(LED_UPLOADING_DATA);
    uploadData();
    
    Serial.print("Done send.\r\n");
    longdelay(1);//1min更新一次
}
void initVal(){
    setStatus(LED_NOT_CONFIGURED);
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
    int i;
    while(Serial.available()==0);
    i = 0x24;
    while(1){
        if(Serial.available()>0){
            ch = Serial.read();
            EEPROM.write(i++,ch);
        }
        if(ch==0x00)
            break;
    }
    EEPROM.write(i-1,0xFE);
    #ifdef DEBUG
        Serial.println("SSID OK,Waiting for PWD");
    #endif
    while(1){
        if(Serial.available()>0){
            ch = Serial.read();
            EEPROM.write(i++,ch);
        }
        if(ch==0x00)
            break;
    }
    EEPROM.write(i-1,0xFE);
    i = 0x20;
    for(i;i<0x24;i++){
        while(!Serial.available());
        ch = Serial.read();
        EEPROM.write(i,ch);    
    }
    //给出配置完成flag
    EEPROM.write(0x7F,0x01);
    #ifdef DEBUG
        Serial.println("ALL ARE OK");
    #endif
    setStatus(LED_CONFIGURE_DONE);
}
void uploadData(){
    /*JSON数据：
     * {"GUID":"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX","DATA":"XX.XX"}
     */
    /*检测WiFi是否已经连接。
     * 使用AP指令AT+CWJAP?检测是否连接。
     * 当连接时，返回的第一位为字符'+'。
     * 未连接时，返回的第一位为字符'N'(NO AP)。
     */
    String tmp;
    begin:delay(1500);
    //首先，我们清空缓冲区。
    while(mySerial.available()>0)
       mySerial.read();
    mySerial.print("AT+CWJAP?\r\n");
    delay(1500);
    //跳过自身的指令
    for(int i=0;i<=11;i++)
        mySerial.read();
    char ch=(char)(mySerial.read());
    #ifdef DEBUG
    Serial.print("Status character is:");
    Serial.print(ch);
    tmp="";
    while(mySerial.available()>0)
       tmp += (char)(mySerial.read());
    Serial.print(tmp);
    Serial.print("\r\n");
    #endif
    //如果依然未连接，则在停顿一段时间后重新查询
    if(ch=='b'){
        goto begin;
    }
    if(ch=='N'){
        setStatus(LED_CONNECT_FAILED);
        return;
    }
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
    mySerial.print("POST /Arduino/SenData"); // 开始发送post请求
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
    Serial.println(analogRead(MQSENSOR));
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
    setStatus(LED_WIFI_CONNECTED);
}
void longdelay(int m){
    int s=m*60;
    for(int i=0;i<s;i++)
        delay(989);
}
