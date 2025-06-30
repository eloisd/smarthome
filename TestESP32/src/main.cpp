#include <Arduino.h>
#include <WiFiMulti.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

// Configuration WiFi et WebSocket
#define WIFI_SSID "El9i "
#define WIFI_PASSWORD "1923456780"
#define WS_HOST "aneblnoqd6.execute-api.us-east-1.amazonaws.com"
#define WS_PORT 443
#define WS_URL "/dev"

#define JSON_DOC_SIZE 2048
#define MSG_SIZE 512

WiFiMulti wifiMulti;
WebSocketsClient wsClient;

// Configuration des pins du système de sécurité
const int TOUCH_PIN = 14;           // Capteur tactile capacitif
const int LED_PIN = 13;             // LED intégrée
const int SOUND_PIN = A2;           // Capteur de son (GPIO 34)
const int ULTRASONIC_PIN = A3;      // Capteur ultrasonique (GPIO 39)
const int SPEAKER_PIN = 21;         // Speaker

// Variables pour la gestion du tactile
bool touchState = false;
bool lastTouchState = false;
bool ledState = false;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
unsigned long touchStartTime = 0;
bool longPressDetected = false;
const unsigned long LONG_PRESS_TIME = 5000; // 5 secondes

// Variables du système de sécurité
enum SecurityState {
  DISARMED,          // Désarmé
  ARMING,           // En cours d'armement (20s)
  ARMED,            // Armé et surveillant
  INTRUSION,        // Intrusion détectée (30s pour désarmer)
  ALARM             // Alarme active
};

SecurityState securityState = DISARMED;
unsigned long stateChangeTime = 0;
const unsigned long ARMING_DELAY = 20000;    // 20 secondes pour quitter
const unsigned long DISARM_DELAY = 30000;    // 30 secondes pour désarmer
bool alarmActive = false;

// Variables pour les capteurs
const int SOUND_THRESHOLD = 200;     // Seuil de détection sonore
const float DISTANCE_THRESHOLD = 50.0; // Seuil de distance en cm
unsigned long lastSensorCheck = 0;
const unsigned long SENSOR_INTERVAL = 100; // Vérification toutes les 100ms

// Variables pour l'alarme
unsigned long lastAlarmToggle = 0;
bool alarmLedState = false;
const unsigned long ALARM_BLINK_INTERVAL = 250; // Clignotement rapide

// Variables pour les bips de statut
unsigned long lastStatusBeep = 0;
int statusBeepCount = 0;

// Variables pour la connexion WiFi/WebSocket
unsigned long lastConnectionCheck = 0;
const unsigned long CONNECTION_CHECK_INTERVAL = 30000; // 30 secondes
bool wasConnected = false;

// Prototypes des fonctions
void handleTouch();
void updateSecuritySystem();
void checkSensors();
void playStatusBeep(int count);
void playAlarmTone();
void updateLED();
float readUltrasonicDistance();
int readSoundLevel();
void sendSecurityStatus();
void sendErrorMessage(const char* error);
void sendOkMessage();
void handleWebSocketMessage(uint8_t* payload);
void onWSEvent(WStype_t type, uint8_t* payload, size_t length);
uint8_t toMode(const char* val);
const char* getSecurityStateString();
void connectToWiFi();
void checkConnection();

void setup() {
  Serial.begin(921600);
  Serial.println("=== Système de Sécurité ESP32 avec Contrôle Internet ===");
  
  // Configuration des pins
  pinMode(TOUCH_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(SOUND_PIN, INPUT);
  pinMode(ULTRASONIC_PIN, INPUT);
  
  // Initialisation
  digitalWrite(LED_PIN, LOW);
  securityState = DISARMED;
  
  Serial.println("Système initialisé - Mode éclairage");
  Serial.println("Appui court: ON/OFF lumière");
  Serial.println("Appui long (5s): Armer/Désarmer sécurité");
  
  // Bip de démarrage
  tone(SPEAKER_PIN, 1000, 200);
  delay(300);
  tone(SPEAKER_PIN, 1500, 200);
  
  // Connexion WiFi
  connectToWiFi();
}

void loop() {
  // Gestion des contrôles physiques
  handleTouch();
  
  // Mise à jour du système de sécurité AVANT la vérification des capteurs
  updateSecuritySystem();
  
  // Vérification des capteurs uniquement si le système est armé
  if (securityState == ARMED && millis() - lastSensorCheck >= SENSOR_INTERVAL) {
    checkSensors();
    lastSensorCheck = millis();
  }
  
  updateLED();
  
  // Gestion de la connexion WebSocket
  if (WiFi.status() == WL_CONNECTED) {
    wsClient.loop();
  }
  
  // Vérification périodique de la connexion
  checkConnection();
  
  delay(10);
}

void connectToWiFi() {
  Serial.println("Connexion au WiFi...");
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
  
  while (wifiMulti.run() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi connecté!");
  Serial.print("Adresse IP: ");
  Serial.println(WiFi.localIP());
  
  // Connexion WebSocket
  wsClient.beginSSL(WS_HOST, WS_PORT, WS_URL, "", "wss");
  wsClient.onEvent(onWSEvent);
  
  wasConnected = true;
  
  // Bip de connexion
  tone(SPEAKER_PIN, 2000, 100);
  delay(150);
  tone(SPEAKER_PIN, 2500, 100);
}

void checkConnection() {
  if (millis() - lastConnectionCheck >= CONNECTION_CHECK_INTERVAL) {
    lastConnectionCheck = millis();
    
    if (WiFi.status() != WL_CONNECTED) {
      if (wasConnected) {
        Serial.println("Connexion WiFi perdue, tentative de reconnexion...");
        wasConnected = false;
        // Bip de déconnexion
        tone(SPEAKER_PIN, 500, 200);
      }
      connectToWiFi();
    } else if (!wasConnected) {
      wasConnected = true;
      Serial.println("Connexion WiFi rétablie");
    }
  }
}

void onWSEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch(type) {
    case WStype_CONNECTED:
      Serial.println("WebSocket connecté");
      // Attendre un peu pour que la connexion soit stable
      delay(500);
      sendSecurityStatus(); // Envoyer le statut actuel
      Serial.println("Statut de sécurité envoyé après connexion");
      break;
    case WStype_DISCONNECTED:
      Serial.println("WebSocket déconnecté");
      break;
    case WStype_TEXT:
      Serial.printf("Message WebSocket: %s\n", payload);
      handleWebSocketMessage(payload);
      break;
  }
}

void handleWebSocketMessage(uint8_t* payload) {
  StaticJsonDocument<JSON_DOC_SIZE> doc;
  
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print("Erreur JSON: ");
    Serial.println(error.f_str());
    sendErrorMessage(error.c_str());
    return;
  }
  
  if (!doc["type"].is<const char*>()) {
    sendErrorMessage("Type de message invalide");
    return;
  }
  
  if (strcmp(doc["type"], "cmd") == 0) {
    if (!doc["body"].is<JsonObject>()) {
      sendErrorMessage("Corps de commande invalide");
      return;
    }
    
    JsonObject body = doc["body"];
    const char* cmdType = body["type"];
    
    if (strcmp(cmdType, "security") == 0) {
      const char* action = body["action"];
      
      if (strcmp(action, "arm") == 0) {
        if (securityState == DISARMED) {
          securityState = ARMING;
          stateChangeTime = millis();
          Serial.println("=== ARMEMENT DU SYSTÈME (Commande Internet) ===");
          playStatusBeep(3);
          sendSecurityStatus();
        } else {
          sendErrorMessage("Le système ne peut pas être armé dans son état actuel");
        }
      } else if (strcmp(action, "disarm") == 0) {
        if (securityState != DISARMED) {
          securityState = DISARMED;
          alarmActive = false;
          ledState = false; // Éteindre la LED
          noTone(SPEAKER_PIN);
          Serial.println("=== SYSTÈME DÉSARMÉ (Commande Internet) ===");
          playStatusBeep(1);
          sendSecurityStatus();
        } else {
          sendErrorMessage("Le système est déjà désarmé");
        }
      } else if (strcmp(action, "status") == 0) {
        Serial.println("Demande de statut reçue - Envoi du statut actuel");
        sendSecurityStatus();
      } else {
        sendErrorMessage("Action de sécurité non reconnue");
      }
    } else if (strcmp(cmdType, "pinMode") == 0) {
      pinMode(body["pin"], toMode(body["mode"]));
      sendOkMessage();
    } else if (strcmp(cmdType, "digitalWrite") == 0) {
      digitalWrite(body["pin"], body["value"]);
      if (body["pin"] == LED_PIN) {
        ledState = body["value"];
      }
      sendOkMessage();
    } else if (strcmp(cmdType, "digitalRead") == 0) {
      auto value = digitalRead(body["pin"]);
      char msg[MSG_SIZE];
      sprintf(msg, "{\"action\": \"msg\", \"type\": \"output\", \"body\": %d}", value);
      wsClient.sendTXT(msg);
    } else {
      sendErrorMessage("Type de commande non supporté");
    }
  } else {
    sendErrorMessage("Type de message non supporté");
  }
}

void sendSecurityStatus() {
  char msg[MSG_SIZE];
  sprintf(msg, 
    "{\"action\": \"msg\", \"type\": \"security_status\", \"body\": {\"state\": \"%s\", \"led\": %s, \"alarm\": %s}}", 
    getSecurityStateString(), 
    ledState ? "true" : "false",
    alarmActive ? "true" : "false"
  );
  wsClient.sendTXT(msg);
}

const char* getSecurityStateString() {
  switch (securityState) {
    case DISARMED: return "disarmed";
    case ARMING: return "arming";
    case ARMED: return "armed";
    case INTRUSION: return "intrusion";
    case ALARM: return "alarm";
    default: return "unknown";
  }
}

void sendErrorMessage(const char* error) {
  char msg[MSG_SIZE];
  sprintf(msg, "{\"action\": \"msg\", \"type\": \"error\", \"body\": \"%s\"}", error);
  wsClient.sendTXT(msg);
}

void sendOkMessage() {
  wsClient.sendTXT("{\"action\": \"msg\", \"type\": \"status\", \"body\": \"ok\"}");
}

uint8_t toMode(const char* val) {
  if (strcmp(val, "output") == 0) {
    return OUTPUT;
  }
  if (strcmp(val, "input_pull") == 0) {
    return INPUT_PULLUP;
  }
  return INPUT;
}

void handleTouch() {
  bool currentTouchState = digitalRead(TOUCH_PIN);
  
  // Gestion de l'anti-rebond
  if (currentTouchState != lastTouchState) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (currentTouchState != touchState) {
      touchState = currentTouchState;
      
      if (touchState == HIGH) {
        // Début du contact
        touchStartTime = millis();
        longPressDetected = false;
        
      } else {
        // Fin du contact
        unsigned long touchDuration = millis() - touchStartTime;
        
        if (!longPressDetected && touchDuration < LONG_PRESS_TIME) {
          // Appui court - basculer l'éclairage
          if (securityState == DISARMED) {
            ledState = !ledState;
            Serial.print("Éclairage : ");
            Serial.println(ledState ? "ALLUMÉ" : "ÉTEINT");
            tone(SPEAKER_PIN, ledState ? 800 : 400, 100);
            sendSecurityStatus(); // Notifier le changement
          }
        }
      }
    }
  }
  
  // Vérifier l'appui long pendant que le capteur est touché
  if (touchState == HIGH && !longPressDetected) {
    unsigned long touchDuration = millis() - touchStartTime;
    
    if (touchDuration >= LONG_PRESS_TIME) {
      longPressDetected = true;
      
      // Gestion de l'armement/désarmement
      if (securityState == DISARMED) {
        // Armer le système
        securityState = ARMING;
        stateChangeTime = millis();
        Serial.println("=== ARMEMENT DU SYSTÈME (Contrôle Physique) ===");
        Serial.println("20 secondes pour quitter les lieux...");
        playStatusBeep(3); // 3 bips pour armement
        sendSecurityStatus(); // Notifier le changement
        
      } else if (securityState == ARMING || securityState == ARMED) {
        // Désarmer le système
        securityState = DISARMED;
        ledState = false; // Éteindre la LED
        Serial.println("=== SYSTÈME DÉSARMÉ (Contrôle Physique) ===");
        playStatusBeep(1); // 1 bip pour désarmement
        sendSecurityStatus(); // Notifier le changement
        
      } else if (securityState == INTRUSION) {
        // Désarmer pendant la période de grâce
        securityState = DISARMED;
        Serial.println("=== INTRUSION ANNULÉE - SYSTÈME DÉSARMÉ (Contrôle Physique) ===");
        playStatusBeep(2); // 2 bips pour annulation
        sendSecurityStatus(); // Notifier le changement
        
      } else if (securityState == ALARM) {
        // Arrêter l'alarme
        securityState = DISARMED;
        alarmActive = false;
        Serial.println("=== ALARME ARRÊTÉE - SYSTÈME DÉSARMÉ (Contrôle Physique) ===");
        noTone(SPEAKER_PIN);
        playStatusBeep(1);
        sendSecurityStatus(); // Notifier le changement
      }
    }
  }
  
  lastTouchState = currentTouchState;
}

void updateSecuritySystem() {
  unsigned long currentTime = millis();
  
  switch (securityState) {
    case ARMING:
      // Période d'armement (20 secondes)
      if (currentTime - stateChangeTime >= ARMING_DELAY) {
        securityState = ARMED;
        Serial.println("=== SYSTÈME ARMÉ - SURVEILLANCE ACTIVE ===");
        playStatusBeep(5); // 5 bips rapides pour confirmer l'armement
        sendSecurityStatus(); // Notifier le changement
      } else {
        // Bips de countdown toutes les 5 secondes
        unsigned long elapsed = currentTime - stateChangeTime;
        if (elapsed % 5000 < 100) { // Bip au début de chaque période de 5s
          tone(SPEAKER_PIN, 600, 50);
          Serial.print("Armement en cours... ");
          Serial.print((ARMING_DELAY - elapsed) / 1000);
          Serial.println("s restantes");
        }
      }
      break;
      
    case INTRUSION:
      // Période de grâce (30 secondes pour désarmer)
      if (currentTime - stateChangeTime >= DISARM_DELAY) {
        securityState = ALARM;
        alarmActive = true;
        Serial.println("=== ALARME ACTIVÉE ===");
        sendSecurityStatus(); // Notifier le changement
      } else {
        // Bips d'avertissement rapides
        if (currentTime % 500 < 50) {
          tone(SPEAKER_PIN, 1200, 30);
        }
        
        // Debug: afficher le temps restant
        unsigned long elapsed = currentTime - stateChangeTime;
        if (elapsed % 5000 < 100) {
          Serial.print("INTRUSION! Temps pour désarmer: ");
          Serial.print((DISARM_DELAY - elapsed) / 1000);
          Serial.println("s");
        }
      }
      break;
      
    case ALARM:
      // Alarme active
      playAlarmTone();
      break;
  }
}

void checkSensors() {
  bool intrusionDetected = false;
  
  // Debug: indiquer qu'on vérifie les capteurs
  static unsigned long lastDebugSensor = 0;
  if (millis() - lastDebugSensor >= 10000) { // Debug toutes les 10s
    Serial.println("Surveillance active - Vérification des capteurs...");
    lastDebugSensor = millis();
  }
  
  // Vérifier le capteur de son
  int soundLevel = readSoundLevel();
  if (soundLevel > SOUND_THRESHOLD) {
    Serial.print("Son détecté: ");
    Serial.println(soundLevel);
    intrusionDetected = true;
  }
  
  // Vérifier le capteur de distance
  float distance = readUltrasonicDistance();
  if (distance > 0 && distance < DISTANCE_THRESHOLD) {
    Serial.print("Mouvement détecté à ");
    Serial.print(distance);
    Serial.println(" cm");
    intrusionDetected = true;
  }
  
  // Si intrusion détectée, passer en mode INTRUSION
  if (intrusionDetected && securityState == ARMED) {
    securityState = INTRUSION;
    stateChangeTime = millis();
    Serial.println("=== INTRUSION DÉTECTÉE ===");
    Serial.println("30 secondes pour désarmer le système !");
    sendSecurityStatus(); // Notifier le changement
  }
}

int readSoundLevel() {
  int rawValue = analogRead(SOUND_PIN);
  return rawValue;
}

float readUltrasonicDistance() {
  int rawValue = analogRead(ULTRASONIC_PIN);
  float voltage = (rawValue * 3.3) / 4095.0;
  
  // Conversion basée sur les spécifications du URM09
  if (voltage < 0.2) {
    return 0; // Pas d'objet détecté
  }
  
  // Formule de conversion approximative pour URM09
  float distance = ((voltage - 0.3) / (3.0 - 0.3)) * (500.0 - 2.0) + 2.0;
  
  // Limiter aux valeurs valides
  if (distance < 2.0) distance = 2.0;
  if (distance > 500.0) distance = 500.0;
  
  return distance;
}

void playStatusBeep(int count) {
  for (int i = 0; i < count; i++) {
    tone(SPEAKER_PIN, 1000, 150);
    delay(200);
  }
}

void playAlarmTone() {
  static unsigned long lastToneChange = 0;
  static bool highTone = true;
  
  if (millis() - lastToneChange >= 500) {
    if (highTone) {
      tone(SPEAKER_PIN, 2000);
    } else {
      tone(SPEAKER_PIN, 800);
    }
    highTone = !highTone;
    lastToneChange = millis();
  }
}

void updateLED() {
  switch (securityState) {
    case DISARMED:
      // Mode éclairage normal
      digitalWrite(LED_PIN, ledState);
      break;
      
    case ARMING:
      // Clignotement lent pendant l'armement
      if (millis() % 1000 < 500) {
        digitalWrite(LED_PIN, HIGH);
      } else {
        digitalWrite(LED_PIN, LOW);
      }
      break;
      
    case ARMED:
      // LED allumée fixe quand armé
      digitalWrite(LED_PIN, HIGH);
      break;
      
    case INTRUSION:
      // Clignotement rapide pendant la période de grâce
      if (millis() % 250 < 125) {
        digitalWrite(LED_PIN, HIGH);
      } else {
        digitalWrite(LED_PIN, LOW);
      }
      break;
      
    case ALARM:
      // Clignotement très rapide pendant l'alarme
      if (millis() % 100 < 50) {
        digitalWrite(LED_PIN, HIGH);
      } else {
        digitalWrite(LED_PIN, LOW);
      }
      break;
  }
}