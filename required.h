#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>

#include <Adafruit_Sensor.h>    // Adafruit  sensor library
#include <Adafruit_ADXL345_U.h> // ADXL345 library

#include <NTPClient.h>
#include <WiFiUdp.h>

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// getting location data from app ; for http connection
#include <ESP8266HTTPClient.h> // 
#include <WiFiClient.h>        //
// #define timer_delay 5000       // fetch location data every 'timer_delay' ms.
//Your Domain name with URL path or IP address with path
String serverName = "http://192.168.149.122:8080";
String Location = ""; // fetched data to be stored here

// Insert your network credentials
#define WIFI_SSID "Keerthana"
#define WIFI_PASSWORD "Password"

// Insert Firebase project API Key
#define API_KEY "AIzaSyC3w-8kiBswLxJvOYjySBQUl68nbwGsQpA"
#define max_retry 5

// Insert Authorized Email and Corresponding Password
#define USER_EMAIL "keyur@maxwlfw.com"
#define USER_PASSWORD "maxwlfw-rocks"

// Insert RTDB URLefine the RTDB URL
#define DATABASE_URL "https://dbtrials-default-rtdb.asia-southeast1.firebasedatabase.app/"
