#include "M5CoreS3.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_netif.h>
#include <lwip/ip4_addr.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

AsyncWebServer server(80);

#define COLOR_BG      CoreS3.Display.color565(15, 23, 42)
#define COLOR_CARD    CoreS3.Display.color565(30, 41, 59)
#define COLOR_ACCENT  CoreS3.Display.color565(99, 102, 241)
#define COLOR_TEXT    CoreS3.Display.color565(248, 250, 252)
#define COLOR_SUBTEXT CoreS3.Display.color565(148, 163, 184)
#define COLOR_SUCCESS CoreS3.Display.color565(34, 197, 94)
#define COLOR_WARNING CoreS3.Display.color565(251, 146, 60)

unsigned long lastUpdate = 0;
const unsigned long UPDATE_INTERVAL = 2000;

struct LogEntry {
    String message;
    uint16_t color;
};

#define MAX_LOGS 10
LogEntry logs[MAX_LOGS];
int logIndex = 0;

struct ClientInfo {
    uint8_t mac[6];
    IPAddress ip;
    bool active;
};

#define MAX_CLIENTS 10
ClientInfo clients[MAX_CLIENTS];
int clientCount = 0;

const char* externalAPICheckURL = "http://192.168.4.2:3000/check";
const char* externalAPIAuthURL = "http://192.168.4.2:3000/auth";

void addLog(String message, uint16_t color = COLOR_TEXT) {
    logs[logIndex].message = message;
    logs[logIndex].color = color;
    logIndex = (logIndex + 1) % MAX_LOGS;
}

void drawCard(int x, int y, int w, int h) {
    CoreS3.Display.fillRoundRect(x, y, w, h, 12, COLOR_CARD);
}

String macToString(uint8_t* mac) {
    char macStr[18];
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}

void updateClientList() {
    wifi_sta_list_t stationList;
    esp_wifi_ap_get_sta_list(&stationList);
    
    esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    esp_netif_pair_mac_ip_t clients_list[10];
    int num_clients = 10;
    
    esp_netif_dhcps_get_clients_by_mac(netif, num_clients, clients_list);

    for (int i = 0; i < clientCount; i++) {
        clients[i].active = false;
    }
    
    for (int i = 0; i < stationList.num; i++) {
        wifi_sta_info_t station = stationList.sta[i];
        
        IPAddress clientIP(0, 0, 0, 0);
        for (int k = 0; k < num_clients; k++) {
            if (memcmp(clients_list[k].mac, station.mac, 6) == 0) {
                clientIP = IPAddress(clients_list[k].ip.addr);
                break;
            }
        }
        
        bool found = false;
        for (int j = 0; j < clientCount; j++) {
            if (memcmp(clients[j].mac, station.mac, 6) == 0) {
                clients[j].active = true;
                if (clientIP != IPAddress(0, 0, 0, 0)) {
                    clients[j].ip = clientIP;
                }
                found = true;
                break;
            }
        }
        
        if (!found && clientCount < MAX_CLIENTS) {
            memcpy(clients[clientCount].mac, station.mac, 6);
            clients[clientCount].ip = clientIP;
            clients[clientCount].active = true;
            
            String logMsg = "Connected: " + (clientIP != IPAddress(0, 0, 0, 0) ? clientIP.toString() : macToString(station.mac));
            addLog(logMsg, COLOR_SUCCESS);
            
            Serial.println("\n=== New client connected ===");
            Serial.println("MAC: " + macToString(clients[clientCount].mac));
            Serial.println("IP:  " + clients[clientCount].ip.toString());
            
            clientCount++;
        }
    }
    
    for (int i = 0; i < clientCount; i++) {
        if (!clients[i].active) {
            String logMsg = "Disconnected: " + (clients[i].ip != IPAddress(0, 0, 0, 0) ? clients[i].ip.toString() : macToString(clients[i].mac));
            addLog(logMsg, COLOR_WARNING);
            
            Serial.println("\n=== Client disconnected ===");
            Serial.println("MAC: " + macToString(clients[i].mac));
            Serial.println("IP:  " + clients[i].ip.toString());
            
            for (int j = i; j < clientCount - 1; j++) {
                clients[j] = clients[j + 1];
            }
            clientCount--;
            i--;
        }
    }
}

void drawInterface() {
    CoreS3.Display.startWrite();
    CoreS3.Display.fillScreen(COLOR_BG);

    int startY = 20;
    drawCard(10, startY, 140, 60);
    CoreS3.Display.setTextFont(2);
    CoreS3.Display.setTextColor(COLOR_ACCENT);
    CoreS3.Display.setTextDatum(TC_DATUM);
    CoreS3.Display.drawString("MyHouseOS", 80, startY + 15);
    CoreS3.Display.setTextColor(COLOR_TEXT);
    CoreS3.Display.setTextFont(1);
    CoreS3.Display.drawString("192.168.4.1", 80, startY + 40);

    drawCard(160, startY, 150, 60);
    CoreS3.Display.setTextFont(2);
    CoreS3.Display.setTextColor(COLOR_SUBTEXT);
    CoreS3.Display.drawString("CLIENTS", 235, startY + 10);

    int activeClients = 0;
    for (int i = 0; i < clientCount; i++) {
        if (clients[i].active) activeClients++;
    }

    CoreS3.Display.setTextFont(4);
    CoreS3.Display.setTextColor(activeClients > 0 ? COLOR_SUCCESS : COLOR_TEXT);
    CoreS3.Display.drawString(String(activeClients), 235, startY + 33);

    int logY = 95;
    drawCard(10, logY, 300, 135);
    
    CoreS3.Display.setTextFont(2);
    CoreS3.Display.setTextColor(COLOR_ACCENT);
    CoreS3.Display.setTextDatum(TL_DATUM);
    CoreS3.Display.drawString("ACTIVITY LOGS", 20, logY + 10);
    
    CoreS3.Display.setTextFont(1);
    int yPos = logY + 35;
    
    for (int i = 0; i < 8; i++) {
        int idx = (logIndex - 1 - i + MAX_LOGS) % MAX_LOGS;
        if (logs[idx].message.length() > 0) {
            CoreS3.Display.setTextColor(logs[idx].color);
            String msg = logs[idx].message;
            if (msg.length() > 40) msg = msg.substring(0, 40) + "...";
            CoreS3.Display.drawString(msg, 20, yPos);
            
            yPos += 12;
        }
    }
    
    CoreS3.Display.endWrite();
}

void setup() {
    auto cfg = M5.config();
    CoreS3.begin(cfg);
    
    Serial.begin(115200);

    CoreS3.Display.fillScreen(COLOR_BG);
    CoreS3.Display.setTextFont(2);
    CoreS3.Display.setTextDatum(MC_DATUM);
    CoreS3.Display.setTextColor(COLOR_SUBTEXT);
    CoreS3.Display.drawString("STARTING...", 160, 120);

    IPAddress localIP(192, 168, 4, 1);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);

    WiFi.softAPConfig(localIP, gateway, subnet);
    WiFi.softAP("MyHouseOS", "12345678");

    Serial.println("\n=== MyHouseOS started ===");
    Serial.println("SSID: MyHouseOS");
    Serial.println("IP AP: " + WiFi.softAPIP().toString());
    
    server.on("/link", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
            
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, data, len);
            
            if (error) {
                Serial.println("JSON Parsing Error");
                addLog("Link: JSON Error", COLOR_WARNING);
                request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
                return;
            }

            if (!doc.containsKey("id") || doc["id"].as<String>().length() == 0) {
                addLog("Link: Empty ID", COLOR_WARNING);
                request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing or empty ID\"}");
                return;
            }
            
            String espId = doc["id"];
            
            addLog("Link request: " + espId, COLOR_ACCENT);
            Serial.println("\n=== Link request from: " + espId + " ===");
            
            CoreS3.Display.fillScreen(COLOR_BG);
            CoreS3.Display.setTextFont(2);
            CoreS3.Display.setTextColor(COLOR_ACCENT);
            CoreS3.Display.setTextDatum(MC_DATUM);
            CoreS3.Display.drawString("PAIRING REQUEST", 160, 60);
            
            CoreS3.Display.setTextColor(COLOR_TEXT);
            CoreS3.Display.setTextFont(3);
            CoreS3.Display.drawString(espId, 160, 100);
            
            CoreS3.Display.setTextFont(2);
            CoreS3.Display.setTextColor(COLOR_SUCCESS);
            CoreS3.Display.drawString("BtnA = Accept", 160, 150);
            CoreS3.Display.setTextColor(COLOR_WARNING);
            CoreS3.Display.drawString("BtnB = Reject", 160, 180);
            
            bool acceptConnection = false;
            bool buttonPressed = false;
            unsigned long startTime = millis();
            const unsigned long TIMEOUT = 1000;
            
            while (millis() - startTime < TIMEOUT) {
                M5.update();
                
                if (M5.BtnA.wasPressed()) {
                    acceptConnection = true;
                    buttonPressed = true;
                    addLog("Accepted by user: " + espId, COLOR_SUCCESS);
                    Serial.println("Connection accepted by user");
                    break;
                } else if (M5.BtnB.wasPressed()) {
                    acceptConnection = false;
                    buttonPressed = true;
                    addLog("Rejected by user: " + espId, COLOR_WARNING);
                    Serial.println("Connection rejected by user");
                    break;
                }
                
                delay(50);
            }
            
            if (!buttonPressed) {
                Serial.println("Timeout - Connection refused for " + espId);
                addLog("Timeout: " + espId, COLOR_WARNING);
                request->send(408, "application/json", "{\"status\":\"error\",\"message\":\"Request timeout\"}");
                drawInterface();
                return;
            }
            
            if (!acceptConnection) {
                Serial.println("Connection refused for " + espId);
                addLog("Rejected: " + espId, COLOR_WARNING);
                request->send(403, "application/json", "{\"status\":\"error\",\"message\":\"Connection rejected by user\"}");
                drawInterface();
                return;
            }
            
            Serial.println("User accepted connection for " + espId);
            addLog("Accepted: " + espId, COLOR_SUCCESS);
            
            HTTPClient httpCheck;
            String checkURL = String(externalAPICheckURL) + "?id=" + espId;
            httpCheck.begin(checkURL);
            
            Serial.println("Checking if device exists: " + checkURL);
            
            int checkCode = httpCheck.GET();
            
            if (checkCode > 0) {
                String checkResponse = httpCheck.getString();
                Serial.println("Check response (" + String(checkCode) + "): " + checkResponse);
                
                JsonDocument checkDoc;
                deserializeJson(checkDoc, checkResponse);
                
                if (checkDoc["exists"] == true && checkDoc.containsKey("token")) {
                    String existingToken = checkDoc["token"].as<String>();
                    
                    Serial.println("Device already exists with token: " + existingToken);
                    addLog("Already linked: " + espId, COLOR_SUCCESS);
                    
                    String clientResponse = "{\"status\":\"success\",\"token\":\"" + existingToken + "\"}";
                    request->send(200, "application/json", clientResponse);
                    drawInterface();
                    httpCheck.end();
                    return;
                }
                
                Serial.println("Device does not exist, creating new token...");
                
                String newToken = "";
                for (int i = 0; i < 32; i++) {
                    newToken += String(random(0, 16), HEX);
                    if (i == 7 || i == 11 || i == 15 || i == 19) {
                        newToken += "-";
                    }
                }
                
                Serial.println("Generated token: " + newToken);
                addLog("New device: " + espId, COLOR_SUCCESS);
                
                HTTPClient httpAuth;
                httpAuth.begin(externalAPIAuthURL);
                httpAuth.addHeader("Content-Type", "application/json");
                
                JsonDocument authDoc;
                authDoc["id"] = espId;
                authDoc["token"] = newToken;
                
                String authPayload;
                serializeJson(authDoc, authPayload);
                
                Serial.println("Sending to API: " + authPayload);
                
                int authCode = httpAuth.POST(authPayload);
                
                if (authCode > 0) {
                    String authResponse = httpAuth.getString();
                    Serial.println("Auth API response (" + String(authCode) + "): " + authResponse);
                    
                    if (authCode == 200) {
                        addLog("API OK: " + espId, COLOR_SUCCESS);
                        
                        String clientResponse = "{\"status\":\"success\",\"token\":\"" + newToken + "\"}";
                        request->send(200, "application/json", clientResponse);
                    } else {
                        addLog("API failed: " + String(authCode), COLOR_WARNING);
                        request->send(authCode, "application/json", authResponse);
                    }
                    
                    drawInterface();
                } else {
                    Serial.println("Auth API Error: " + String(authCode));
                    addLog("API Error: " + String(authCode), COLOR_WARNING);
                    request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Auth API unreachable\"}");
                    drawInterface();
                }
                
                httpAuth.end();
                
            } else {
                Serial.println("Check API Error: " + String(checkCode));
                addLog("Check API Error", COLOR_WARNING);
                request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Check API unreachable\"}");
                drawInterface();
            }
            
            httpCheck.end();
        }
    );

    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "application/json", "{\"status\":\"running\"}");
    });

    server.begin();
    
    addLog("System started", COLOR_SUCCESS);
    addLog("Server running", COLOR_SUCCESS);
    
    delay(1000);
    drawInterface();
}

void loop() {
    unsigned long currentMillis = millis();
    
    if (currentMillis - lastUpdate >= UPDATE_INTERVAL) {
        lastUpdate = currentMillis;
        updateClientList();
        drawInterface();
    }
    
    delay(50);
}