#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include "DHT.h"
#include <ThingSpeak.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

/// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "Che-Steve"
#define WIFI_PASSWORD "10345679"

// Insert Firebase project API Key
#define API_KEY "AIzaSyDEP9Sp-gquKxZrQ1zxeCPpf4bNXIn2Kmc"

// Insert Authorized Email and Corresponding Password
#define USER_EMAIL "stevenkamwaza@gmail.com"
#define USER_PASSWORD "stevkamwa"

// Insert RTDB URLefine the RTDB URL
#define DATABASE_URL "projectdb-e7c09-default-rtdb.europe-west1.firebasedatabase.app"

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;

// Database main path (to be updated in setup with the user UID)
String databasePath;
// Database child nodes
String tempPath = "/temperature";
String humPath = "/humidity";
String rainPath = "/rain";
String timePath = "/timestamp";
String lightPath = "/light";

// Parent Node (to be updated in every loop)
String parentPath;
FirebaseJson json;
// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
// Variable to save current epoch time
int timestamp;

//ldr sensor
const int LDR = A0;
//alarm variables
int buzzer = D1;
long startBuzzer, stopBuzzer;
bool flag = false;
unsigned long buzzerTime = 0;
#define rain D3
boolean isRaining;
DHT dht(D4, DHT11);
WiFiClient client;
long thingsChannelNum = 1635256;
const char thingWriteAPI[] = "95IR0A340G6P54M3";

// Initialize WiFi
void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.print('Connected to  ');
  Serial.print(WiFi.localIP());
  Serial.println();
}

  // Function that gets current epoch time
  unsigned long getTime() {
    timeClient.update();
    unsigned long now = timeClient.getEpochTime();
    return now;
}
// Timer variables (send new readings every three minutes)
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 180000;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  initWiFi();
  timeClient.begin();
  //ldr sensor

  //Dht begin
  dht.begin();
  pinMode(buzzer,OUTPUT);
  ThingSpeak.begin(client);

  // Assign the api key (required)
  config.api_key = API_KEY;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  // Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);
  // Update database path
  databasePath = "/weatherdata";
  
}

void runBuzzer(){
  //tone(buzzer,2000,500);
  digitalWrite (buzzer, HIGH); //turn buzzer on
//  delay(100);
//  digitalWrite (buzzer, LOW);  //turn buzzer off
//  delay(100);
 }

void loop() {
  // put your main code here, to run repeatedly:
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  //read digital value
  int rainValue = digitalRead(rain);
  //buzzer
  if(digitalRead(rain)==HIGH){
    if(!flag){
        flag = true;
        buzzerTime = millis();
    }
    if((millis() - buzzerTime) >= 5000){
      runBuzzer();
    }
  }else{
    flag =false;
  }
  
  if(rainValue==0){
    Serial.println(" It is raining : " + (String) rainValue);
  }
  else{
    Serial.println(" No rain : " + (String) rainValue);
  }
  //ldr sensor
  int lightValue = analogRead(A0);
  
  Serial.println(" Temperature: " + (String) temp);
  Serial.println(" Humidity: " + (String) hum);
  
  Serial.println(" LDR : " + (String) lightValue);

  //check if any reading is nan
 if(isnan(hum)|| isnan(temp) || isnan(rainValue)||isnan(lightValue)){
   Serial.println("Failed to read a sensor");
   delay(2000);
   return;
 }
  
   
 ThingSpeak.writeField(thingsChannelNum, 1, temp,thingWriteAPI);
 ThingSpeak.writeField(thingsChannelNum, 2, hum,thingWriteAPI);
 ThingSpeak.writeField(thingsChannelNum, 3, rainValue,thingWriteAPI);
 ThingSpeak.writeField(thingsChannelNum, 4, lightValue,thingWriteAPI);

   // Send new readings to database
  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();
    //Get current timestamp
    timestamp = getTime();
    Serial.print ("time: ");
    Serial.println (timestamp);

    parentPath= databasePath + "/" + String(timestamp);

    json.set(tempPath.c_str(), String(temp));
    json.set(humPath.c_str(), String(hum));
    json.set(rainPath.c_str(), String(rainValue));
    json.set(timePath, String(timestamp));
    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
  }
  delay(2000);

}
