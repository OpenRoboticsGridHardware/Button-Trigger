#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#define SSID "your_SSID"
#define PASSWORD "your_PASSWORD"
#define BUTTON1_PIN 5
#define BUTTON2_PIN 6
#define ENDPOINT1 "http://your_endpoint1_here"  
#define ENDPOINT2 "http://your_endpoint2_here" 

String cachedUUID;
SemaphoreHandle_t wifiSemaphore;
SemaphoreHandle_t timeSemaphore;
SemaphoreHandle_t uuidSemaphore;

void generateUUID() {
    uint32_t random1 = esp_random();
    uint32_t random2 = esp_random();
    uint32_t random3 = esp_random();
    uint32_t random4 = esp_random();
    char uuid[37];
    snprintf(uuid, sizeof(uuid), "%08x-%04x-%04x-%04x-%08x%04x",
             random1,
             (random2 >> 16) & 0xFFFF,
             random2 & 0xFFFF,
             (random3 >> 16) & 0xFFFF,
             random3 & 0xFFFFFFFF,
             random4 & 0xFFFF);

    cachedUUID = String(uuid);
}

void initWifiTask(void* parameter) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PASSWORD);
    Serial.println("Connecting to Wi-Fi...");

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(1000);
    }

    Serial.println("Connected to Wi-Fi");
    xSemaphoreGive(wifiSemaphore);
    vTaskDelete(NULL);
}

void setupTimeTask(void* parameter) {
    xSemaphoreTake(wifiSemaphore, portMAX_DELAY);
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    setenv("TZ", "EET-2EEST,M3.5.0/3,M10.5.0/4", 1);
    tzset();
    Serial.println("Time synchronized");
    xSemaphoreGive(timeSemaphore);
    vTaskDelete(NULL);
}

void generateUUIDTask(void* parameter) {
    xSemaphoreTake(timeSemaphore, portMAX_DELAY);
    generateUUID();
    Serial.println("UUID generated");
    xSemaphoreGive(uuidSemaphore);
    vTaskDelete(NULL);
}

void sendHttpRequest1(String endpoint, String buttonState) {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    char timeStr[64];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);

    String payload = String("{\"uuid\":\"") + cachedUUID + "\", \"time\":\"" + timeStr + "\", \"button\":\"" + buttonState + "\"}";

    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(endpoint);  // Initialize HTTP connection
        http.addHeader("Content-Type", "application/json");

        int httpResponseCode = http.POST(payload);  // Send POST request
        if (httpResponseCode > 0) {
            Serial.printf("HTTP Response code from %s: %d\n", endpoint.c_str(), httpResponseCode);
        } else {
            Serial.printf("Error sending POST to %s: %s\n", endpoint.c_str(), http.errorToString(httpResponseCode).c_str());
        }
        http.end();
    } else {
        Serial.println("WiFi not connected, cannot send data.");
    }
}
void sendHttpRequest2(String endpoint, String buttonState) {
 
    String payload = String("{\"uuid\":\"") + cachedUUID + "\", \"time\":\"" + timeStr + "\", \"button\":\"" + buttonState + "\"}";
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(endpoint)
        http.addHeader("Content-Type", "application/json");

        int httpResponseCode = http.POST(payload);  
        if (httpResponseCode > 0) {
            Serial.printf("HTTP Response code from %s: %d\n", endpoint.c_str(), httpResponseCode);
        } else {
            Serial.printf("Error sending POST to %s: %s\n", endpoint.c_str(), http.errorToString(httpResponseCode).c_str());
        }
        http.end();
    } else {
        Serial.println("WiFi not connected, cannot send data.");
    }
}
void sendHttpRequestIfButtonPressedTask(void* parameter) {
    bool lastButton1State = HIGH; 
    bool lastButton2State = HIGH; 

    while (true) {
        bool isButton1Pressed = digitalRead(BUTTON1_PIN) == LOW;  
        bool isButton2Pressed = digitalRead(BUTTON2_PIN) == LOW;  

        if (isButton1Pressed && lastButton1State == HIGH) { 
            sendHttpRequest1(ENDPOINT1, "Button1 Pressed");
        }

        if (isButton2Pressed && lastButton2State == HIGH) { 
            sendHttpRequest2(ENDPOINT2, "Button2 Pressed");
        }
        lastButton1State = isButton1Pressed;  
        lastButton2State = isButton2Pressed;  
        vTaskDelay(100 / portTICK_PERIOD_MS);  
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(BUTTON1_PIN, INPUT_PULLUP);
    pinMode(BUTTON2_PIN, INPUT_PULLUP);

    wifiSemaphore = xSemaphoreCreateBinary();
    timeSemaphore = xSemaphoreCreateBinary();
    uuidSemaphore = xSemaphoreCreateBinary();

    xTaskCreate(initWifiTask, "WiFi Connection Task", 4096, NULL, 1, NULL);
    xTaskCreate(setupTimeTask, "Time Setup Task", 4096, NULL, 1, NULL);
    xTaskCreate(generateUUIDTask, "UUID Generation Task", 4096, NULL, 1, NULL);
    xTaskCreate(sendHttpRequestIfButtonPressedTask, "HTTP Request Task", 4096, NULL, 1, NULL);
}

void loop() {

}
