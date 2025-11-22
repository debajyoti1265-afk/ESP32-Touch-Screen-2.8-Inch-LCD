#include <SPI.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

TFT_eSPI tft = TFT_eSPI();

// --- Configuration ---
// Replace with your network credentials
const char* ssid = "Your SSID";
const char* password = "Your Password";

// Replace with your OpenWeatherMap API key, city, and country code
String openWeatherMapApiKey = "Your API Key";
String city = "Amsterdam";
String countryCode = "NL"; // e.g., "IN", "US", "UK"

// OpenWeatherMap API URL
String serverName = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey + "&units=metric";

unsigned long lastTime = 0;
// Update interval in milliseconds (e.g., 10 minutes)
const unsigned long timerDelay = 600000; 

// Touchscreen pins
#define XPT2046_IRQ 36   // T_IRQ
#define XPT2046_MOSI 32  // T_DIN
#define XPT2046_MISO 39  // T_OUT
#define XPT2046_CLK 25   // T_CLK
#define XPT2046_CS 33    // T_CS

SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define FONT_SIZE 2

// Function Declarations
void connectToWifi();
String httpGETRequest(const char* serverName);
void parseAndDisplayJson(String jsonBuffer);

void setup() {
  Serial.begin(115200);
  
  // Start the SPI for the touchscreen and init the touchscreen
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  touchscreen.setRotation(1); // Set the Touchscreen rotation in landscape mode

  // Start the tft display
  tft.init();
  tft.setRotation(1); // Set the TFT display rotation in landscape mode
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  
  tft.drawCentreString("Connecting to WiFi...", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, FONT_SIZE);
  connectToWifi();
  lastTime = millis(); // Initialize the timer after connection
}

void loop() {
  // Update weather data periodically
  if ((millis() - lastTime) > timerDelay) {
    if (WiFi.status() == WL_CONNECTED) {
      String payload = httpGETRequest(serverName.c_str());
      parseAndDisplayJson(payload);
    } else {
      tft.fillScreen(TFT_RED);
      tft.drawCentreString("WiFi Disconnected!", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, FONT_SIZE);
      connectToWifi(); // Try to reconnect
    }
    lastTime = millis();
  }
}

void connectToWifi() {
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected.");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    tft.fillScreen(TFT_BLACK);
    tft.drawCentreString("WiFi Connected!", SCREEN_WIDTH / 2, 20, FONT_SIZE);
  } else {
    Serial.println("\nFailed to connect to WiFi.");
    tft.fillScreen(TFT_RED);
    tft.drawCentreString("WiFi Connection Failed!", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, FONT_SIZE);
  }
}

String httpGETRequest(const char* serverName) {
  HTTPClient http;
  http.begin(serverName);
  int httpResponseCode = http.GET();

  String payload = "{}";
  if (httpResponseCode > 0) {
    payload = http.getString();
  } else {
    Serial.printf("Error in HTTP request: %d\n", httpResponseCode);
  }
  http.end();
  return payload;
}

void parseAndDisplayJson(String jsonBuffer) {
  // Buffer size estimation can be optimized using the ArduinoJson Assistant
  DynamicJsonDocument doc(2048); 
  DeserializationError error = deserializeJson(doc, jsonBuffer);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    tft.fillScreen(TFT_RED);
    tft.drawCentreString("JSON Parsing Failed!", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, FONT_SIZE);
    return;
  }

  // Extracting data
  JsonObject main = doc["main"];
  float temp = main["temp"];
  int humidity = main["humidity"];
  JsonObject weather = doc["weather"][0]; // Get the first weather description object
  String weatherDescription = weather["description"];
  String cityName = doc["name"];

  // Display results on TFT
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(FONT_SIZE);
  int centerX = SCREEN_WIDTH / 2;
  int textY = 40;

  tft.drawCentreString("Weather in " + cityName, centerX, textY, 4); // Larger font for city
  
  textY += 60;
  tft.drawCentreString("Temp: " + String(temp, 1) + " C", centerX, textY, 2);

  textY += 30;
  tft.drawCentreString("Humidity: " + String(humidity) + " %", centerX, textY, 2);

  textY += 30;
  tft.drawCentreString("Cond: " + weatherDescription, centerX, textY, 2);
}
