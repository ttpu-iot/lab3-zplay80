#include "Arduino.h"
#include "WiFi.h"
#include <ArduinoJson.h>
#include "PubSubClient.h"

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_broker = "mqtt.iotserver.uz";
const int mqtt_port = 1883;
const char* mqtt_username = "given in telegram group";
const char* mqtt_password = "given in telegram group";
const char* client_id = "yourname_lab3";

const char* red_topic = "ttpu/iot/yourname/led/red";
const char* green_topic = "ttpu/iot/yourname/led/green";
const char* blue_topic = "ttpu/iot/yourname/led/blue";
const char* yellow_topic = "ttpu/iot/yourname/led/yellow";

const int red_led = 26;
const int green_led = 27;
const int blue_led = 14;
const int yellow_led = 12;

WiFiClient espClient;
PubSubClient mqtt_client(espClient);

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

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("[MQTT] Received on ");
  Serial.print(topic);
  Serial.print(": ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload, length);
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  const char* state = doc["state"];
  if (state) {
    int pin = -1;
    String led_name = "";
    if (strcmp(topic, red_topic) == 0) {
      pin = red_led;
      led_name = "Red";
    } else if (strcmp(topic, green_topic) == 0) {
      pin = green_led;
      led_name = "Green";
    } else if (strcmp(topic, blue_topic) == 0) {
      pin = blue_led;
      led_name = "Blue";
    } else if (strcmp(topic, yellow_topic) == 0) {
      pin = yellow_led;
      led_name = "Yellow";
    }

    if (pin != -1) {
      if (strcmp(state, "ON") == 0) {
        digitalWrite(pin, HIGH);
        Serial.print("[LED] ");
        Serial.print(led_name);
        Serial.println(" LED -> ON");
      } else if (strcmp(state, "OFF") == 0) {
        digitalWrite(pin, LOW);
        Serial.print("[LED] ");
        Serial.print(led_name);
        Serial.println(" LED -> OFF");
      }
    }
  }
}

void connectMQTT() {
  while (!mqtt_client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqtt_client.connect(client_id, mqtt_username, mqtt_password)) {
      Serial.println("connected");
      mqtt_client.subscribe(red_topic);
      mqtt_client.subscribe(green_topic);
      mqtt_client.subscribe(blue_topic);
      mqtt_client.subscribe(yellow_topic);
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
  mqtt_client.setCallback(mqttCallback);

  pinMode(red_led, OUTPUT);
  pinMode(green_led, OUTPUT);
  pinMode(blue_led, OUTPUT);
  pinMode(yellow_led, OUTPUT);

  digitalWrite(red_led, LOW);
  digitalWrite(green_led, LOW);
  digitalWrite(blue_led, LOW);
  digitalWrite(yellow_led, LOW);
}

void loop() {
  if (!mqtt_client.connected()) {
    connectMQTT();
  }
  mqtt_client.loop();
}
