#include "Herbicide_Robot.h"

#define MAX_NETWORKS 5

// Left Motor Pins
#define L_EN_LEFT       25
#define R_EN_LEFT       26
#define L_PWM_LEFT      23
#define R_PWM_LEFT      33

// Right Motor Pins
#define L_EN_RIGHT      2
#define R_EN_RIGHT      4
#define L_PWM_RIGHT     16
#define R_PWM_RIGHT     17

// Pin for Relay
#define RELAY_PIN       13

// Pins for Ultrasonic sensor being used as Pesticide Level detector
#define TRIG_PIN        18
#define ECHO_PIN        19

// Pin for the voltage sensor
#define VOLT_SENSOR     32

// Pin for the UART Communication between the ESP32 and the ESP32-CAM
#define CAM_TX          12
#define CAM_RX          14

// Pin for the Relay
#define RELAY_PIN       13

// Constants for Pesticide Level Detection
#define SOUND_SPEED           0.034
#define MIN_PESTICIDE_HGT     13.36    
#define MAX_PESTICIDE_HGT     2.06

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
Preferences pref;

JSONVar readings;
JSONVar payload;

float volume = 0;
float voltage = 0;
int speed;

String ssids[MAX_NETWORKS] = {"", "", "", "", ""};
String passwords[MAX_NETWORKS] = {"", "", "", "", ""};


unsigned long cleanupPrevMillis = 0;
unsigned long readingsPrevMillis = 0;

BTS7960 motor1(L_EN_LEFT, R_EN_LEFT, L_PWM_LEFT, R_PWM_LEFT);
BTS7960 motor2(L_EN_RIGHT, R_EN_RIGHT, L_PWM_RIGHT, R_PWM_RIGHT);


void loadCredentials() {
  pref.begin("wifi", true);
  for (int i = 0; i < MAX_NETWORKS; i++) {
    ssids[i] = pref.getString(("ssid" + String(i)).c_str(), "");
    passwords[i] = pref.getString(("pass" + String(i)).c_str(), "");
  }
  pref.end();
}

void saveCredentials(String ssid, String password) {
  pref.begin("wifi", false);
  for (int i = 0; i < MAX_NETWORKS; i++) {
    if (ssids[i] == "") {
      ssids[i] = ssid;
      passwords[i] = password;
      pref.putString(("ssid" + String(i)).c_str(), ssid);
      pref.putString(("pass" + String(i)).c_str(), password);
      break;
    }
  }
  pref.end();
}

void connectToWiFi() {
  WiFi.disconnect(true);   // full reset
  delay(500);

  for (int i = 0; i < MAX_NETWORKS; i++) {
    if (ssids[i].length() == 0) continue;

    Serial.printf("Trying WiFi: %s\n", ssids[i].c_str());

    WiFi.begin(ssids[i].c_str(), passwords[i].c_str());

    unsigned long startAttemptTime = millis();

    while (WiFi.status() != WL_CONNECTED &&
           millis() - startAttemptTime < 10000) {
      delay(300);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnected!");
      Serial.println(WiFi.localIP());
      return;
    }

    Serial.println("\nFailed, disconnecting...");
    WiFi.disconnect(true);
    delay(1000);
  }

  Serial.println("No known WiFi networks reachable");
}

void onAddCredential(AsyncWebServerRequest *request, uint8_t * data, size_t len, size_t index, size_t total) {
  if (index + len != total) return;

  StaticJsonDocument<512> received;
  DeserializationError err = deserializeJson(received, data, total);

  if (err) {
    request->send(400, "text/plain", "Invalid JSON");
    return;
  }

  String newSsid = received["ssid"];
  String newPassword = received["pass"];

  Serial.print("New SSID: ");
  Serial.println(newSsid);

  Serial.print("New Password: ");
  Serial.println(newPassword);

  saveCredentials(newSsid, newPassword);

  StaticJsonDocument<256> send;
    JsonArray SSIDs = send.createNestedArray("ssids");

    for (int i = 0; i < MAX_NETWORKS; i++) {
      if (ssids[i].length() > 0) {
       SSIDs.add(ssids[i]);
      }
    }

    send["max-num"] = MAX_NETWORKS;

    String jsonString;
    serializeJson(send, jsonString);

  request->send(200, "application/json", jsonString);
}

void onDeleteCredential(AsyncWebServerRequest *request, uint8_t * data, size_t len, size_t index, size_t total) {
  if (index + len != total) return;

  StaticJsonDocument<512> received;
  DeserializationError err = deserializeJson(received, data, total);

  if (err) {
    request->send(400, "text/plain", "Invalid JSON");
    return;
  }

  String deleteSsid = received["ssid"];
  pref.begin("wifi", false);
  for (int i = 0; i < MAX_NETWORKS; i++) {
    if (ssids[i] == deleteSsid) {
      Serial.println(deleteSsid);
      pref.remove(("ssid" + String(i)).c_str());
      pref.remove(("pass" + String(i)).c_str());
    }
  }
  pref.end();

  loadCredentials();
  StaticJsonDocument<256> send;
    JsonArray SSIDs = send.createNestedArray("ssids");

    for (int i = 0; i < MAX_NETWORKS; i++) {
      if (ssids[i].length() > 0) {
       SSIDs.add(ssids[i]);
      }
    }
 
    send["max-num"] = MAX_NETWORKS;

    String jsonString;
    serializeJson(send, jsonString);

  request->send(200, "application/json", jsonString);
}

void onEditCredential(AsyncWebServerRequest *request, uint8_t * data, size_t len, size_t index, size_t total) {
  if (index + len != total) return;

  StaticJsonDocument<512> received;
  DeserializationError err = deserializeJson(received, data, total);

  if (err) {
    request->send(400, "text/plain", "Invalid JSON");
    return;
  }

  String editSsid = received["ssid"];
  String newPassword = received["pass"];
  pref.begin("wifi", false);
  for (int i = 0; i < MAX_NETWORKS; i++) {
    if (ssids[i] == editSsid) {
      Serial.println(editSsid);
      pref.putString(("pass" + String(i)).c_str(), newPassword);
    }
  }
  pref.end();

  loadCredentials();
  StaticJsonDocument<256> send;
    JsonArray SSIDs = send.createNestedArray("ssids");

    for (int i = 0; i < MAX_NETWORKS; i++) {
      if (ssids[i].length() > 0) {
       SSIDs.add(ssids[i]);
      }
    }
 
    send["max-num"] = MAX_NETWORKS;

    String jsonString;
    serializeJson(send, jsonString);

  request->send(200, "application/json", jsonString);
}

void setupWebServer() {
  server.on("/list-credentials", HTTP_GET, [](AsyncWebServerRequest *request) {
    StaticJsonDocument<256> doc;
    JsonArray SSIDs = doc.createNestedArray("ssids");

    for (int i = 0; i < MAX_NETWORKS; i++) {
      if (ssids[i].length() > 0) {
       SSIDs.add(ssids[i]);
      }
    }

    doc["max-num"] = MAX_NETWORKS;

    String jsonString;
    serializeJson(doc, jsonString);
    request->send(200, "application/json", jsonString); 
  });

  server.on("/add-credential", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, onAddCredential);
  server.on("/delete-credential", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, onDeleteCredential);
  server.on("/edit-credential", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, onEditCredential);

  // server.on("/login", HTTP_POST, [](AsyncWebServerRequest *request) {
  //   String username, password;
  //   if (request->hasParam("username", true)) username = request->getParam("username", true)->value();
  //   if (request->hasParam("password", true)) password = request->getParam("password", true)->value();

  //   if (username == "admin" && password == "admin") {
  //     isAuthenticated = true;
  //     request->redirect("/");
  //   } else {
  //     request->send(403, "text/plain", "Invalid credentials");
  //   }
  // });

  // server.begin();
}

void notifyClients(String message) {
  ws.textAll(message);
}

float getPesticideLevel() {
  long duration;
  float distanceCm;
  float percentage;

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  // Send out the sound signal to be used to get the height if the fluid
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duration = pulseIn(ECHO_PIN, HIGH);

  distanceCm = duration * SOUND_SPEED / 2;

  // Serial.print("Distance (cm): ");
  // Serial.println(distanceCm);

  // Generate percentage of fluid present in the container
  percentage = map(distanceCm, MIN_PESTICIDE_HGT, MAX_PESTICIDE_HGT, 0, 100);

  return percentage;
}

float getVoltage() {
  int sensorValue = analogRead(VOLT_SENSOR);

  float voltage = sensorValue * (11.34 / 2768);

  return (voltage);
}

String getSensorReadings() {
  readings["volume"] = String(volume);
  readings["battery"] = String(voltage);
  readings["speed"] = String(speed);

  String jsonString = JSON.stringify(readings);
  return jsonString;
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  String msg = String((char*)data, len);

  if (msg == "forward") {
    motor1.pwm = motor2.pwm = speed;
    motor1.front();
    motor2.front();
  } else if (msg == "backward") {
    motor1.pwm = motor2.pwm = speed;
    motor1.back();
    motor2.back();
  } else if (msg == "left") {
    motor2.pwm = speed;
    motor1.stop();
    motor2.front();
  } else if (msg == "right") {
    motor1.pwm = speed;
    motor1.front();
    motor2.stop();
  } else if (msg == "stop") {
    motor1.stop();
    motor2.stop();
  } else if (msg == "spray") {
    Serial.println("spray");
    digitalWrite(RELAY_PIN, LOW);
  } else if (msg == "stop-spray") {
    Serial.println("spray-stop");
    digitalWrite(RELAY_PIN, HIGH);
  } else if (msg == "getReadings") {
    String sensorReadings = getSensorReadings();
    notifyClients(sensorReadings);
  } else if (msg.indexOf("speedVal") != -1) {
    Serial.println(msg);
    int val = msg.substring(9).toInt();
    Serial.println(val);
    speed = map(val, 0, 100, 150, 255);
  } else if (msg.startsWith("{")) {
    StaticJsonDocument<128> doc;
    DeserializationError err = deserializeJson(doc, msg);

    if (!err && doc.containsKey("type")) {
      const char* type = doc["type"];

      if (strcmp(type, "ping") == 0) {
        StaticJsonDocument<128> resp;
        resp["type"] = "pong";
        resp["t"] = doc["t"];   // echo timestamp back

        String json;
        serializeJson(resp, json);

        ws.textAll(json);   // respond immediately
        return;
      }
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
      break;

    case WS_EVT_ERROR:
      Serial.println("ws error");
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void setup() {
  Serial.begin(9600);
  while (!Serial) {;};

  motor1.begin(); motor1.enable();
  motor2.begin(); motor2.enable();

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(VOLT_SENSOR, INPUT);

  digitalWrite(TRIG_PIN, LOW);
  digitalWrite(RELAY_PIN, HIGH);

    setupWebServer();

    loadCredentials();

    WiFi.mode(WIFI_AP_STA);    
    connectToWiFi();

    WiFi.softAP(accessPoint, accessPass);
    Serial.print("AP IP Adress: ");
    Serial.println(WiFi.localIP());

    Serial.println(WiFi.localIP());
    setupWebServer();

    initWebSocket();

  if (!MDNS.begin("main-robot")) { // Connect to http://main-robot.local 
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
    
    Serial.println("mDNS responder started");
    Serial.println("Connect to web server by using http://main-robot.local");

    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");

    server.onNotFound([](AsyncWebServerRequest *request) {
  if (request->method() == HTTP_OPTIONS) {
    request->send(200);
  } else {
    request->send(404);
  }
});
    server.begin();
}

void loop() {
   unsigned long now = millis();

  if (now - cleanupPrevMillis >= 1000) {
    ws.cleanupClients();
    cleanupPrevMillis = millis();
  }

  if (now - readingsPrevMillis >= 500) {
    volume = getPesticideLevel();
    voltage = getVoltage();

    String sensorReadings = getSensorReadings();
    notifyClients(sensorReadings);
    
   readingsPrevMillis = millis();
  }
}