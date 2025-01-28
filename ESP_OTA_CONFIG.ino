#include <WiFi.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <Update.h>

// Wi-Fi and ThingsBoard Credentials
const char* ssid = "AI-iot";
const char* password = "Aieraiot@123";
const char* mqtt_server = "thingsboard.cloud";
const char* access_token = "Sg9zhTMTCBvUOPW7xwwP";

WiFiClient espClient;
PubSubClient client(espClient);

void handleSharedAttributes(char* topic, byte* payload, unsigned int length) {
    String message = String((char*)payload).substring(0, length);
    Serial.println("Received shared attribute update: " + message);

    // Extract firmware URL from the message
    int urlIndex = message.indexOf("fw_url");
    if (urlIndex != -1) {
        int start = message.indexOf(":", urlIndex) + 2;
        int end = message.indexOf("\"", start);
        String firmwareUrl = message.substring(start, end);
        performOTA(firmwareUrl);
    }
}

void performOTA(String firmwareUrl) {
    HTTPClient http;
    http.begin(firmwareUrl);
    int httpCode = http.GET();

    if (httpCode == 200) {
        int contentLength = http.getSize();
        if (Update.begin(contentLength)) {
            WiFiClient& updateStream = http.getStream();
            size_t written = Update.writeStream(updateStream);

            if (Update.end() && Update.isFinished()) {
                Serial.println("OTA Update Complete. Rebooting...");
                ESP.restart();
            } else {
                Serial.println("OTA Update Failed!");
            }
        } else {
            Serial.println("Not Enough Space for Update!");
        }
    } else {
        Serial.println("Firmware URL Not Found or Unreachable!");
    }
    http.end();
}

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to Wi-Fi...");
    }
    Serial.println("Connected to Wi-Fi");

    client.setServer(mqtt_server, 1883);
    client.setCallback(handleSharedAttributes);

    while (!client.connected()) {
        if (client.connect("OTA_Device", access_token, NULL)) {
            Serial.println("Connected to ThingsBoard");
            client.subscribe("v1/devices/me/attributes"); // Subscribe to shared attributes
        } else {
            delay(1000);
        }
    }
}

void loop() {
    client.loop();
}
