#include <Arduino.h>

#include <Adafruit_SHT31.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include "credentials.h"

const unsigned int updateInterval = 300; // in seconds

// Initialize the WiFi and MQTT client object
WiFiClient espClient;
PubSubClient client(espClient);

// Initialize SHT31 sensor
Adafruit_SHT31 sht31 = Adafruit_SHT31();

// function prototypes
bool getAndSendTemperatureAndHumidityData();
bool InitWiFi();
bool connect();
void blinkSuccessAndSleep();
void blinkFailedAndSleep();

void setup()
{
  digitalWrite(D4, HIGH);
  pinMode(D4, OUTPUT); // LED
  pinMode(D0, WAKEUP_PULLUP);

  Serial.begin(115200);

  if (! sht31.begin(0x45)) {
    Serial.println("Couldn't find SHT31");
    blinkFailedAndSleep();
  }

  if (! InitWiFi())
  {
    blinkFailedAndSleep();
  }

  client.setServer(THINGSBOARD_SERVER, 1883);
  if (!connect())
  {
    blinkFailedAndSleep();
  }

  if (getAndSendTemperatureAndHumidityData())
  {
    client.loop();
    blinkSuccessAndSleep();
  }
  else
  {
    blinkFailedAndSleep();
  }
}

void loop()
{

}

bool getAndSendTemperatureAndHumidityData()
{
  float t = sht31.readTemperature();
  float h = sht31.readHumidity();
  bool ret = true;

  Serial.print("Collecting temperature data ... ");

  if (isnan(t)) {  // check if 'is not a number'
    ret = false;
  }

  if (isnan(h)) {  // check if 'is not a number'
    ret = false;
  }

  if (ret == true)
  {
    Serial.println("[DONE]");
  }
  else
  {
    Serial.println("[FAILED]");
    return false;
  }

  String temperature = String(t);
  String humidity = String(h);

  // Prepare a JSON payload string
  String payload = "{";
  payload += "\"temperature\":"; payload += temperature; payload += ",";
  payload += "\"humidity\":"; payload += humidity;
  payload += "}";

  // Send payload
  char attributes[100];
  payload.toCharArray(attributes, 100);
  Serial.print("Sending \"");
  Serial.print(attributes);
  Serial.print("\" to server ... ");
  if (client.publish("v1/devices/me/telemetry", attributes))
  {
    Serial.println("[DONE]");
  }
  else
  {
    Serial.println("[FAILED]");
  }
  return ret;
}

bool InitWiFi()
{
  int i = 0;

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_AP, WIFI_PASSWORD);

  // attempt to connect to WiFi network
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println();
    Serial.print("Connecting to SSID ");
    Serial.print(WIFI_AP);
    Serial.print(" ... ");
    delay(1000);
    if (i++ > 5)
      break;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[DONE]");
    return true;
  }
  else
  {
    Serial.println("[FAILED]");
    return false;
  }
}

bool connect()
{
  bool ret = false;
  int i = 0;

  // Loop until we're connected
  while (!client.connected()) {
    Serial.print("Connecting to ThingsBoard node ... ");
    // Attempt to connect (clientId, username, password)
    if (client.connect("Arduino Uno Device", TOKEN, NULL)) {
      Serial.println("[DONE]");
      ret = true;
    } else {
      Serial.print("[FAILED] [ rc = ");
      Serial.print(client.state());
      Serial.println(" : retrying in 5 seconds]");
      // Wait 5 seconds before retrying
      delay(5000);
      i++;
    }
    if (i > 5)
    break;
  }
  return ret;
}

// pattern "- -"
void blinkSuccessAndSleep()
{
  digitalWrite(D4, LOW);
  delay(500);
  digitalWrite(D4, HIGH);
  delay(500);
  digitalWrite(D4, LOW);
  delay(500);
  digitalWrite(D4, HIGH);

  // sleep for x seconds
  ESP.deepSleep(updateInterval * 1e6);
  delay(100);
}
// pattern "- ..."
void blinkFailedAndSleep()
{
  digitalWrite(D4, LOW);
  delay(500);
  digitalWrite(D4, HIGH);
  delay(500);
  digitalWrite(D4, LOW);
  delay(50);
  digitalWrite(D4, HIGH);
  delay(200);
  digitalWrite(D4, LOW);
  delay(50);
  digitalWrite(D4, HIGH);
  delay(200);
  digitalWrite(D4, LOW);
  delay(50);
  digitalWrite(D4, HIGH);  // sleep for x seconds

  ESP.deepSleep(updateInterval * 1e6);
  delay(100);
}
