#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <Servo.h>

#include "credentials.h"

/**************************** FOR OTA **************************************************/
#define SENSORNAME "lovebox"
#define OTApassword "OTAPass" // change this to whatever password you want to use when you upload OTA
#define OTAport 8266

/************* MQTT TOPICS (change these topics as you wish)  **************************/
#define message_topic "lovebox/Message"

/************************ Screen Settings **********************************************/
// Screen dimensions
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 128

// The SSD1351 is connected like this (plus VCC plus GND)
const uint8_t   OLED_pin_scl_sck        = 14;
const uint8_t   OLED_pin_sda_mosi       = 13;
const uint8_t   OLED_pin_cs_ss          = 15;
const uint8_t   OLED_pin_res_rst        = 5;
const uint8_t   OLED_pin_dc_rs          = 4;

// SSD1331 color definitions
const uint16_t  OLED_Color_Black        = 0x0000;
const uint16_t  OLED_Color_Blue         = 0x001F;
const uint16_t  OLED_Color_Red          = 0xF800;
const uint16_t  OLED_Color_Green        = 0x07E0;
const uint16_t  OLED_Color_Cyan         = 0x07FF;
const uint16_t  OLED_Color_Magenta      = 0xF81F;
const uint16_t  OLED_Color_Yellow       = 0xFFE0;
const uint16_t  OLED_Color_White        = 0xFFFF;

// The colors we actually want to use
uint16_t        OLED_Text_Color         = OLED_Color_White;
uint16_t        OLED_Backround_Color    = OLED_Color_Black;

// declare the display

Adafruit_SSD1351 oled =
    Adafruit_SSD1351(
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        &SPI,
        OLED_pin_cs_ss,
        OLED_pin_dc_rs,
        OLED_pin_res_rst
     );
// assume the display is off until configured in setup()
bool            isDisplayVisible        = false;


Servo myservo; 
int pos = 90;
int increment = -1;
int lightValue;
String line;
String modus;
char idSaved; 
bool wasRead;  

char message_buff[100];

const int BUFFER_SIZE = 300;

#define MQTT_MAX_PACKET_SIZE 512

WiFiClient espClient;
PubSubClient client(espClient);

void drawMessage(const String& message) {
  Serial.println("Clearing display");
  oled.fillScreen(OLED_Color_Black);
    
  Serial.println("Attempting to draw message");
  // home the cursor
  oled.setCursor(0,0);
  
  // change the text color to foreground color
  oled.setTextColor(OLED_Text_Color);

  // draw the new time value
  oled.print(message);
}



/********************************** START CONNECT*****************************************/
void setup_wifi() {

  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

/********************************** START CALLBACK*****************************************/
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  char message[length + 1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  Serial.println(message);
  drawMessage(message);
}

/********************************** START SEND STATE*****************************************/
void sendState() {
  /*
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();

  root["humidity"] = (String)humValue;
  root["motion"] = (String)motionStatus;
  root["temperature"] = (String)tempValue;
  root["heatIndex"] = (String)dht.computeHeatIndex(tempValue, humValue, IsFahrenheit);

  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));

  Serial.println(buffer);
  client.publish(light_state_topic, buffer, true);
  */
}

/********************************** START RECONNECT*****************************************/
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(SENSORNAME, mqtt_user, mqtt_password)) {
      Serial.println("connected");
      client.subscribe(message_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void spinServo(){
    myservo.write(pos);      
    delay(50);    // Warte 50ms um den Servo zu drehen

    if(pos == 75 || pos == 105){ // Drehbereich zwischen 75°-105°
      increment *= -1;
    }
    pos += increment;
}

void setup() {

  
  myservo.attach(2);       // Servo an D0
  myservo.write(0);

  // settling time
  delay(250);
  
  Serial.begin(9600);
  
  delay(10);

  ArduinoOTA.setPort(OTAport);

  ArduinoOTA.setHostname(SENSORNAME);

  ArduinoOTA.setPassword((const char *)OTApassword);

  Serial.println("Starting Node named " + String(SENSORNAME));

  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  ArduinoOTA.onStart([]() {
    Serial.println("Starting");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  reconnect();

  // initialise the SSD1331
  oled.begin();
  oled.setFont();
  oled.fillScreen(OLED_Backround_Color);
  oled.setTextColor(OLED_Text_Color);
  oled.setTextSize(1);

  // the display is now on
  isDisplayVisible = true;
  
  //EEPROM.begin(512);
  //idSaved = EEPROM.get(142, idSaved);
  //wasRead = EEPROM.get(144, wasRead);


}

void loop() {

  ArduinoOTA.handle();
  
  client.loop();
  
  if (WiFi.status() != WL_CONNECTED) {
    setup_wifi();
  }


  /*
  while(!wasRead){   
    spinServo();    // Drehe Herz
    lightValue = analogRead(0);      // Lese Helligkeitswert
    if(lightValue > 300) { 
      wasRead = 1;
      EEPROM.write(144, wasRead);
      EEPROM.commit();
    }
  }
  */
}

/****reset***/
void software_Reset() // Restarts program from beginning but does not reset the peripherals and registers
{
Serial.print("resetting");
ESP.reset(); 
}
