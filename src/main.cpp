#include <Arduino.h>
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>

#include "Settings.h"
#define VERSION "0.1"

#define DEBUG 0

// IP Adresse vom ESP
IPAddress ip(192, 168, 0, 148);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

//MQTT Broker
const String mqttNode = "bu_Blume";
const String mqttDiscoveryPrefix = "/FHEM/Buero/";

// PIN
#define SOIL_PIN A0

int soilMoistureLevel;
float soilMoistureLevelf;   

int dryPercent = 28;
int wetPercent = 69;

WiFiClient net;
PubSubClient client(net);

void connect_wifi()
{
  Serial.begin(115200);
  WiFi.hostname(sensorname);
  WiFi.config(ip, gateway, subnet);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {  //Wait for connection
    delay(500);
    Serial.println("Waiting to connect…");
  }

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  //Print the local IP
}

void reconnect() {
  while (!client.connected()) {
    String clientId = sensorname;
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      client.subscribe("bu_Blume_dev");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");

      delay(5000);
    }
  }
}
void setup_OTA()
{
  Serial.println("Setup OTAg");
  delay(100);
  ArduinoOTA.setPort(ota_port);
  ArduinoOTA.setHostname(sensorname);
  ArduinoOTA.setPassword((const char *)ota_passwd);

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
}
void callback(char* topic, byte* payload, unsigned int length) {
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void connect_MQTT() {
  Serial.print("---> Connecting to MQTT, ");
  client.setServer(mqtt_server, mqtt_Port);
  
  while (!client.connected()) {
    Serial.println("reconnecting MQTT...");
    reconnect(); 
  }
  Serial.println("MQTT connected ok.");
}

void getMoistureLevel()
{
  Serial.println("getMoistureLevel...");
  delay(100);
  soilMoistureLevelf = analogRead(SOIL_PIN);
  soilMoistureLevelf = ((soilMoistureLevelf - dryPercent)/(wetPercent-dryPercent))*100;
  soilMoistureLevel = int(soilMoistureLevelf);

  String soilLevelstring = String(soilMoistureLevel);
  char const *soilLevel_str = soilLevelstring.c_str();
  //soilLevel = (1-(soilLevel/1023))*100;
  #if DEBUG >0
    client.publish("bu_blume/debug/soilLevel", soilLevel_str);
  #endif
  String mqttMSG = mqttDiscoveryPrefix + mqttNode + "/soilMoistureLevel";
  Serial.println(mqttMSG);
  client.publish(mqttMSG.c_str(), soilLevel_str);

  delay(100);
}
void setup() {
  Serial.begin(115200);                     // Debug console
  Serial.println("Soil Moisture Sensor v1.2");     // Soil moisture sensor v1
  delay( 3000 );  
 
  connect_wifi();
  setup_OTA();
  connect_MQTT();

}

void loop() {
  getMoistureLevel();
  delay(100);

  Serial.println(soilMoistureLevel);
  Serial.println("wait");
  delay(60 * 60000);
}