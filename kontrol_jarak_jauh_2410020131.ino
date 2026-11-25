
#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

// ====== KONFIGURASI (ubah bagian ini) ======
const char* WIFI_SSID   = "Iphone (2)";
const char* WIFI_PASS   = "PASSWORD_WIFI";
const char* MQTT_SERVER = "172.20.10.6";   // IP Mac
const int   MQTT_PORT   = 1883;

#define DHT_PIN    4
#define DHT_TYPE   DHT22     // ganti ke DHT11 jika memakai DHT11
#define RELAY_PIN  26
#define PWM_PIN    2

const char* TOPIC_SUHU  = "iot/2410020131/sensor/suhu";
const char* TOPIC_RELAY = "iot/2410020131/aktuator/relay";
const char* TOPIC_PWM   = "iot/2410020131/aktuator/pwm";

const unsigned long INTERVAL_KIRIM = 3000;  // 3 detik
// ===========================================

WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHT_PIN, DHT_TYPE);
unsigned long terakhirKirim = 0;

void connectWiFi() {
  Serial.print("Menghubungkan WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi terhubung, IP: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  message.trim();
  Serial.printf("Pesan [%s]: %s\n", topic, message.c_str());

  if (String(topic) == TOPIC_RELAY) {
    if (message == "ON") {
      digitalWrite(RELAY_PIN, HIGH);
    } else if (message == "OFF") {
      digitalWrite(RELAY_PIN, LOW);
    }
  } else if (String(topic) == TOPIC_PWM) {
    int nilai = constrain(message.toInt(), 0, 255);
    ledcWrite(PWM_PIN, nilai);
  }
}

void connectMQTT() {
  while (!client.connected()) {
    Serial.print("Menghubungkan MQTT...");
    if (client.connect("ESP32-2410020131")) {
      Serial.println("terhubung");
      client.subscribe(TOPIC_RELAY);
      client.subscribe(TOPIC_PWM);
    } else {
      Serial.printf("gagal (rc=%d), coba lagi 3 detik\n", client.state());
      delay(3000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  ledcAttach(PWM_PIN, 5000, 8);   // pin, frekuensi 5 kHz, resolusi 8 bit (0-255)
  ledcWrite(PWM_PIN, 0);
  dht.begin();
  connectWiFi();
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!client.connected()) connectMQTT();
  client.loop();

  unsigned long sekarang = millis();
  if (sekarang - terakhirKirim >= INTERVAL_KIRIM) {
    terakhirKirim = sekarang;
    float suhu = dht.readTemperature();
    if (isnan(suhu)) {
      Serial.println("Gagal membaca sensor DHT");
    } else {
      client.publish(TOPIC_SUHU, String(suhu, 1).c_str());
      Serial.printf("Publish suhu: %.1f C\n", suhu);
    }
  }
} 