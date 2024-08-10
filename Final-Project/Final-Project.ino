#include "Arduino.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ESP32Servo.h> 
// Thay thế bằng thông tin mạng và token của ThingsBoard
const char* ssid = "Bacninh"; // wifi
const char* password = "28101972"; // mk wifi
const char* thingsboardServer = "demo.thingsboard.io";
const char* accessToken = "eAlia0D8EOinIUuyuJjH";    // token của devide

#define DHTPIN 23     // Chân kết nối DHT11 (thay đổi nếu cần)
#define DHTTYPE DHT11   // Định nghĩa loại cảm biến DHT

#define WARTER_PUMP  2UL
#define SERVO_MOTOR 18UL

Servo myservo;
float humidity=0;
float temperature =0;
DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);


void auto_mode_main_task()
{
     if(temperature>=40)
     {
        digitalWrite(WARTER_PUMP,HIGH);
        send_water_pump_state(true);
     }else if(humidity<=40)
     {

      send_servo_motor_state(true);
     }
}

void send_water_pump_state(bool state) {
  String payload = "{";
  payload += "\"WaterPump\":"; payload += state ? "true" : "false";
  payload += "}";

  Serial.print("Gửi trạng thái WaterPump: ");
  Serial.println(payload);

  client.publish("v1/devices/me/telemetry", (char*) payload.c_str());
}

void send_servo_motor_state(bool state) {
  String payload = "{";
  payload += "\"ServoMoter\":"; payload += state ? "true" : "false";
  payload += "}";

  Serial.print("Gửi trạng thái ServoMoter: ");
  Serial.println(payload);

  client.publish("v1/devices/me/telemetry", (char*) payload.c_str());
}

void rpcCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Tin nhắn nhận được từ topic: ");
  Serial.println(topic);

  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("Nội dung: ");
  Serial.println(message);


  if (String(topic).startsWith("v1/devices/me/rpc/request/")) 
  {
    if (message.indexOf("turnOnWaterPump") != -1) {
      digitalWrite(WARTER_PUMP, HIGH);  
      Serial.println("WaterPump bật!");
      send_water_pump_state(true);          
    } else if (message.indexOf("turnOffWaterPump") != -1) 
    {
      digitalWrite(WARTER_PUMP, LOW);  
      Serial.println("WaterPump tắt!");
      send_water_pump_state(false);         
    }else if (message.indexOf("turnOnmotor") != -1)
     {
      myservo.write(180); 
      Serial.println("motor bật!");
      send_servo_motor_state(true);          
    } else if (message.indexOf("turnOffmotor") != -1) 
    {
      myservo.write(0);  
      Serial.println("motor tắt!");
      send_servo_motor_state(false);         
    }

  }
}


void setup() {
  Serial.begin(115200);
  dht.begin();
   
  pinMode(WARTER_PUMP,OUTPUT);
  myservo.attach(SERVO_MOTOR);
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
  auto_mode_main_task();
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Lỗi đọc từ cảm biến DHT!");
    return;
  }

  String payload = "{";

  payload += "\"temperature\":"; payload += temperature; payload += ",";
  payload += "\"humidity\":"; payload += humidity;

  payload += "}";

  Serial.print("Gửi payload: ");
  Serial.println(payload);
  client.publish("v1/devices/me/telemetry", (char*) payload.c_str());
  delay(2000); // Đợi 2 giây trước khi đọc lại
}
