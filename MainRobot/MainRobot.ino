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

float volume = 0;
float voltage = 0;

String accessPoint = "ESP32_Config";
String accessPass = "password";

String ssids[MAX_NETWORKS];
String passwords[MAX_NETWORKS];

String admin_user = "admin";
String admin_password = "admin";

bool configMode = false;
bool isAuthenticated = false;

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
  for (int i = 0; i < MAX_NETWORKS; i++) {
    if (ssids[i] != "") {
      Serial.printf("Trying WiFi: %s\n", ssids[i].c_str());
      WiFi.begin(ssids[i].c_str(), passwords[i].c_str());
      if (WiFi.waitForConnectResult() == WL_CONNECTED) {
        Serial.println("Connected!");
        Serial.println(WiFi.localIP());
        return;
      }
    }
  }

  Serial.println("No known WiFi networks reachable");
}

void setupWebServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (isAuthenticated) {
      request->send(LittleFS, "/index.html", String(), false);
    } else {
      request->send(LittleFS, "/login.html", String(), false);
    }
  });

  server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/styles.css", "text/css");
  });

  server.on("/login.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/login.js", "application/javascript");
  });

  server.on("/login", HTTP_POST, [](AsyncWebServerRequest *request) {
    String username, password;
    if (request->hasParam("username", true)) username = request->getParam("username", true)->value();
    if (request->hasParam("password", true)) password = request->getParam("password", true)->value();

    if (username == "admin" && password == "admin") {
      isAuthenticated = true;
      request->redirect("/");
    } else {
      request->send(403, "text/plain", "Invalid credentials");
    }
  });

  server.begin();
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

  Serial.print("Percentage (%): ");
  Serial.println(percentage);

  return percentage;
}

float getVoltage() {
  int sensorValue = analogRead(VOLT_SENSOR);

  Serial.println("SensorValue: ");
  Serial.println(sensorValue);

  float voltage = sensorValue * (11.34 / 2768);
  Serial.print("Voltage: ");
  Serial.println(voltage);

  return (voltage);
}

String getSensorReadings() {
  readings["volume"] = String(volume);
  readings["battery"] = String(voltage);

  String jsonString = JSON.stringify(readings);
  return jsonString;
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  String msg = String((char*)data, len);

  if (msg == "forward") {
    motor1.pwm = motor2.pwm = 255;
    motor1.front();
    motor2.front();
  } else if (msg == "backward") {
    motor1.pwm = motor2.pwm = 200;
    motor1.back();
    motor2.back();
  } else if (msg == "left") {
    motor2.pwm = 200;
    motor1.stop();
    motor2.front();
  } else if (msg == "right") {
    motor1.pwm = 200;
    motor1.front();
    motor2.stop();
  } else if (msg == "stop") {
    motor1.stop();
    motor2.stop();
  } else if (msg == "spray") {
    Serial.println("spray");
    digitalWrite(RELAY_PIN, HIGH);
  } else if (msg == "stop-spray") {
    Serial.println("spray-stop");
    digitalWrite(RELAY_PIN, LOW);
  } else if (msg == "getReadings") {
    String sensorReadings = getSensorReadings();
    notifyClients(sensorReadings);
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
  // if (!LittleFS.begin()) {
  //   Serial.println("LittleFS mount failed!");
  //   return;
  // }

  motor1.begin(); motor1.enable();
  motor2.begin(); motor2.enable();

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(VOLT_SENSOR, INPUT);

  digitalWrite(TRIG_PIN, LOW);
  digitalWrite(RELAY_PIN, LOW);
  if (configMode) {
    Serial.println("======= Config Mode ========");
    WiFi.softAP(accessPoint, accessPass);
    Serial.println(WiFi.softAPIP());

    setupWebServer();
  } else {
    Serial.println("======= Running Mode ========");
    connectToWiFi();
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) delay(500);

    initWebSocket();
    server.begin();
  }

  if (!MDNS.begin("main-robot")) { // Connect to http://main-robot.local 
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
    
    Serial.println("mDNS responder started");
    Serial.println("Connect to web server by using http://main-robot.local");
}

bool doOnce = true;

void loop() {
   unsigned long now = millis();

  if (!configMode && (now - cleanupPrevMillis >= 1000)) {
    ws.cleanupClients();
    cleanupPrevMillis = millis();
  }

  if (!configMode && (now - readingsPrevMillis >= 5000)) {
    volume = getPesticideLevel();
    voltage = getVoltage();

    String sensorReadings = getSensorReadings();
    notifyClients(sensorReadings);
    
   readingsPrevMillis = millis();
  }
}