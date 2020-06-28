
#include "Arduino.h"
#include "DFRobotDFPlayerMini.h"
#include "SoftwareSerial.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include <Wire.h>  
#include "SSD1306Wire.h"

char ssid[] = "tiger";
char password[] = "huafeixue107";
const char *mqtt_server = "192.168.191.1";
const int mqtt_port = 1883;

const char* topic_name = "LED";//订阅的主题
const char* client_id = "ae86cli";//尽量保持唯一，相同的id连接会被替代

WiFiClient espClient;
PubSubClient client(espClient);

// for 128x64 displays:
SSD1306Wire display(0x3c, D6, D5);  // ADDRESS, SDA, SCL
String oled_center("I'm AE86.");
String oled_mline;
String oled_mode("center");

// D5 is RX of ESP8266, connect to TX of DFPlayer
// D6 is TX of ESP8266, connect to RX of DFPlayer module
SoftwareSerial softSerial(D1, D2); //TX, RX

DFRobotDFPlayerMini myDFPlayer;
void printDetail(uint8_t type, int value);

enum led_mode {
  LED_OFF,
  LED_ON,
  LED_FADE,
};
int light = 1;
int light_step = 1;
unsigned int light_intv = 8;
int back_led_mode = LED_FADE;
int front_led_mode = LED_ON;



//初始化WIFI
void setup_wifi() {//自动连WIFI接入网络
  delay(10);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("...");
    
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String callMsg = "";
  Serial.print("Message arrived [");
  Serial.print(topic);   // 打印主题信息
  Serial.print("] ");

  for (int i = 0; i < length; i++) {
    callMsg += char(payload[i]);
  }
  
  Serial.println(callMsg);
  String cmd = topic;
  if (cmd.equals("LED")){
    //如果返回ON关闭LED
    if(callMsg.equals("ON")){
      digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
    }
    //如果返回OFF关闭LED
    if(callMsg.equals("OFF")){
      digitalWrite(LED_BUILTIN, HIGH);   // Turn the LED off by making the voltage HIGH
    }
  }
  else if (cmd.equals("front_led")){
    int n = callMsg.toInt();
    front_led_mode = n;
    if (front_led_mode == LED_OFF)
      analogWrite(D7, 0);
    else if (front_led_mode == LED_ON)
      analogWrite(D7, 250);
  }
  else if (cmd.equals("back_led")){
    int n = callMsg.toInt();
    back_led_mode = n;
    if (back_led_mode == LED_OFF)
      analogWrite(D3, 0);
    else if (back_led_mode == LED_ON)
      analogWrite(D3, 250);
  }
  else if (cmd.equals("mp3")){
    if(callMsg.equals("stop")){
      myDFPlayer.stop();
    }
    else if(callMsg.equals("next")){
      myDFPlayer.next();
    }
  }
  else if (cmd.equals("mp3play")){
    int n = callMsg.toInt();
    if (n>0)
      myDFPlayer.play(n);
  }
  else if (cmd.equals("mp3vol")){
     int n = callMsg.toInt();
      if (n>=0)
        myDFPlayer.volume(n);
  }
  else if (cmd.equals("oled_center")){
     oled_center = callMsg;
     oled_mode = "center";
  }
  else if (cmd.equals("oled_mline")){
     oled_mline = callMsg;
     oled_mode = "mline";
  }
  else if (cmd.equals("light_intv")){
     int n = callMsg.toInt();
      if (n>0)
        light_intv = n;
  }
}
void reconnect() {//等待，直到连接上服务器
  // while (!client.connected()) {//如果没有连接上
  if (!client.connected()) {//如果没有连接上
    if (client.connect(client_id)) {//接入时的用户名，尽量取一个很不常用的用户名
      client.subscribe(topic_name);//接收外来的数据时的intopic
      client.subscribe("front_led");
      client.subscribe("back_led");
      client.subscribe("mp3");
      client.subscribe("mp3play");
      client.subscribe("mp3vol");
      client.subscribe("oled_center");
      client.subscribe("oled_mline");
      client.subscribe("light_intv");
      Serial.println("connect success");
    } else {
      Serial.print("mqtt connect failed, rc=");//连接失败
      Serial.print(client.state());//重新连接
      // Serial.println(" try again in 5 seconds");//延时5秒后重新连接
      // delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  softSerial.begin(9600);
  // Blynk.begin()
  // Blynk.begin(softSerial, ssid, pass, auth);
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  digitalWrite(LED_BUILTIN, HIGH);
  pinMode(D3, OUTPUT);
  analogWrite(D3, 0);
  // digitalWrite(D3, LOW);
  pinMode(D7, OUTPUT);
  analogWrite(D7, 250);
  
  setup_wifi();//自动连WIFI接入网络
  client.setServer(mqtt_server, mqtt_port);//端口号
  client.setCallback(callback); //用于接收服务器接收的数据
  client.publish("LED", "hello mqtt");

  Serial.println();
  Serial.println(F("DFRobot DFPlayer Mini Demo"));
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
  
  if (!myDFPlayer.begin(softSerial)) {  //Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true){
      delay(0); // Code to compatible with ESP8266 watch dog.
    }
  }
  Serial.println(F("DFPlayer Mini online."));
  
  myDFPlayer.volume(20);  //Set volume value. From 0 to 30
 
  // initialize dispaly
 // Initialising the UI will init the display too.
  display.init();

  // display.flipScreenVertically();
  display.setFont(ArialMT_Plain_16);

}

void loop()
{
  reconnect();//确保连上服务器，否则一直等待。
  client.loop();//MUC接收数据的主循环函数
  static unsigned long timer = millis();
  if (back_led_mode == LED_FADE || front_led_mode == LED_FADE){
    if (millis() - timer > light_intv){
      timer = millis();
      light += light_step;
    }

    if (light>=255 || light<=0)
      light_step = -light_step;
    if (back_led_mode == LED_FADE)
      analogWrite(D3, light);
    if (front_led_mode == LED_FADE)
      analogWrite(D7, light);
  }

  // for (int a=0; a<=255;a++)                /*循环语句，控制PWM亮度的增加*/
  // {
  //   analogWrite(D3,a);
  //   delay(8);                             /*当前亮度级别维持的时间,单位毫秒*/            
  // }
  
  // for (int a=255; a>=0;a--)             /*循环语句，控制PWM亮度减小*/
  // {
  //   analogWrite(D3,a);
  //   delay(8);                             /*当前亮度的维持的时间,单位毫秒*/  
  // }

  display.clear();
  if (oled_mode=="center"){
    display.setFont(ArialMT_Plain_16);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 22, oled_center);
  }
  else if (oled_mode == "mline") {
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawStringMaxWidth(0, 0, 128, oled_mline);
  }

  display.display();

  if (myDFPlayer.available()) {
    printDetail(myDFPlayer.readType(), myDFPlayer.read()); //Print the detail message from DFPlayer to handle different errors and states.
  }
}

void printDetail(uint8_t type, int value){
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerUSBInserted:
      Serial.println("USB Inserted!");
      break;
    case DFPlayerUSBRemoved:
      Serial.println("USB Removed!");
      break;
    case DFPlayerPlayFinished:
      // myDFPlayer.next();
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
  
}
