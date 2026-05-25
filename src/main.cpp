#include <Arduino.h>
#include <DHT.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// sensor pin and model
#define DHT_PIN   12
#define DHT_TYPE  DHT22
#define DEVICE_ID 1

// WiFi credentials and API endpoint
#define WIFI_SSID "<ssid>"
#define WIFI_PASS "<pass>"
#define API_URL   "https://<mock-api-url>/api/devices"

DHT dht(DHT_PIN, DHT_TYPE);
TFT_eSPI tft;

// connect to WiFi, block and show status on screen until connected
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 60);
  tft.print("Connecting WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

// top-left API activity indicator circle
#define INDICATOR_X 12
#define INDICATOR_Y 12
#define INDICATOR_R 8

// filled = sending data, outline = idle
void drawIndicator(bool filled) {
  if (filled) {
    tft.fillCircle(INDICATOR_X, INDICATOR_Y, INDICATOR_R, TFT_WHITE);
  } else {
    tft.fillCircle(INDICATOR_X, INDICATOR_Y, INDICATOR_R, TFT_BLACK);
    tft.drawCircle(INDICATOR_X, INDICATOR_Y, INDICATOR_R, TFT_WHITE);
  }
}

// PUT temperature and humidity to server, fill indicator while in progress
void sendData(float temperature, float humidity) {
  drawIndicator(true);

  // skip certificate verification for mock API
  WiFiClientSecure client;
  client.setInsecure();

  // build URL and send JSON body
  HTTPClient http;
  String url = String(API_URL) + "/" + DEVICE_ID;
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");

  String body = "{\"temp\":" + String(temperature, 2) + ",\"humid\":" + String(humidity, 2) + "}";
  http.PUT(body);
  http.end();

  drawIndicator(false);
}

void setup() {
  // init TFT display
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  // set backlight brightness via PWM (~40%)
  ledcSetup(0, 5000, 8);
  ledcAttachPin(4, 0);
  ledcWrite(0, 100);

  dht.begin();
  connectWiFi();
}

unsigned long lastSendTime = 0;

void loop() {
  // reconnect if WiFi dropped
  connectWiFi();

  // read sensor (random values while DHT22 is not connected)
  // float humidity    = dht.readHumidity();
  // float temperature = dht.readTemperature();
  float humidity    = random(500, 701) / 10.0;
  float temperature = random(250, 351) / 10.0;

  // send data to server every 10 seconds
  if (millis() - lastSendTime >= 10000) {
    sendData(temperature, humidity);
    lastSendTime = millis();
  }

  // refresh display
  tft.fillScreen(TFT_BLACK);
  drawIndicator(false);

  // IP address at top-right
  tft.setTextSize(2);
  tft.setTextDatum(TR_DATUM);
  tft.drawString(WiFi.localIP().toString(), 235, 5);
  tft.setTextDatum(TL_DATUM);

  // sensor readings or error message
  if (isnan(humidity) || isnan(temperature)) {
    tft.setTextSize(2);
    tft.setCursor(10, 60);
    tft.print("Sensor Error");
  } else {
    tft.setTextSize(3);
    tft.setCursor(10, 50);
    tft.printf("T: %.2f C", temperature);

    tft.setCursor(10, 85);
    tft.printf("H: %.2f %%", humidity);
  }

  delay(2000);
}
