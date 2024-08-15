#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#define SSID "your_SSID"
#define PASSWORD "your_PASSWORD"

#define BUTTON_PIN 5

AsyncWebServer server(932);
AsyncWebSocket ws("/ws");

String cachedUUID;
SemaphoreHandle_t wifiSemaphore;
SemaphoreHandle_t timeSemaphore;
SemaphoreHandle_t uuidSemaphore;
// Primul thread
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
// Al doilea Thread
void initWifiTask(void * parameter) {
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

// Utimul thread
void setupTimeTask(void * parameter) {
    xSemaphoreTake(wifiSemaphore, portMAX_DELAY);  
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    setenv("TZ", "EET-2EEST,M3.5.0/3,M10.5.0/4", 1);
    tzset();
    Serial.println("Time synchronized");
    xSemaphoreGive(timeSemaphore);  
    vTaskDelete(NULL);
}

void generateUUIDTask(void * parameter) {
    xSemaphoreTake(timeSemaphore, portMAX_DELAY);  
    generateUUID();
    Serial.println("UUID generated");
    xSemaphoreGive(uuidSemaphore);  
    vTaskDelete(NULL);
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.println("WebSocket client connected");
        xSemaphoreTake(uuidSemaphore, portMAX_DELAY);
        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);
        char timeStr[64];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
        bool isButtonPressed = digitalRead(BUTTON_PIN) == LOW;  // Assuming LOW means pressed
        String buttonState = isButtonPressed ? "Pressed" : "Not Pressed";
        String message = String("UUID: ") + cachedUUID + ", Time: " + timeStr + ", Button: " + buttonState;
        client->text(message);
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(BUTTON_PIN, INPUT_PULLUP);  // Aici cred ca nu merge 
    wifiSemaphore = xSemaphoreCreateBinary();
    timeSemaphore = xSemaphoreCreateBinary();
    uuidSemaphore = xSemaphoreCreateBinary();
    xTaskCreate(initWifiTask, "WiFi Connection Task", 4096, NULL, 1, NULL);
    xTaskCreate(setupTimeTask, "Time Setup Task", 4096, NULL, 1, NULL);
    xTaskCreate(generateUUIDTask, "UUID Generation Task", 4096, NULL, 1, NULL);
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);
    server.begin();
}

void loop() {

    ws.cleanupClients();
}
