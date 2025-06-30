#include <Arduino.h>
// Capteur de son analogique avancé avec moyennage et détection de pics
// Adafruit HUZZAH32 ESP32 Feather + DFRobot DFR0034

#define SOUND_PIN A0
#define LED_PIN 13
#define SAMPLE_SIZE 50    // Nombre d'échantillons pour le moyennage
#define THRESHOLD_OFFSET 200  // Seuil au-dessus de la moyenne

int samples[SAMPLE_SIZE];
int sampleIndex = 0;
long sampleSum = 0;
int averageSound = 0;
int maxSound = 0;
int minSound = 4095;

void setup() {
  Serial.begin(921600);
  pinMode(LED_PIN, OUTPUT);
  analogReadResolution(12);
  
  // Initialisation du tableau d'échantillons
  for (int i = 0; i < SAMPLE_SIZE; i++) {
    samples[i] = 0;
  }
  
  Serial.println("=== Capteur de son ESP32 - Version Avancée ===");
  Serial.println("Calibration en cours...");
  
  // Calibration initiale (2 secondes)
  for (int i = 0; i < 100; i++) {
    int val = analogRead(SOUND_PIN);
    if (val > maxSound) maxSound = val;
    if (val < minSound) minSound = val;
    delay(20);
  }
  
  Serial.print("Calibration terminée - Min: ");
  Serial.print(minSound);
  Serial.print(" Max: ");
  Serial.println(maxSound);
}

void loop() {
  // Lecture et stockage dans le buffer circulaire
  int currentSound = analogRead(SOUND_PIN);
  
  // Mise à jour du buffer circulaire
  sampleSum -= samples[sampleIndex];  // Retirer l'ancien échantillon
  samples[sampleIndex] = currentSound;  // Ajouter le nouveau
  sampleSum += currentSound;
  sampleIndex = (sampleIndex + 1) % SAMPLE_SIZE;
  
  // Calculer la moyenne
  averageSound = sampleSum / SAMPLE_SIZE;
  
  // Mise à jour des extremes
  if (currentSound > maxSound) maxSound = currentSound;
  if (currentSound < minSound) minSound = currentSound;
  
  // Détection de pic sonore
  bool soundDetected = (currentSound > averageSound + THRESHOLD_OFFSET);
  
  // Contrôle de la LED
  digitalWrite(LED_PIN, soundDetected ? HIGH : LOW);
  
  // Affichage périodique (toutes les 50 lectures)
  static int displayCounter = 0;
  if (displayCounter++ >= 50) {
    displayCounter = 0;
    
    Serial.print("Actuel: ");
    Serial.print(currentSound);
    Serial.print(" | Moyenne: ");
    Serial.print(averageSound);
    Serial.print(" | Min: ");
    Serial.print(minSound);
    Serial.print(" | Max: ");
    Serial.print(maxSound);
    Serial.print(" | Seuil: ");
    Serial.print(averageSound + THRESHOLD_OFFSET);
    
    if (soundDetected) {
      Serial.print(" >>> PIC DÉTECTÉ!");
    }
    Serial.println();
  }
  
  delay(10);  // Échantillonnage rapide
}