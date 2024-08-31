#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#define SSID "your_SSID"
#define PASSWORD "your_PASSWORD"
#define BUTTON_PIN 5
#define ENDPOINT "http://your_endpoint_here"  // Replace with your actual endpoint

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

void sendHttpRequestIfButtonPressedTask(void* parameter) {
    bool lastButtonState = HIGH;  // Assume button is not pressed initially
    while (true) {
        bool isButtonPressed = digitalRead(BUTTON_PIN) == LOW;  // Assuming LOW means pressed
        if (isButtonPressed && lastButtonState == HIGH) {  // Button was pressed now but was not pressed before
            // Button press detected, send HTTP request
            time_t now;
            struct tm timeinfo;
            time(&now);
            localtime_r(&now, &timeinfo);
            char timeStr[64];
            strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);

            String payload = String("{\"uuid\":\"") + cachedUUID + "\", \"time\":\"" + timeStr + "\", \"button\":\"Pressed\"}";

            if (WiFi.status() == WL_CONNECTED) {
                HTTPClient http;
                http.begin(ENDPOINT);  // Initialize HTTP connection
                http.addHeader("Content-Type", "application/json");

                int httpResponseCode = http.POST(payload);  // Send POST request
                if (httpResponseCode > 0) {
                    Serial.printf("HTTP Response code: %d\n", httpResponseCode);
                } else {
                    Serial.printf("Error sending POST: %s\n", http.errorToString(httpResponseCode).c_str());
                }
                http.end();
            } else {
                Serial.println("WiFi not connected, cannot send data.");
            }
        }
        
        lastButtonState = isButtonPressed;  // Update the button state
        vTaskDelay(100 / portTICK_PERIOD_MS);  // Check every 100 ms
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    wifiSemaphore = xSemaphoreCreateBinary();
    timeSemaphore = xSemaphoreCreateBinary();
    uuidSemaphore = xSemaphoreCreateBinary();

    xTaskCreate(initWifiTask, "WiFi Connection Task", 4096, NULL, 1, NULL);
    xTaskCreate(setupTimeTask, "Time Setup Task", 4096, NULL, 1, NULL);
    xTaskCreate(generateUUIDTask, "UUID Generation Task", 4096, NULL, 1, NULL);
    xTaskCreate(sendHttpRequestIfButtonPressedTask, "HTTP Request Task", 4096, NULL, 1, NULL);
}

void loop() {
    // Nothing to do here, everything is handled by FreeRTOS tasks
}
