#include "Arduino.h"
#include "WiFi.h"
#include "PubSubClient.h"
#include <ArduinoJson.h>
#include <time.h>

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_broker = "mqtt.iotserver.uz";
const int mqtt_port = 1883;
const char* mqtt_username = "given in telegram group";
const char* mqtt_password = "given in telegram group";
const char* client_id = "yourname_lab3";

const char* light_topic = "ttpu/iot/yourname/sensors/light";
const char* button_topic = "ttpu/iot/yourname/events/button";

const int red_led = 26;
const int green_led = 27;
const int blue_led = 14;
const int yellow_led = 12;
const int button_pin = 25;
const int light_pin = 33;

WiFiClient espClient;
PubSubClient mqtt_client(espClient);

unsigned long lastPublishTime = 0;
const long publishInterval = 5000;

int lastButtonState = LOW;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

void connectWiFi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void connectMQTT() {
  while (!mqtt_client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqtt_client.connect(client_id, mqtt_username, mqtt_password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt_client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  connectWiFi();
  mqtt_client.setServer(mqtt_broker, mqtt_port);

  pinMode(red_led, OUTPUT);
  pinMode(green_led, OUTPUT);
  pinMode(blue_led, OUTPUT);
  pinMode(yellow_led, OUTPUT);
  pinMode(button_pin, INPUT);
  pinMode(light_pin, INPUT);

  configTime(0, 0, "pool.ntp.org");
  while (!time(nullptr)) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("Time synced");
}

void loop() {
  if (!mqtt_client.connected()) {
    connectMQTT();
  }
  mqtt_client.loop();

  unsigned long currentMillis = millis();
  if (currentMillis - lastPublishTime >= publishInterval) {
    lastPublishTime = currentMillis;

    int lightValue = analogRead(light_pin);
    time_t now = time(nullptr);

    JsonDocument doc;
    doc["light"] = lightValue;
    doc["timestamp"] = now;

    char buffer[256];
    serializeJson(doc, buffer);

    mqtt_client.publish(light_topic, buffer);
    Serial.print("Published to ");
    Serial.print(light_topic);
    Serial.print(": ");
    Serial.println(buffer);
  }

  int reading = digitalRead(button_pin);
  if (reading != lastButtonState) {
    lastDebounceTime = currentMillis;
  }

  if ((currentMillis - lastDebounceTime) > debounceDelay) {
    if (reading != lastButtonState) {
      lastButtonState = reading;

      time_t now = time(nullptr);

      JsonDocument doc;
      doc["event"] = (reading == HIGH) ? "PRESSED" : "RELEASED";
      doc["timestamp"] = now;

      char buffer[256];
      serializeJson(doc, buffer);

      mqtt_client.publish(button_topic, buffer);
      Serial.print("Published to ");
      Serial.print(button_topic);
      Serial.print(": ");
      Serial.println(buffer);
    }
  }
}
