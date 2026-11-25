"""Simulasi ESP32 - NIM 2410020131
- Kirim suhu (simulasi DHT22) setiap 3 detik
- Terima perintah relay (ON/OFF) -> GPIO 26
- Terima nilai PWM (0-255)       -> GPIO 2
"""
import time
import random
import paho.mqtt.client as mqtt

BROKER = "localhost"
PORT = 1883
NIM = "2410020131"

TOPIC_SUHU = f"iot/{NIM}/sensor/suhu"
TOPIC_RELAY = f"iot/{NIM}/aktuator/relay"
TOPIC_PWM = f"iot/{NIM}/aktuator/pwm"
INTERVAL = 3  # detik


def on_connect(client, userdata, flags, rc, properties=None):
    print("[MQTT] Terhubung ke broker")
    client.subscribe(TOPIC_RELAY)
    client.subscribe(TOPIC_PWM)


def on_message(client, userdata, msg):
    pesan = msg.payload.decode().strip()
    if msg.topic == TOPIC_RELAY:
        if pesan == "ON":
            print("[RELAY GPIO 26] ON  -> HIGH")
        elif pesan == "OFF":
            print("[RELAY GPIO 26] OFF -> LOW")
    elif msg.topic == TOPIC_PWM:
        try:
            nilai = max(0, min(255, int(float(pesan))))
            persen = round(nilai / 255 * 100)
            print(f"[PWM GPIO 2] nilai = {nilai} (kecerahan {persen}%)")
        except ValueError:
            print(f"[PWM] nilai tidak valid: {pesan}")


try:
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id=f"ESP32-{NIM}")
except AttributeError:
    client = mqtt.Client(client_id=f"ESP32-{NIM}")

client.on_connect = on_connect
client.on_message = on_message
client.connect(BROKER, PORT, 60)
client.loop_start()

try:
    while True:
        suhu = round(random.uniform(26.0, 32.0), 1)
        client.publish(TOPIC_SUHU, str(suhu))
        print(f"[SENSOR] Publish suhu: {suhu} C")
        time.sleep(INTERVAL)
except KeyboardInterrupt:
    print("\nBerhenti.")
    client.loop_stop()
    client.disconnect()
