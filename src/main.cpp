#include <M5Unified.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_SHT31.h>
#include "secrets.h"

// ENV3 Unit sensors (connected via I2C)
Adafruit_SHT31 sht31;
Adafruit_BMP280 bmp;

// WiFi and MQTT clients
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Timing
unsigned long lastPublishTime = 0;
const unsigned long PUBLISH_INTERVAL = 10000; // 10 seconds

void setupWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  
  Serial.begin(115200);
  delay(1000);
  Serial.println("M5Stack ATOM S3 + ENV3 Unit");
  
  // Initialize I2C for ENV3 Unit
  Wire.begin(2, 1); // SDA=GPIO2, SCL=GPIO1 for ATOM S3
  
  // Initialize SHT31 sensor (Temperature and Humidity)
  if (!sht31.begin(0x44)) {
    Serial.println("Could not find SHT31 sensor!");
    while (1) delay(10);
  }
  Serial.println("SHT31 sensor initialized");
  
  // Initialize BMP280 sensor (Pressure and Temperature)
  if (!bmp.begin(0x76)) {
    Serial.println("Could not find BMP280 sensor!");
    while (1) delay(10);
  }
  Serial.println("BMP280 sensor initialized");
  
  // Configure BMP280
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                  Adafruit_BMP280::SAMPLING_X2,
                  Adafruit_BMP280::SAMPLING_X16,
                  Adafruit_BMP280::FILTER_X16,
                  Adafruit_BMP280::STANDBY_MS_500);
  
  // Setup WiFi
  setupWiFi();
  
  // Setup MQTT
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  
  Serial.println("Setup complete!");
}

void loop() {
  M5.update();
  
  // Ensure MQTT connection
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();
  
  // Publish sensor data every 10 seconds
  unsigned long currentTime = millis();
  if (currentTime - lastPublishTime >= PUBLISH_INTERVAL) {
    lastPublishTime = currentTime;
    
    // Read sensor data
    float temperature = sht31.readTemperature();
    float humidity = sht31.readHumidity();
    float pressure = bmp.readPressure() / 100.0F; // Convert Pa to hPa
    
    // Check if readings are valid
    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("Failed to read from SHT31 sensor!");
      return;
    }
    
    // Create JSON payload
    char payload[256];
    snprintf(payload, sizeof(payload), 
             "{\"temperature\":%.2f,\"humidity\":%.2f,\"pressure\":%.2f}",
             temperature, humidity, pressure);
    
    // Publish to MQTT
    if (mqttClient.publish(MQTT_TOPIC, payload)) {
      Serial.println("Data published:");
      Serial.println(payload);
    } else {
      Serial.println("Failed to publish data");
    }
  }
}
