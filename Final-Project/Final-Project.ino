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
// const char* clientID = "ESP32_Client";

#define DHTPIN 23
#define DHTTYPE DHT11

#define LED_PIN 2        // Chân kết nối LED
#define BUTTON_LED_PIN 22     // Chân kết nối nút nhấn  

#define WATER_PUMP 2UL
#define SERVO_MOTOR 5UL
#define BTN_WATER_PUMP 16

bool ledState = false;   // Biến theo dõi trạng thái LED
bool lastButtonState = HIGH; // Trạng thái ban đầu của nút nhấn

bool waterpump_state = false;
bool last_btn_waterpump_state = HIGH;
bool auto_mode=false;

Servo myservo;

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

    if (incomingMessage.indexOf("\"method\":\"setLEDState\"") != -1) {
        if (incomingMessage.indexOf("\"params\":true") != -1) {
            ledState = true;
            digitalWrite(LED_PIN, HIGH);
            Serial.println("LED turned ON by ThingsBoard");
        } else if (incomingMessage.indexOf("\"params\":false") != -1) {
          ledState = false;
          digitalWrite(LED_PIN, LOW);
          Serial.println("LED turned OFF by ThingsBoard");
        }
        syncLedState(); // Đồng bộ hóa trạng thái LED sau khi nhận lệnh từ ThingsBoard
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
    
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    pinMode(BUTTON_LED_PIN, INPUT_PULLUP);
    // Đặt chân nút nhấn là đầu vào với điện trở kéo lên bên trong
    
    pinMode(WATER_PUMP, OUTPUT);
    digitalWrite(WATER_PUMP, LOW);
    pinMode(BTN_WATER_PUMP, INPUT_PULLUP);

    myservo.attach(SERVO_MOTOR);
    myservo.write(0); // Khởi tạo servo ở vị trí 0 độ

    setup_wifi();

    client.setServer(thingsboardServer, 1883);
    client.setCallback(rpcCallback);

    reconnect();

    client.subscribe("v1/devices/me/rpc/request/+");
    syncLedState(); // Đồng bộ hóa trạng thái LED khi kết nối
}

void loop() {
    if (!client.connected()) {
        reconnect();
        }
        syncLedState();

    bool buttonState = digitalRead(BUTTON_LED_PIN);
    // Bật/tắt LED khi nút nhấn được bấm
    if (buttonState == LOW && lastButtonState == HIGH) { // Nút nhấn được bấm
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState ? HIGH : LOW);
        syncLedState(); // Đồng bộ hóa trạng thái với ThingsBoard
        delay(50); // Trễ để chống rung
    }
    lastButtonState = buttonState;

    bool btn_waterpump_state = digitalRead(BTN_WATER_PUMP);
    if (btn_waterpump_state == LOW && last_btn_waterpump_state == HIGH) {
        waterpump_state = !waterpump_state;
        digitalWrite(WATER_PUMP, waterpump_state ? HIGH : LOW);
        send_water_pump_state(waterpump_state);
        delay(50);
    }
    last_btn_waterpump_state = btn_waterpump_state;
    
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
    
    if(auto_mode==true)
    {
      auto_mode_main_task();
    }

    client.loop();

    delay(2000); // Chờ 2 giây trước khi đọc lại
}

void syncLedState() {
  // Tạo chuỗi JSON để gửi trạng thái LED
  String payload = String("{\"LEDState\":") + (ledState ? "true" : "false") + "}";

  // Gửi trạng thái đến ThingsBoard
  client.publish("v1/devices/me/attributes", payload.c_str());

  Serial.print("Trạng thái LED đã được đồng bộ hóa: ");
  Serial.println(payload);
}
