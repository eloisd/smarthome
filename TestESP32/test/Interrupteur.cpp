#include <Arduino.h>

/*
 * Contrôle d'une LED avec capteur tactile capacitif
 * ESP32 HUZZAH32 Feather
 * 
 * Connexions :
 * - Capteur tactile : Pin D14 (GPIO 14)
 * - LED intégrée : Pin 13 (automatique)
 * 
 * Fonctionnement :
 * - Appui court : basculer la LED (ON/OFF)
 * - Appui long (5s) : activer/désactiver le mode clignotement
 * - Mode clignotement : la LED clignote même quand elle est "éteinte"
 */

// Configuration des pins
const int TOUCH_PIN = 14;        // Pin pour le capteur tactile capacitif
const int LED_BUILTIN_PIN = 13;  // LED intégrée sur l'ESP32 HUZZAH32

// Variables pour la gestion du capteur
bool touchState = false;
bool lastTouchState = false;
bool ledState = false;

// Variables pour l'anti-rebond
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

// Variables pour l'appui long
unsigned long touchStartTime = 0;
bool longPressDetected = false;
const unsigned long LONG_PRESS_TIME = 5000; // 5 secondes

// Variables pour le clignotement
bool blinkMode = false;
unsigned long lastBlinkTime = 0;
unsigned long blinkInterval = 500; // 500ms entre chaque clignotement
bool blinkLedState = false;

// Prototypes des fonctions
void handleBlinking();
void updateLED();

void setup() {
  // Initialiser la communication série
  Serial.begin(115200);
  Serial.println("=== Contrôle LED avec capteur tactile capacitif ===");
  Serial.println("Appui court: ON/OFF | Appui long (5s): Mode clignotement");
  
  // Configuration des pins
  pinMode(TOUCH_PIN, INPUT);
  pinMode(LED_BUILTIN_PIN, OUTPUT);
  
  // Éteindre la LED au démarrage
  digitalWrite(LED_BUILTIN_PIN, LOW);
  
  Serial.println("Système prêt !");
}

void loop() {
  // Lire l'état du capteur tactile
  bool currentTouchState = digitalRead(TOUCH_PIN);
  
  // Gestion des transitions du capteur tactile
  if (currentTouchState != lastTouchState) {
    lastDebounceTime = millis();
  }
  
  // Si l'état est stable depuis assez longtemps
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // Si l'état du capteur a vraiment changé
    if (currentTouchState != touchState) {
      touchState = currentTouchState;
      
      if (touchState == HIGH) {
        // Début du contact - démarrer le compteur pour appui long
        touchStartTime = millis();
        longPressDetected = false;
        Serial.println("Contact détecté - maintenir 5s pour mode clignotement");
        
      } else {
        // Fin du contact - vérifier si c'était un appui court
        unsigned long touchDuration = millis() - touchStartTime;
        
        if (!longPressDetected && touchDuration < LONG_PRESS_TIME) {
          // Appui court - basculer l'état de la LED
          ledState = !ledState;
          Serial.print("Appui court - LED : ");
          Serial.println(ledState ? "ALLUMÉE" : "ÉTEINTE");
        }
      }
    }
  }
  
  // Vérifier l'appui long pendant que le capteur est touché
  if (touchState == HIGH && !longPressDetected) {
    unsigned long touchDuration = millis() - touchStartTime;
    
    if (touchDuration >= LONG_PRESS_TIME) {
      // Appui long détecté - basculer le mode clignotement
      longPressDetected = true;
      blinkMode = !blinkMode;
      
      Serial.print("Appui long détecté - Mode clignotement : ");
      Serial.println(blinkMode ? "ACTIVÉ" : "DÉSACTIVÉ");
      
      // Feedback visuel : faire clignoter rapidement 3 fois
      for (int i = 0; i < 3; i++) {
        digitalWrite(LED_BUILTIN_PIN, HIGH);
        delay(100);
        digitalWrite(LED_BUILTIN_PIN, LOW);
        delay(100);
      }
    }
  }
  
  // Gestion du clignotement
  handleBlinking();
  
  // Contrôler la LED finale
  updateLED();
  
  // Sauvegarder l'état précédent
  lastTouchState = currentTouchState;
  
  // Petite pause pour éviter la surcharge du processeur
  delay(10);
}

void handleBlinking() {
  if (blinkMode && (millis() - lastBlinkTime) >= blinkInterval) {
    blinkLedState = !blinkLedState;
    lastBlinkTime = millis();
  }
}

void updateLED() {
  bool finalLedState;
  
  if (blinkMode) {
    // En mode clignotement : LED clignote selon blinkLedState
    // Si ledState est OFF, la LED clignote entre OFF et une luminosité réduite
    // Si ledState est ON, la LED clignote normalement
    finalLedState = blinkLedState;
  } else {
    // Mode normal : LED suit ledState
    finalLedState = ledState;
  }
  
  digitalWrite(LED_BUILTIN_PIN, finalLedState);
}

/*
 * Fonction alternative : LED suit directement le capteur
 * (cette fonction n'est plus utilisée avec le nouveau système)
 */
/*
void loop() {
  // Lire l'état du capteur tactile
  bool touchDetected = digitalRead(TOUCH_PIN);
  
  // Contrôler directement la LED
  digitalWrite(LED_BUILTIN_PIN, touchDetected);
  
  // Afficher l'état (limité pour éviter le spam)
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 500) {
    Serial.print("Capteur: ");
    Serial.print(touchDetected ? "TOUCHÉ" : "LIBRE");
    Serial.print(" | LED: ");
    Serial.println(touchDetected ? "ALLUMÉE" : "ÉTEINTE");
    lastPrint = millis();
  }
  
  delay(50);
}
*/