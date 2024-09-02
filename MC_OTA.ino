#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>
#include <Preferences.h>  // Include the Preferences library for NVS

#define ssid "Kothuis"
#define password "D13h3r3s0rg!"

const char* updateInfoUrl = "http://ota.spain.vaneck.app/update";
const char* firmwareBaseUrl = "http://ota.spain.vaneck.app/static/";

Preferences preferences;  // Create a Preferences object
const char* nvsVersionKey = "firmware_ver";
int currentVersion = 7; // Default version if not found in NVS

void performOTA(const char* firmwareUrl);
void checkForUpdate();
void storeVersionInNVS(int version);
int getVersionFromNVS();

void setup() {
    Serial.begin(115200);

    preferences.begin("ota", false);  // Open NVS with the namespace "ota"
    currentVersion = getVersionFromNVS();

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("Connected to WiFi");

    // Check for updates
    checkForUpdate();
    delay(5000);
}

void checkForUpdate() {
    HTTPClient http;
    http.begin(updateInfoUrl);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();

        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (error) {
            Serial.println("Failed to parse JSON");
            http.end();
            return;
        }

        const char* filename = doc["filename"];
        int newVersion = doc["version"];

        Serial.println("Current Version: " + String(currentVersion));
        Serial.println("Available Version: " + String(newVersion));

        if (newVersion > currentVersion) {
            Serial.println("New version available. Starting OTA...");
            String firmwareUrl = String(firmwareBaseUrl) + filename;
            performOTA(firmwareUrl.c_str());
        } else {
            Serial.println("No new update available.");
        }

    } else {
        Serial.println("Failed to fetch update info. HTTP Code: " + String(httpCode));
    }

    http.end();
}

void performOTA(const char* firmwareUrl) {
    HTTPClient http;
    http.begin(firmwareUrl);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        int contentLength = http.getSize();
        bool canBegin = Update.begin(contentLength);

        if (canBegin) {
            Serial.println("Begin OTA...");
            WiFiClient& client = http.getStream();
            size_t written = Update.writeStream(client);

            if (written == contentLength) {
                Serial.println("Written: " + String(written) + " successfully");
            } else {
                Serial.println("Written only: " + String(written) + "/" + String(contentLength) + ". Retry?");
            }

            if (Update.end()) {
                Serial.println("OTA done!");
                if (Update.isFinished()) {
                    Serial.println("Update successfully completed. Rebooting.");
                    storeVersionInNVS(currentVersion + 1);  // Update version in NVS
                    ESP.restart();
                } else {
                    Serial.println("Update not finished? Something went wrong!");
                }
            } else {
                Serial.println("Error Occurred. Error #: " + String(Update.getError()));
            }

        } else {
            Serial.println("Not enough space to begin OTA");
        }
    } else {
        Serial.println("Can't connect to the server. HTTP Code: " + String(httpCode));
    }

    http.end();
}

void storeVersionInNVS(int version) {
    preferences.putInt(nvsVersionKey, version);
    Serial.println("Stored new version in NVS: " + String(version));
}

int getVersionFromNVS() {
    int version = preferences.getInt(nvsVersionKey, currentVersion);
    Serial.println("Retrieved version from NVS: " + String(version));
    return version;
}

void loop() {
    // Flash the LED when entering the loop
    // pinMode(ledPin, OUTPUT);  // Set the LED pin as output
    // digitalWrite(ledPin, HIGH);   // Turn the LED on
    // delay(500);                   // Wait for 500 ms
    // digitalWrite(ledPin, LOW);    // Turn the LED off
    // delay(500);                   // Wait for 500 ms

    // Leave empty or put your code here
}
