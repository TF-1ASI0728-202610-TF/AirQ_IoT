#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <qrcode.h> 


const char* ssid = "Andres y Anthony";
const char* password = "15180309";
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_topic_pub = "airq/edge/raw"; 
const char* mqtt_topic_sub = "airq/edge/command";    

String nodeId = "02"; 
String macAddress = "00:1A:2B:3C:4D:" + nodeId;
String mqttClientId = "AirQ_SensorNode_" + nodeId;

// --- PINES DE HARDWARE REAL (6 LEDs) ---
const int LED_EXTRACTOR = 12; // Azul
const int LED_HEPA = 14;      // Naranja
const int LED_COOL = 27;      // Blanco
const int LED_DRY = 2;        // Morado
const int LED_DAMP_OPEN = 26; // Verde (Rejilla Abierta)
const int LED_DAMP_CLOSED = 33; // Rojo (Rejilla Cerrada / Aislado)
const int BTN_CAOS = 25;      // Botón Generador

float sim_co2 = 600.0;
float sim_pm25 = 5.0;
float sim_temp = 22.0;
float sim_hum = 45.0;

bool anomaly_co2 = false;
bool anomaly_pm25 = false;
bool anomaly_temp = false;
bool anomaly_hum = false;
bool last_btn_state = HIGH;

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastUpdate = 0;

void setup_wifi() {
  Serial.print("Conectando WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { 
    delay(500); 
    Serial.print("."); 
  }
  Serial.println(" Conectado!");
}

// --- ESCUCHAR COMANDOS DE LA IA ---
void callback(char* topic, byte* payload, unsigned int length) {
  String messageTemp;
  for (int i = 0; i < length; i++) { messageTemp += (char)payload[i]; }
  
  StaticJsonDocument<512> doc; 
  if (!deserializeJson(doc, messageTemp)) {
    digitalWrite(LED_EXTRACTOR, doc["extractor"] ? HIGH : LOW);
    digitalWrite(LED_HEPA, doc["hepa"] ? HIGH : LOW);
    digitalWrite(LED_COOL, doc["ac_cool"] ? HIGH : LOW);
    digitalWrite(LED_DRY, doc["ac_dry"] ? HIGH : LOW);
    
    // LOGICA HMI PARA LAS REJILLAS: Alternar Verde y Rojo
    bool isDamperOpen = doc["dampers_open"];
    digitalWrite(LED_DAMP_OPEN, isDamperOpen ? HIGH : LOW);
    digitalWrite(LED_DAMP_CLOSED, isDamperOpen ? LOW : HIGH);
  }
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect(mqttClientId.c_str())) {
      client.subscribe(mqtt_topic_sub); 
    } else { 
      delay(5000); 
    }
  }
}

// --- FUNCIÓN RENDERIZAR QR (CORREGIDA CON API NATIVA) ---
void renderQRCode() {
  Serial.println("\n\n=== CÓDIGO QR DEL DISPOSITIVO ===");
  Serial.println("MAC Address: " + macAddress + "\n");
  
  // Usamos las estructuras de configuración que vienen integradas en el ESP32
  esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG_DEFAULT();
  esp_qrcode_generate(&cfg, macAddress.c_str());
  
  Serial.println("=================================\n\n");
}

void setup() {
  Serial.begin(115200);
  delay(1000); 
  randomSeed(analogRead(0)); 
  
  pinMode(LED_EXTRACTOR, OUTPUT);
  pinMode(LED_HEPA, OUTPUT);
  pinMode(LED_COOL, OUTPUT);
  pinMode(LED_DRY, OUTPUT);
  pinMode(LED_DAMP_OPEN, OUTPUT);
  pinMode(LED_DAMP_CLOSED, OUTPUT);
  pinMode(BTN_CAOS, INPUT_PULLUP);

  // Estado Base Inicial: Ambiente ventilado (Verde ON, Rojo OFF)
  digitalWrite(LED_DAMP_OPEN, HIGH);
  digitalWrite(LED_DAMP_CLOSED, LOW);

  // Generamos el QR directamente en el monitor serie
  renderQRCode();

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  // 1. BOTÓN DE CAOS
  bool btn_state = digitalRead(BTN_CAOS);
  if (last_btn_state == HIGH && btn_state == LOW) {
    Serial.println("\n GENERANDO CAOS...");
    anomaly_co2 = random(0, 2);
    anomaly_pm25 = random(0, 2);
    anomaly_temp = random(0, 2);
    anomaly_hum = random(0, 2);
    if (!anomaly_co2 && !anomaly_pm25 && !anomaly_temp && !anomaly_hum) anomaly_co2 = true; 
    delay(200); 
  }
  last_btn_state = btn_state;

  // 2. FÍSICA SEPARADA
  if (digitalRead(LED_EXTRACTOR) == HIGH) {
    float tasa_limpieza = (digitalRead(LED_DAMP_OPEN) == HIGH) ? 8.0 : 3.0;
    if (sim_co2 > 600.0) sim_co2 -= tasa_limpieza; 
    if (sim_co2 <= 600.0) { sim_co2 = 600.0; anomaly_co2 = false; }
  } else if (anomaly_co2 && sim_co2 < 1600.0) {
    sim_co2 += (digitalRead(LED_DAMP_CLOSED) == HIGH) ? 6.0 : 3.0; 
  }

  if (digitalRead(LED_HEPA) == HIGH) {
    float tasa_hepa = (digitalRead(LED_DAMP_CLOSED) == HIGH) ? 1.0 : 0.4;
    if (sim_pm25 > 5.0) sim_pm25 -= tasa_hepa; 
    if (sim_pm25 <= 5.0) { sim_pm25 = 5.0; anomaly_pm25 = false; }
  } else if (anomaly_pm25 && sim_pm25 < 65.0) {
    sim_pm25 += 0.5;
  }

  if (digitalRead(LED_COOL) == HIGH) {
    if (sim_temp > 22.0) sim_temp -= 0.15;
    if (sim_temp <= 22.0) { sim_temp = 22.0; anomaly_temp = false; }
  } else if (anomaly_temp && sim_temp < 32.0) {
    sim_temp += 0.05;
  }

  if (digitalRead(LED_DRY) == HIGH) {
    if (sim_hum > 45.0) sim_hum -= 0.5;
    if (sim_hum <= 45.0) { sim_hum = 45.0; anomaly_hum = false; }
  } else if (anomaly_hum && sim_hum < 85.0) {
    sim_hum += 0.2;
  }

  // 3. ENVÍO DE TELEMETRÍA
  if (millis() - lastUpdate > 5000) {
    lastUpdate = millis();

    StaticJsonDocument<200> doc;
    doc["hardware_mac_id"] = macAddress;
    doc["temperature"] = String(sim_temp, 1);
    doc["humidity"] = String(sim_hum, 1);
    doc["co2"] = (int)sim_co2;
    doc["pm25"] = (int)sim_pm25;

    char jsonBuffer[512];
    serializeJson(doc, jsonBuffer);
    client.publish(mqtt_topic_pub, jsonBuffer);
    
    Serial.printf("CO2: %d | PM25: %d | Temp: %.1f | Hum: %.1f\n", 
                  (int)sim_co2, (int)sim_pm25, sim_temp, sim_hum);
  }
  delay(100); 
}