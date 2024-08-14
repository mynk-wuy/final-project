#include "Arduino.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ESP32Servo.h>

// Thông tin mạng và token ThingsBoard
const char* ssid = "2107";
const char* password = "88888888";
const char* thingsboardServer = "demo.thingsboard.io";
const char* accessToken = "gBCwrjsULdwRMsWmU2F8";

#define DHTPIN 23
#define DHTTYPE DHT11

#define WATER_PUMP 2UL
#define SERVO_MOTOR 5UL
#define BTN_WATER_PUMP 16

bool waterpump_state = false;
bool last_btn_waterpump_state = HIGH;
bool auto_mode=false;

Servo myservo;
float humidity = 0;
float temperature = 0;

DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

void auto_mode_main_task() {
    if (humidity <= 50) {
        digitalWrite(WATER_PUMP, HIGH);
        send_water_pump_state(true);
    } else {
        digitalWrite(WATER_PUMP, LOW);
        send_water_pump_state(false);
    }

    if (temperature >= 40) {
        myservo.write(180);
        send_servo_motor_state(true);
    } else {
        myservo.write(0);
        send_servo_motor_state(false);
    }
}

void send_water_pump_state(bool state) {
    String payload = String("{\"WaterState\":") + (state ? "true" : "false") + "}";
    Serial.print("Gửi trạng thái WaterPump: ");
    Serial.println(payload);
    client.publish("v1/devices/me/attributes", payload.c_str());
}

void send_servo_motor_state(bool state) {
    String payload = String("{\"ServoState\":") + (state ? "true" : "false") + "}";
    Serial.print("Gửi trạng thái ServoMotor: ");
    Serial.println(payload);
    client.publish("v1/devices/me/attributes", payload.c_str());
}

void mode_state(bool state) {
    String payload = String("{\"ModeState\":") + (state ? "true" : "false") + "}";
    Serial.print("Gửi trạng thái mode: ");
    Serial.println(payload);
    client.publish("v1/devices/me/attributes", payload.c_str());
}
void rpcCallback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");

    String incomingMessage;
    for (int i = 0; i < length; i++) {
        incomingMessage += (char)payload[i];
    }
    Serial.println(incomingMessage);

    if (incomingMessage.indexOf("\"method\":\"setServostate\"") != -1) {
        if (incomingMessage.indexOf("\"params\":true") != -1) {
            myservo.write(180);
            Serial.println("Motor bật!");
            send_servo_motor_state(true);
        } else if (incomingMessage.indexOf("\"params\":false") != -1) {
            myservo.write(0);
            Serial.println("Motor tắt!");
            send_servo_motor_state(false);
        }
    }

    if (incomingMessage.indexOf("\"method\":\"setWaterstate\"") != -1) {
        if (incomingMessage.indexOf("\"params\":true") != -1) {
            digitalWrite(WATER_PUMP, HIGH);
            Serial.println("WaterPump bật!");
            send_water_pump_state(true);
        } else if (incomingMessage.indexOf("\"params\":false") != -1) {
            digitalWrite(WATER_PUMP, LOW);
            Serial.println("WaterPump tắt!");
            send_water_pump_state(false);
        }
    }
        if (incomingMessage.indexOf("\"method\":\"setmode\"") != -1) {
        if (incomingMessage.indexOf("\"params\":true") != -1) {
           auto_mode=true;
           mode_state(true);
        } else if (incomingMessage.indexOf("\"params\":false") != -1) {
           auto_mode=false;
            mode_state(false);

        }
    }
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
            client.setCallback(rpcCallback);
        } else {
            Serial.print("thất bại, rc=");
            Serial.print(client.state());
            Serial.println(" thử lại trong 5 giây");
            delay(5000);
        }
    }
}

void setup() {
    Serial.begin(115200);
    dht.begin();
    
    pinMode(WATER_PUMP, OUTPUT);
    digitalWrite(WATER_PUMP, LOW);
    pinMode(BTN_WATER_PUMP, INPUT_PULLUP);

    myservo.attach(SERVO_MOTOR);
    myservo.write(0); // Khởi tạo servo ở vị trí 0 độ

    setup_wifi();

    client.setServer(thingsboardServer, 1883);
    client.setCallback(rpcCallback);
    client.subscribe("v1/devices/me/rpc/request/+");
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
    
    bool btn_waterpump_state = digitalRead(BTN_WATER_PUMP);
    if (btn_waterpump_state == LOW && last_btn_waterpump_state == HIGH) {
        waterpump_state = !waterpump_state;
        digitalWrite(WATER_PUMP, waterpump_state ? HIGH : LOW);
        send_water_pump_state(waterpump_state);
        delay(50);
    }
    last_btn_waterpump_state = btn_waterpump_state;
    
    humidity = dht.readHumidity();;
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
    
    if(auto_mode==true)
    {
      auto_mode_main_task();
    }

    delay(2000); // Chờ 2 giây trước khi đọc lại
}
