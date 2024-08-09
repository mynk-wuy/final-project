#include <WiFi.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include "DHT.h"

// khai báo loại dht và chân
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// WiFi
const char* ssid = "Your_SSID";
const char* password = "Your_PASSWORD";

// ThingsBoard
const char* mqtt_server = "demo.thingsboard.io"; // Thay bằng địa chỉ của server ThingsBoard
const char* access_token = "Your_Access_Token"; // Thay bằng Access Token của thiết bị trên ThingsBoard

WiFiClient espClient;
PubSubClient client(espClient);

// InfluxDB
const char* influxdb_url = "http://Your_InfluxDB_Server_IP:8086/write?db=Your_Database_Name";
const char* influxdb_token = "Your_InfluxDB_Token"; // Nếu cần thiết

void setup() {
  Serial.begin(115200);
  dht.begin();

  // Kết nối WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Cấu hình MQTT
  client.setServer(mqtt_server, 1883);
  while (!client.connected()) {
    if (client.connect("ESP32Client", access_token, NULL)) {
      Serial.println("Connected to ThingsBoard");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

void sendToThingsBoard(float temperature, float humidity) {
  String payload = "{";
  payload += "\"temperature\":"; payload += String(temperature); payload += ",";
  payload += "\"humidity\":"; payload += String(humidity);
  payload += "}";

  if (client.publish("v1/devices/me/telemetry", (char*) payload.c_str())) {
    Serial.println("Data sent to ThingsBoard: " + payload);
  } else {
    Serial.println("Failed to send data to ThingsBoard");
  }
}

void sendToInfluxDB(float temperature, float humidity) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String data = "weather_data temperature=" + String(temperature) + ",humidity=" + String(humidity);

    http.begin(influxdb_url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
    int httpResponseCode = http.POST(data);
    if (httpResponseCode > 0) {
      Serial.println("Data sent to InfluxDB: " + data);
    } else {
      Serial.println("Failed to send data to InfluxDB");
    }
    http.end();
  }
}

void loop() {
  delay(2000);
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  sendToThingsBoard(temperature, humidity);
  sendToInfluxDB(temperature, humidity);

  client.loop();
}
