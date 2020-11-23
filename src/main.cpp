#include <Arduino.h>
#include <DHT.h>
#include "ESP8266WiFi.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#include "credentials.h" // ATTENTION: look at credentials.sample.h file

#define PWR_PIN_RAIN D7

WiFiClient client;

Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

Adafruit_MQTT_Publish temperaturePub = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temperature", MQTT_QOS_1);
Adafruit_MQTT_Publish humidityPub = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/humidity", MQTT_QOS_1);
Adafruit_MQTT_Publish rainPercentPub = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/rainPercent", MQTT_QOS_1);

const int DHT22_PIN = D4;
DHT dht(DHT22_PIN, DHT22);

float umidade = 0;
float temperatura = 0;
float rainLevel = 0;
float rainPercent = 0.0;
float qtdReadings = 0;

void readWeather();
void readRainLevel();
void connectWiFi();
void checkWifi();
void connectMqtt();

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(2000);
  while (!Serial) { }

  pinMode(PWR_PIN_RAIN, OUTPUT);

  dht.begin();

  delay(1000);

  connectWiFi();

  delay(10);

}

void loop() {
  delay(500);

  checkWifi();

  connectMqtt();
  mqtt.processPackets(5000);

  delay(2000);

  readWeather();

  delay(2000);

  readRainLevel();

  delay(1000);

  // if (qtdReadings >= 5) {
  //   Serial.println("Indo dormir...");
  //   delay(10);
  //   ESP.deepSleep(3e+8); // 5 min
  //   // ESP.deepSleep(30e6); // 30 sec
  // }

  qtdReadings++;
}

void connectMqtt() {
  int8_t ret;
 
  if (mqtt.connected()) {
    return;
  }
 
  Serial.println("Conectando-se ao broker mqtt...");
 
  uint8_t num_tentativas = 5;
  
  while ((ret = mqtt.connect()) != 0) {
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Falha ao se conectar. Tentando se reconectar em 5 segundos.");
    mqtt.disconnect();
    delay(5000);
    num_tentativas--;
    if (num_tentativas == 0) {
      Serial.println("Seu ESP será resetado.");
      ESP.restart();
    }
  }
 
  Serial.println("Conectado ao broker com sucesso.");
}

void readRainLevel() {

  digitalWrite(PWR_PIN_RAIN, HIGH);

  delay(1000);

  rainLevel = analogRead(A0);
  rainPercent = 100 - ((rainLevel / 1024L) * 100L);

  Serial.println("Rain level: " + (String) rainLevel);
  Serial.print("Rain %: ");
  Serial.println(rainPercent);

  delay(2000);

  if (! rainPercentPub.publish(rainPercent)) {
    Serial.println("Falha ao enviar dados de percentual de chuva.");
  }

  digitalWrite(PWR_PIN_RAIN, LOW);

  delay (1000);
}

void readWeather() {
  
  delay(1000);

  float tempIni = temperatura;
  float humIni = umidade;

  temperatura = dht.readTemperature();
  umidade = dht.readHumidity();

  if (isnan(umidade) || isnan(temperatura)) {
    Serial.println("Failed to read from DHT sensor!");
    temperatura = tempIni;
    umidade = humIni;
    return;
  }

  Serial.print("Temperatura: ");
  Serial.println(temperatura);
  Serial.print("Humidade: ");
  Serial.println(umidade);

  if (! temperaturePub.publish(temperatura)) {
    Serial.println("Falha ao enviar dados de temperatura.");
  }

  delay(2000);

  if (! humidityPub.publish(umidade)) {
    Serial.println("Falha ao enviar dados de umidade do ar.");
  }

  delay (2000);
}

void checkWifi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(" Conexão não estabelecida. Reiniciando...");
    ESP.restart();
  } 
}

void connectWiFi() {

  delay(100);
  
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Conectando em ");
  Serial.print(WIFI_SSID);

  int timeout = 0;

  while (WiFi.status() != WL_CONNECTED && ++timeout <= 10) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    Serial.print("."); 
  }

  checkWifi();

  Serial.println("");
  Serial.print("Conectado em ");
  Serial.print(WIFI_SSID);
  Serial.print(" com o IP ");
  Serial.println(WiFi.localIP());
}
