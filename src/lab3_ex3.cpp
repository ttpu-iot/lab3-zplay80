#include "Arduino.h"
#include "WiFi.h"
#include <ArduinoJson.h>
#include "PubSubClient.h"
#include <time.h>
#include <LiquidCrystal_I2C.h>

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_broker = "mqtt.iotserver.uz";
const int mqtt_port = 1883;
const char* mqtt_username = "given in telegram group";
const char* mqtt_password = "given in telegram group";
const char* client_id = "yourname_lab3";

const char* button_topic = "ttpu/iot/yourname/events/button";
const char* red_topic = "ttpu/iot/yourname/led/red";
const char* green_topic = "ttpu/iot/yourname/led/green";
const char* blue_topic = "ttpu/iot/yourname/led/blue";
const char* yellow_topic = "ttpu/iot/yourname/led/yellow";
const char* display_topic = "ttpu/iot/yourname/display";

const int red_led = 26;
const int green_led = 27;
const int blue_led = 14;
const int yellow_led = 12;
const int button_pin = 25;

LiquidCrystal_I2C lcd(0x27, 16, 2);

WiFiClient espClient;
PubSubClient mqtt_client(espClient);

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

void updateLCD(const String& text) {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  char buffer[20];
  strftime(buffer, sizeof(buffer), "%d/%m %H:%M:%S", &timeinfo);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(text.substring(0, 16));
  lcd.setCursor(0, 1);
  lcd.print(buffer);

  Serial.print("LCD Updated: Line1=");
  Serial.print(text.substring(0, 16));
  Serial.print(", Line2=");
  Serial.println(buffer);
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

  if (strcmp(topic, display_topic) == 0) {
    const char* txt = doc["text"];
    if (txt) {
      updateLCD(txt);
      Serial.print("[DISPLAY] Text: ");
      Serial.println(txt);
    }
  } else {
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
      mqtt_client.subscribe(display_topic);
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
  pinMode(button_pin, INPUT);

  digitalWrite(red_led, LOW);
  digitalWrite(green_led, LOW);
  digitalWrite(blue_led, LOW);
  digitalWrite(yellow_led, LOW);

  lcd.init();
  lcd.backlight();

  configTime(18000, 0, "pool.ntp.org");
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    Serial.println("NTP sync successful");
  } else {
    Serial.println("NTP sync failed");
  }
}

void loop() {
  if (!mqtt_client.connected()) {
    connectMQTT();
  }
  mqtt_client.loop();

  unsigned long currentMillis = millis();
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
