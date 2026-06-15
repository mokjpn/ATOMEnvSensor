#include <M5Unified.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include "QMP6988.h"
#include <Adafruit_SHT31.h>
#include "secrets.h"

// ENV3 Unit sensors (connected via I2C)
Adafruit_SHT31 sht31;
QMP6988 qmp6988;

// WiFi and MQTT clients
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Timing
unsigned long lastPublishTime = 0;
const unsigned long PUBLISH_INTERVAL = 10000; // 10 seconds
const unsigned long WIFI_CONNECT_TIMEOUT = 15000; // 15 seconds
const unsigned long WIFI_RETRY_INTERVAL = 30000; // 30 seconds
const unsigned long MQTT_RETRY_INTERVAL = 5000; // 5 seconds

unsigned long lastWiFiRetryTime = 0;
unsigned long lastMQTTRetryTime = 0;

bool connectWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < WIFI_CONNECT_TIMEOUT) {
    delay(500);
    Serial.print(".");
    M5.update();
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWiFi connection timed out");
    return false;
  }

  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  return true;
}

bool ensureWiFiConnected(unsigned long currentTime) {
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }

  if (currentTime - lastWiFiRetryTime < WIFI_RETRY_INTERVAL) {
    return false;
  }

  lastWiFiRetryTime = currentTime;
  Serial.println("WiFi disconnected; retrying");
  mqttClient.disconnect();
  return connectWiFi();
}

bool reconnectMQTT(unsigned long currentTime) {
  if (mqttClient.connected()) {
    return true;
  }

  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  if (currentTime - lastMQTTRetryTime < MQTT_RETRY_INTERVAL) {
    return false;
  }

  lastMQTTRetryTime = currentTime;
  Serial.print("Attempting MQTT connection...");

  if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
    Serial.println("connected");
    return true;
  }

  Serial.print("failed, rc=");
  Serial.println(mqttClient.state());
  return false;
}

void setup() {
  auto cfg = M5.config();
  cfg.serial_baudrate = 115200;
  M5.begin(cfg);

  delay(1000);
  Serial.println("M5Stack ATOM S3 + ENV3 Unit");
  
  // Initialize I2C for ENV3 Unit
  // Wire.begin(2, 1); // SDA=GPIO2, SCL=GPIO1 for ATOM S3
  Wire.begin(8, 5) ; // SDA=GPIO8, SCL=GPIO5 for ATOM S3 with ToUnit Base
  
  // Initialize SHT31 sensor (Temperature and Humidity)
  if (!sht31.begin(0x44)) {
    Serial.println("Could not find SHT31 sensor!");
    while (1) delay(10);
  }
  Serial.println("SHT31 sensor initialized");
  
  // Initialize QMP6988 sensor (Pressure and Temperature)
  if (!qmp6988.init()) {
    Serial.println("Could not find QMP6988 sensor!");
    while (1) delay(10);
  }
  Serial.println("QMP6988 sensor initialized");
  
  // Setup MQTT
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setSocketTimeout(5);

  // Setup WiFi
  connectWiFi();
  
  Serial.println("Setup complete!");
}

void loop() {
  M5.update();
  
  unsigned long currentTime = millis();

  // Keep network maintenance non-blocking so the device can recover after outages.
  if (ensureWiFiConnected(currentTime) && reconnectMQTT(currentTime)) {
    mqttClient.loop();
  }
  
  // Publish sensor data every 10 seconds
  if (currentTime - lastPublishTime >= PUBLISH_INTERVAL) {
    lastPublishTime = currentTime;

    if (WiFi.status() != WL_CONNECTED || !mqttClient.connected()) {
      Serial.println("Network unavailable; skipping publish");
      return;
    }
    
    // Read sensor data
    float temperature = sht31.readTemperature();
    float humidity = sht31.readHumidity();
    float pressure = qmp6988.calcPressure() / 100.0F; // Convert Pa to hPa
    
    // Check if readings are valid
    if (isnan(temperature) || isnan(humidity) || isnan(pressure)) {
      Serial.println("Failed to read from sensors!");
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
