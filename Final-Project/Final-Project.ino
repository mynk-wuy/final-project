
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// Thay thế bằng thông tin mạng và token của ThingsBoard
const char* ssid = "Bacninh"; // wifi
const char* password = "28101972"; // mk wifi
const char* thingsboardServer = "demo.thingsboard.io";
const char* accessToken = "eAlia0D8EOinIUuyuJjH";    // token của devide

#define DHTPIN 23     // Chân kết nối DHT11 (thay đổi nếu cần)
#define DHTTYPE DHT11   // Định nghĩa loại cảm biến DHT

DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
  dht.begin();

  // Kết nối Wi-Fi
  setup_wifi();
  
  // Kết nối MQTT
  client.setServer(thingsboardServer, 1883);
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Đang kết nối tới ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("Đã kết nối WiFi");
  Serial.println("Địa chỉ IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Đang kết nối tới MQTT...");
    if (client.connect("ESP32Client", accessToken, NULL)) {
      Serial.println("đã kết nối");
    } else {
      Serial.print("thất bại, rc=");
      Serial.print(client.state());
      Serial.println(" thử lại trong 5 giây");
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Lỗi đọc từ cảm biến DHT!");
    return;
  }

  String payload = "{";
  payload += "\"temperature\":"; payload += t; payload += ",";
  payload += "\"humidity\":"; payload += h;
  payload += "}";

  Serial.print("Gửi payload: ");
  Serial.println(payload);

  client.publish("v1/devices/me/telemetry", (char*) payload.c_str());

  delay(2000); // Đợi 2 giây trước khi đọc lại
}
