/**************************** INCLUDES ********************************************/
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>
#include <Servo.h>
#include <stdlib.h>
#include "credentials.h"


/******************************** OTA **************************************************/
#define SENSORNAME "lovebox"
#define OTApassword "OTAPass" // change this to whatever password you want to use when you upload OTA
#define OTAport 8266


/******************************* MQTT **************************************************/
#define message_topic "lovebox/Message"
#define status_topic "lovebox/Status"


/******************************* WIFI **************************************************/
WiFiClient espClient;
PubSubClient client(espClient);


/******************************* LIGHT SENSOR  *****************************************/
#define LS_PIN A0
#define POWER 4


/********************************* SCREEN **********************************************/
// Screen dimensions
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 128

// Pin Settings
const uint8_t   OLED_pin_scl_sck        = 14;
const uint8_t   OLED_pin_sda_mosi       = 13;
const uint8_t   OLED_pin_cs_ss          = 15;
const uint8_t   OLED_pin_res_rst        = 5;
const uint8_t   OLED_pin_dc_rs          = 4;

// Color Definitions
const uint16_t  OLED_Color_Black        = 0x0000;
const uint16_t  OLED_Color_Blue         = 0x001F;
const uint16_t  OLED_Color_Red          = 0xF800;
const uint16_t  OLED_Color_Green        = 0x07E0;
const uint16_t  OLED_Color_Cyan         = 0x07FF;
const uint16_t  OLED_Color_Magenta      = 0xF81F;
const uint16_t  OLED_Color_Yellow       = 0xFFE0;
const uint16_t  OLED_Color_White        = 0xFFFF;

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


/********************************* SERVO **********************************************/
Servo myservo; 
int startpos = 90;
int pos = 90;
int increment = -1;
int ccw = 75;
int cw = 105; 


/********************************* MESSAGE ********************************************/
bool unopened_message = false;


/************************ START PRINT MESSAGE ******************************************/
void printMessage(const String& message) {
  //Determine the type of message
  
  if (message[0]=='t'){
    //Text Message
    Serial.println("Clearing display");
    oled.fillScreen(OLED_Color_Black);
    Serial.println("Text Message - Writing to OLED");
    write_message(message);  
  }else{
    //Image Message
    Serial.println("Image Message - Drawing on OLED");
    draw_message(message);
  }
}


/************************ START DRAW MESSAGE ******************************************/
void write_message(const String& message) {
  // home the cursor
  oled.setCursor(0,0);
  
  // change the text color to foreground color
  oled.setTextColor(OLED_Color_White);

  // Write the message
  oled.print(message);
}


/************************ START DRAW MESSAGE ******************************************/
void draw_message(const String& message) {
  //First character is the line that we are drawing
  //To get the full number, we need to check where teh first color code starts
  int line_number;
  int offset;
  if (message[2] == '0' && message[3] == 'x'){
    offset = 2;
    line_number = message[0] - 48;
  }else if(message[3] == '0'){
    offset = 3;
    int x = message[0] - 48;
    int y = message[1] - 48;
    line_number = 10 * x + y;
  }else{
    offset = 4;
    int x = message[0] - 48;
    int y = message[1] - 48;
    int z = message[2] - 48;
    line_number = x * 100 + y * 10 + z;
  }
  Serial.println(line_number);
  if (line_number == 0){
    Serial.println("Clearing display");
    oled.fillScreen(OLED_Color_Black);
  }
  for(uint16_t i = 0; i < SCREEN_WIDTH; i++){
    String hex_string = "";
    for(int j = 0; j < 6; j++){
      hex_string = hex_string + message[offset+6*i+j]; 
    }
    //Serial.println(hex_string);
    int16_t pixel_color = strtol(hex_string.c_str(), NULL, 0);
    oled.drawPixel(line_number, i, pixel_color);  
  }
}


/********************************** START WIFI ********************************************/
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


/********************************** START OTA ********************************************/
void setup_ota() {
  Serial.println("Setting up OTA.");
  ArduinoOTA.setPort(OTAport);
  ArduinoOTA.setHostname(SENSORNAME);
  ArduinoOTA.setPassword((const char *)OTApassword);
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
  Serial.println("OTA Setup Complete.");
}


/********************************** START CALLBACK*****************************************/
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]");

  char message[length + 1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  Serial.println(message[0]);
  unopened_message = true;
  EEPROM.put(144, unopened_message);
  printMessage(message);
}


/********************************** START SEND STATE*****************************************/
void sendState() {
  const char* status_message = "Message has been opened";
  if (unopened_message){
    status_message = "Message has not been opened";
  }
  client.publish(status_topic, status_message);
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


/********************************** START SPIN SERVO ***************************************/
void spin_servo(){
  //Write the new position to the servo
  myservo.write(pos);      
  //Wait 50ms for servo to actuate
  delay(50);
  //If we are at one of the two boundaries, then reverse the current increment to turn the other way
  if(pos == ccw || pos == cw){
    increment *= -1;
  }
  //Increment the position for next call
  pos += increment;
}


/********************************** START RESET SERVO ***************************************/
void reset_servo(){
  //Read the current position from the servo
  int currpos = myservo.read();      
  while (currpos != startpos){
    //Set the increment to work in the direction of the start position
    if(currpos < startpos){
      increment = 1;
    }else{
      increment = -1;
    }
    //Increment the position for next call
    currpos += increment;
    myservo.write(currpos);
    //Wait 50ms for servo to actuate
    delay(50);
  }
}

/********************************** START GET LIGHT ****************************************/
float get_light(){
  //Get the light value as a percentage of max
  float reading = analogRead(LS_PIN) / 1024.0 * 100.0;
  return reading;
}


/********************************** START SETUP*****************************************/
void setup() {

  //Set the serial baud rate
  Serial.begin(9600);
  delay(10);
  Serial.println("Starting Node named " + String(SENSORNAME));


  //Set up the servo
  myservo.attach(2);
  myservo.write(90);

  //Set up the light sensor
  pinMode(LS_PIN,  INPUT);

  //Allow sensors to settle
  delay(250);
  
  //Set up the OTA
  setup_ota();

  //Set up WiFi
  setup_wifi();

  //Set up MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  //Print success to serial
  Serial.println("Ready");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  reconnect();

  //Set up the screen
  Serial.println("Setting up screen");
  oled.begin();
  oled.setFont();
  oled.fillScreen(OLED_Color_Black);
  oled.setTextColor(OLED_Color_White);
  oled.setTextSize(1);
  isDisplayVisible = true;

  //Set up EEPROM to maintain values when powered off
  EEPROM.begin(512);
  EEPROM.get(144, unopened_message);
  Serial.println("Setting up EEPROM with a starting value of " + (String)unopened_message);
}


/********************************** START LOOP*****************************************/
void loop() {
  //For OTA
  ArduinoOTA.handle();

  //For MQTT
  client.loop();

  //Make sure WiFi is connected
  if (WiFi.status() != WL_CONNECTED) {
    setup_wifi();
  }
  /*
  while (unopened_message){
    //While there is an unopened message, spin the servo and check the light sensor
    spin_servo();
    float lightval = get_light();
    if(lightval > 5.0){
      //Box was opened. 
      
      Serial.println("Box Opened!");
      reset_servo();
      unopened_message = false;
      EEPROM.put(144, unopened_message);
    }
  }
  */ 
  delay(100);
}
