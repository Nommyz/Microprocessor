// DHT Temperature & Humidity Sensor
// Unified Sensor Library Example
// Written by Tony DiCola for Adafruit Industries
// Released under an MIT license.

// REQUIRES the following Arduino libraries:
// - DHT Sensor Library: https://github.com/adafruit/DHT-sensor-library
// - Adafruit Unified Sensor Lib: https://github.com/adafruit/Adafruit_Sensor
#include <PID_v2.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <WiFi.h>
#include <MQTT.h>
#define Hinput1 17
#define Hinput2 16
#define relay 23
#define DHTPIN 18  // Digital pin connected to the DHT sensor
// Feather HUZZAH ESP8266 note: use pins 3, 4, 5, 12, 13 or 14 --
// Pin 15 can work but DHT must be disconnected during program upload.

// Uncomment the type of sensor in use:
//#define DHTTYPE    DHT11     // DHT 11
#define DHTTYPE DHT22  // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

// See guide for details on sensor wiring and usage:
//   https://learn.adafruit.com/dht/overview

DHT_Unified dht(DHTPIN, DHTTYPE);
const char ssid[] = "FrienD";
const char pass[] = "789632145";
const char mqtt_broker[] = "test.mosquitto.org";
const char mqtt_plotgraphTopic[] = "group1/command1";
const char mqtt_readinputTopic[] = "group1/command2";
const char mqtt_tuner[] = "group1/command3";
const char mqtt_client_id[] = "arduino_group_x";  // must change this string to a unique value
bool state = true;
bool isfirst = true;
bool reach = false;
double tempNow = 0;
int MQTT_PORT = 1883;
double calculate = 0;
WiFiClient net;
MQTTClient client;
unsigned long lastMillis = 0;
String expect;
String oldExpect = "0";
String curTemp;
String tuner;
// Specify the links and initial tuning parameters
double Kp = 2, Ki = 5, Kd = 1;
PID_v2 myPID(Kp, Ki, Kd, PID::Direct);

void connect() {
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.print("\nconnecting...");
  while (!client.connect(mqtt_client_id)) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nconnected!");

  client.subscribe(mqtt_plotgraphTopic);
  client.subscribe(mqtt_readinputTopic);
  client.subscribe(mqtt_tuner);
}
void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);
  if (topic == "group1/command2") {
    expect = payload;
  }
  if (topic == "group1/command1") {
    curTemp = payload;
  }
  if (topic == "group1/command3") {
    tuner = payload;
  }
}

void setup() {
  Serial.begin(9600);
  tuner = 0.5;
  pinMode(Hinput1, OUTPUT);
  pinMode(Hinput2, OUTPUT);
  pinMode(relay, OUTPUT);
  dht.begin();
  WiFi.begin(ssid, pass);

  client.begin(mqtt_broker, MQTT_PORT, net);
  client.onMessage(messageReceived);


  // tell the PID to range between 0 and the full window size
  myPID.SetOutputLimits(0, 50);
  myPID.Start(analogRead(DHTPIN),  // input
              0,                   // current output
              expect.toDouble());  // setpoint

  connect();
}
void loop() {
  client.loop();
  delay(10);
  if (!client.connected()) {
    connect();
  }
  myPID.Setpoint(expect.toDouble());
  if (millis() - lastMillis >= 2000) {

    sensors_event_t event;
    dht.temperature().getEvent(&event);
    const double output = myPID.Run(event.temperature);
    if (isnan(event.temperature)) {
      Serial.println("Error reading temperature!");
    } else {
      String temp = String(event.temperature);
      client.publish(mqtt_plotgraphTopic, temp);
      // Serial.println(output);
    }
    if (isfirst) {
      tempNow = event.temperature;
      Serial.println("AAA");
      isfirst = false;
    }

    if (!reach) {
      calculate = (expect.toDouble() - curTemp.toDouble()) / (expect.toDouble() - tempNow);
      Serial.printf("Calculate = %f \n", calculate);
      Serial.println(expect.toDouble());
      Serial.println(expect.toDouble() - curTemp.toDouble());
      Serial.println(expect.toDouble() - tempNow);
      if (expect.toDouble() > curTemp.toDouble() && calculate > tuner.toDouble()) {
        // turn on light
        if (state) {
          tempNow = curTemp.toDouble();
          state = false;
        }
        digitalWrite(relay, HIGH);
        digitalWrite(Hinput1, LOW);
        //digitalWrite(Hinput2, LOW);
      } else {
        // turn on fan
        digitalWrite(relay, LOW);
        digitalWrite(Hinput1, HIGH);
        state = true;
        //digitalWrite(Hinput2, LOW);
      }
      if ((expect.toDouble() <= event.temperature + 0.1) && (expect.toDouble() > 10)) {
        reach = !reach;
      }

    } else {
      if (expect.toDouble() > curTemp.toDouble()) {
        // turn on light
        digitalWrite(relay, HIGH);
        digitalWrite(Hinput1, LOW);
        //digitalWrite(Hinput2, LOW);
      } else {
        // turn on fan
        digitalWrite(relay, LOW);
        digitalWrite(Hinput1, HIGH);
        state = true;
        //digitalWrite(Hinput2, LOW);
      }
    }

    if (oldExpect.toDouble() < expect.toDouble()) {

      reach = false;
    }
    if (!expect.equals(oldExpect)) {
      oldExpect = expect;
      tempNow = event.temperature;
      Serial.println(oldExpect);
      Serial.println(expect);
    }
    Serial.println(tuner);
    lastMillis = millis();
  }
}