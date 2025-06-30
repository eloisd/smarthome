#include <Arduino.h>

/*
 * Système de Sécurité ESP32 HUZZAH32 Feather
 * 
 * Connexions :
 * - Capteur tactile capacitif : Pin D14 (GPIO 14)
 * - LED intégrée : Pin 13 (automatique)
 * - Capteur de son : Pin A2 (GPIO 34)
 * - Capteur ultrasonique URM09 : Pin A3 (GPIO 39)
 * - Speaker : Pin D21 (GPIO 21)
 * 
 * Fonctionnement :
 * - Appui court : basculer l'éclairage (ON/OFF)
 * - Appui long (5s) : armer/désarmer le système de sécurité
 * - Si armé : 20s pour quitter, puis surveillance active
 * - Détection : 30s pour désarmer, sinon alarme
 */

// Configuration des pins
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

// Prototypes des fonctions
void handleTouch();
void updateSecuritySystem();
void checkSensors();
void playStatusBeep(int count);
void playAlarmTone();
void updateLED();
float readUltrasonicDistance();
int readSoundLevel();

void setup() {
  Serial.begin(921600);
  Serial.println("=== Système de Sécurité ESP32 ===");
  
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
}

void loop() {
  handleTouch();
  updateSecuritySystem();
  
  // Vérification des capteurs uniquement si le système est armé
  if (securityState == ARMED && millis() - lastSensorCheck >= SENSOR_INTERVAL) {
    checkSensors();
    lastSensorCheck = millis();
  }
  
  updateLED();
  
  delay(10);
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
        Serial.println("=== ARMEMENT DU SYSTÈME ===");
        Serial.println("20 secondes pour quitter les lieux...");
        playStatusBeep(3); // 3 bips pour armement
        
      } else if (securityState == ARMING || securityState == ARMED) {
        // Désarmer le système
        securityState = DISARMED;
        ledState = false; // Éteindre la LED
        Serial.println("=== SYSTÈME DÉSARMÉ ===");
        playStatusBeep(1); // 1 bip pour désarmement
        
      } else if (securityState == INTRUSION) {
        // Désarmer pendant la période de grâce
        securityState = DISARMED;
        Serial.println("=== INTRUSION ANNULÉE - SYSTÈME DÉSARMÉ ===");
        playStatusBeep(2); // 2 bips pour annulation
        
      } else if (securityState == ALARM) {
        // Arrêter l'alarme
        securityState = DISARMED;
        alarmActive = false;
        Serial.println("=== ALARME ARRÊTÉE - SYSTÈME DÉSARMÉ ===");
        noTone(SPEAKER_PIN);
        playStatusBeep(1);
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
      } else {
        // Bips de countdown toutes les 5 secondes
        unsigned long elapsed = currentTime - stateChangeTime;
        if (elapsed % 5000 < 100) { // Bip au début de chaque période de 5s
          tone(SPEAKER_PIN, 600, 50);
        }
      }
      break;
      
    case INTRUSION:
      // Période de grâce (30 secondes pour désarmer)
      if (currentTime - stateChangeTime >= DISARM_DELAY) {
        securityState = ALARM;
        alarmActive = true;
        Serial.println("=== ALARME ACTIVÉE ===");
      } else {
        // Bips d'avertissement rapides
        if (currentTime % 500 < 50) {
          tone(SPEAKER_PIN, 1200, 30);
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
  }
}

int readSoundLevel() {
  int rawValue = analogRead(SOUND_PIN);
  // Le capteur retourne une valeur entre 0 et 4095
  // On peut ajuster le seuil selon l'environnement
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