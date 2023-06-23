  // Exmaple of using the MQTT library for ESP32 
// Library by Joël Gähwiler
// https://github.com/256dpi/arduino-mqtt
// Modified by Arnan Sipitakiat


#include <WiFi.h>
#include <MQTT.h>
#include <LCD_I2C.h>
#include <Adafruit_I2CDevice.h>
#include "RTClib.h"
#define BUTTON 32
#define HINPUT1 16
#define HINPUT2 17
#define PCF8574_Address 0x20
Adafruit_I2CDevice i2c_dev = Adafruit_I2CDevice(PCF8574_Address);
uint8_t buffer[32];
const char ssid[] = "FrienD";
const char pass[] = "789632145";
const char mqtt_broker[]="test.mosquitto.org";
const char mqtt_topic[]="group1/command";
int MQTT_PORT=1883;
int counter=0;
String word1;
long randomTime;
bool spinning1; 
int showtext=1;

WiFiClient net;
MQTTClient client;


unsigned long lastMillis = 0;
RTC_DS1307 rtc;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
LCD_I2C lcd(0x27, 16, 2); // Default address of most PCF8574 modules, change according

void connect() {
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.print("\nconnecting...");
  while (!client.connect("arduino_group_1")) {  // connection ID should be unique
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nconnected!");

  client.subscribe(mqtt_topic);
  // client.unsubscribe("/hello");
}

void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);
  word1 = payload;

}

void setup() {
  Serial.begin(9600);
  #ifndef ESP32 Dev Module
  while (!Serial); // wait for serial port to connect. Needed for native USB
#endif

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
    
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
   
  }

  
    lcd.begin(); 
    lcd.backlight();
  WiFi.begin(ssid, pass);

  Serial.println("I2C address detection test");
  if (!i2c_dev.begin()) {
    Serial.print("Did not find device at 0x");
    Serial.println(i2c_dev.address(), HEX);
    while (1);
  }
  Serial.print("Device found on address 0x");
  Serial.println(i2c_dev.address(), HEX);

  // Config PCF8574
  buffer[0]=1;               
  i2c_dev.write(buffer,1);   // send the config to PCF8574
  
  client.begin(mqtt_broker, MQTT_PORT, net);
  client.onMessage(messageReceived);
  pinMode(BUTTON,INPUT);
  pinMode(HINPUT1,OUTPUT);
  pinMode(HINPUT2,OUTPUT);
  connect();
}

void loop() {
  client.loop();
  delay(10);  // <- fixes some issues with WiFi stability

  if (!client.connected()) {
    connect();
  }
  i2c_dev.read(buffer,1);

 
  if((buffer[0]&0b1) == 0b0||word1 == "spin"||spinning1==true){
    word1 = "temp";
    if(spinning1 == false){
      randomTime = random(2000,7000);
      lastMillis = millis();
    }
   if(randomTime % 2 == 0){
       buffer[0]=0b10000001;
       i2c_dev.write(buffer,1);
      // digitalWrite(HINPUT1, HIGH);
      // digitalWrite(HINPUT2, LOW);
      if(showtext == 1){
       client.publish(mqtt_topic, "Spinning..."); 
       lcd.clear();
       lcd.print("Spinning...");
       showtext += 1;
      }
      spinning1 = true;
      if(millis() - lastMillis >= randomTime){
         buffer[0]=0b11000001;
         i2c_dev.write(buffer,1);
        // digitalWrite(HINPUT1, HIGH);
        // digitalWrite(HINPUT2, HIGH);
        client.publish(mqtt_topic, "Spin Completed");
        lcd.clear();
        lcd.print("Spin Complete");
        lcd.setCursor(0, 1); 
        DateTime now = rtc.now();
        lcd.print(now.hour(), DEC);
        lcd.print(':');
        lcd.print(now.minute(), DEC);
        lcd.print(':');
        lcd.print(now.second(), DEC);
        spinning1 = false;
        showtext -= 1;
      }
    }else{
      buffer[0]=0b01000001;
      i2c_dev.write(buffer,1);
      // digitalWrite(HINPUT1, LOW);
      // digitalWrite(HINPUT2, HIGH);
      if(showtext == 1){
       client.publish(mqtt_topic, "Spinning..."); 
       lcd.clear();
       lcd.print("Spinning...");
       showtext += 1;
      }
      spinning1 = true;
      if(millis() - lastMillis >= randomTime){
        buffer[0]=0b11000001;
        i2c_dev.write(buffer,1);
        // digitalWrite(HINPUT1, HIGH);
        // digitalWrite(HINPUT2, HIGH);
        client.publish(mqtt_topic, "Spin Completed");
        lcd.clear();
        lcd.print("Spin Complete");
        lcd.setCursor(0, 1); 
        DateTime now = rtc.now();
        lcd.print(now.hour(), DEC);
        lcd.print(':');
        lcd.print(now.minute(), DEC);
        lcd.print(':');
        lcd.print(now.second(), DEC);
        spinning1 = false;
        showtext -= 1;
      }
    }
    while(digitalRead(BUTTON)==LOW){
        delay(10);
    }
  }

}