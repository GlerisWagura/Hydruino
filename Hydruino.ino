#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "SimpleTimer.h"
#include <LM35.h>
#include <EEPROM.h>
#include "GravityTDS.h"
#include "NewPing.h"
#include <SoftwareSerial.h>       //Software Serial library
//ESP8266
SoftwareSerial espSerial(5, 6);   //Pin 5 and 6 act as RX and TX.       
#define DEBUG true
String mySSID = "Wagura";       // WiFi SSID
String myPWD = "12345678"; // WiFi Password
String myAPI = "A17EYJMI59TG0IT1";   // API Key
String myHOST = "api.thingspeak.com";
String myPORT = "80";
String myFIELD = "field7"; 
int sendVal;

SimpleTimer timer;
NewPing sonar (11,10,20);
float calibration_value = 21.34 +0.25;
int phval = 0; 
unsigned long int avgval; 
int buffer_arr[10],temp;

float ph_act;

#define OLED_RESET     4
Adafruit_SSD1306 display(OLED_RESET);

#define LM35 A2
float tempr;

#define TdsSensorPin A1
GravityTDS gravityTds;
float temperature = 25,tdsValue = 0;

//Relays
int relay1=9;
int relay2=12;


void setup() {
  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  Serial.begin(9600); 
  gravityTds.begin();  
  gravityTds.setPin(TdsSensorPin);
  gravityTds.setAref(5.0);  //reference voltage on ADC, default 5.0V on Arduino UNO
  gravityTds.setAdcRange(1024);
  //timer.setInterval(500L, display_pHValue);
//  Relays setup
  pinMode(relay1, OUTPUT); 
  pinMode(relay2, OUTPUT);  
  digitalWrite(relay1, HIGH);
  digitalWrite(relay2, HIGH); 
// ESP8266 set-up
  Serial.begin(9600);
  espSerial.begin(115200);
  
  espData("AT+RST", 1000, DEBUG);                      //Reset the ESP8266 module
  espData("AT+CWMODE=1", 1000, DEBUG);                 //Set the ESP mode as station mode
  espData("AT+CWJAP=\""+ mySSID+"\",\""+ myPWD +"\"", 1000, DEBUG);   //Connect to WiFi network
  
  delay(1000);
}
void getData() {
    delay(2000); 
    float lmvalue = analogRead(LM35);
    tempr = (lmvalue * 500)/1023 ;
     gravityTds.setTemperature(temperature);  // set the temperature and execute temperature compensation
     gravityTds.update();      
     tdsValue = gravityTds.getTdsValue();
     timer.run(); // Initiates SimpleTimer 
    for(int i=0;i<10;i++)  
    {buffer_arr[i]=analogRead(A0); 
    delay(30); } 
     for(int i=0;i<9;i++) { for(int j=i+1;j<10;j++) { if(buffer_arr[i]>buffer_arr[j]) { temp=buffer_arr[i]; 
     buffer_arr[i]=buffer_arr[j]; 
     buffer_arr[j]=temp;
   } 
   } 
   } 
   avgval=0; 
   for(int i=2;i<8;i++) avgval+=buffer_arr[i]; 
   float volt=(float)avgval*5.0/1024/6;   
   ph_act = -5.70 * volt + calibration_value; 
   Serial.print("pH Val: "); 
   Serial.println(ph_act); 
   delay(1000);     
     Serial.print(tdsValue,0);
     Serial.println("ppm");    
     Serial.println(tempr); 
    Serial.print("The distance is:");
   Serial.println(sonar.ping_cm());   
}
       
void displayData() {
     display.clearDisplay();
     display.setTextColor(WHITE);
     display.setTextSize(1); 
     display.setCursor(0,0);  
     display.print("Temp:"); 
     display.print(tempr);    
     display.setTextColor(WHITE);
     display.setTextSize(1); 
     display.setCursor(0,10);  
     display.print("TDS:"); 
     display.print(tdsValue); 
     display.setTextColor(WHITE);
     display.setTextSize(1); 
     display.setCursor(0,20);  
     display.print("pH:"); 
     display.print(ph_act); 
}
void loop() {
    getData(); 
    relay();  
    displayData();       
   display.display();   
   ESP8266();
   delay(1000);  
}    
    
void relay() {  
if(tdsValue >= 700){
    digitalWrite(relay1, LOW);
    Serial.println("Pump A on");
    }    
    else{
      digitalWrite(relay1, HIGH);
    }
delay (1000) ;  
if(ph_act >= 6){
    digitalWrite(relay2, LOW);
    Serial.println("Pump B on");
    }    
    else{
      digitalWrite(relay2, HIGH);
    }    

}
void ESP8266(){
    sendVal = tdsValue; 
    String sendData = "GET /update?api_key="+ myAPI +"&"+ myFIELD +"="+String(sendVal);
    espData("START", 1000, DEBUG);       //Allow multiple connections
    espData("AT+CIPMUX=1", 1000, DEBUG);       //Allow multiple connections
    espData("AT+CIPSTART=0,\"TCP\",\""+ myHOST +"\","+ myPORT, 1000, DEBUG);
    espData("AT+CIPSEND=0," +String(sendData.length()+4),1000,DEBUG);  
    espSerial.find(">"); 
    espSerial.println(sendData);
    Serial.print("TDS is: ");
    Serial.println(sendVal);
     
    espData("AT+CIPCLOSE=0",1000,DEBUG);
    // delay(10000);
  }

  String espData(String command, const int timeout, boolean debug)
{
  Serial.print("AT Command ==> ");
  Serial.print(command);
  Serial.println("     ");
  
  String response = "";
  espSerial.println(command);
  long int time = millis();
  while ( (time + timeout) > millis())
  {
    while (espSerial.available())
    {
      char c = espSerial.read();
      response += c;
    }
  }
  if (debug)
  {
    //Serial.print(response);
  }
  return response;
}
  
