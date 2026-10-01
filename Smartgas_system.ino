#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include <DHT.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// WiFi
#define WIFI_SSID "daher"
#define WIFI_PASSWORD "100200300"

// Firebase
#define API_KEY "AIzaSyBMEPp7mA155eFLOU9BHraMjTtQGyG06BU"
#define DATABASE_URL "https://smartfiresystem-587aa-default-rtdb.firebaseio.com/"
#define BOT_TOKEN "8640887791:AAG_Y4gUATbfrQ6AafLUUzAcEPzdG5sfy0A"

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

bool alertSent = false;
// DHT11
#define DHTPIN D4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Output Pins
#define LED_PIN D5
#define BUZZER_PIN D6



FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool signupOK = false;

// Thresholds
int gasThreshold = 100;
float tempThreshold = 25.0;

void setup()
{
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(LED_PIN, LOW);

  dht.begin();

  // WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.println("WiFi Connected");
  client.setInsecure();

  // Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", ""))
  {
    Serial.println("Firebase SignUp OK");
    signupOK = true;
  }
  else
  {
    Serial.println(config.signer.signupError.message.c_str());
  }

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop()
{
  int gasValue = analogRead(A0);

  float temperature = dht.readTemperature();

  if (isnan(temperature))
  {
    Serial.println("DHT Error");
    delay(2000);
    return;
  }
String statusText;

if (gasValue > gasThreshold && temperature > tempThreshold)
{
  statusText = "Danger";

  digitalWrite(LED_PIN, HIGH);
  tone(BUZZER_PIN, 2000);

  if (!alertSent)
  {
    String message = "⚠️ FIRE & GAS ALERT\n\n";
    message += "Temperature: " + String(temperature) + " C\n";
    message += "Gas Value: " + String(gasValue) + "\n";
    message += "Status: DANGER";

    bot.sendMessage("2053909849", message, "");

    Serial.println("Telegram Alert Sent");

    alertSent = true;
  }
}
else
{
  statusText = "Safe";
  digitalWrite(LED_PIN, LOW);
  noTone(BUZZER_PIN);
  alertSent = false;
}
  

  // Alert Logic
  if (gasValue > gasThreshold && temperature > tempThreshold)
  {
    statusText = "Danger";

    digitalWrite(LED_PIN, HIGH);
    tone(BUZZER_PIN, 2000);
  }
  else
  {
    digitalWrite(LED_PIN, LOW);
    noTone(BUZZER_PIN);
  }

  Serial.println("---------------");
  Serial.print("Gas: ");
  Serial.println(gasValue);

  Serial.print("Temperature: ");
  Serial.println(temperature);

  Serial.print("Status: ");
  Serial.println(statusText);
// Firebase Upload
  if (Firebase.ready() && signupOK)
  {
    Firebase.RTDB.setInt(&fbdo,  "/SensorData/Gas",   gasValue);

    Firebase.RTDB.setFloat(&fbdo,   "/SensorData/Temperature",    temperature);

    Firebase.RTDB.setString(&fbdo,    "/SensorData/Status", statusText);
  }
FirebaseJson historyData;

historyData.set("Gas", gasValue);
historyData.set("Temperature", temperature);
historyData.set("Status", statusText);

Firebase.RTDB.pushJSON(&fbdo, "/History", &historyData);
  delay(5000);
}