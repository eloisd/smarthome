/*
 * Test du capteur ultrasonique URM09 avec ESP32 Feather (HUZZAH32)
 * 
 * Connexions:
 * - VCC du URM09 -> 3.3V de l'ESP32
 * - GND du URM09 -> GND de l'ESP32  
 * - Signal du URM09 -> A2 (GPIO 34) de l'ESP32
 * 
 * Le URM09 fournit une sortie analogique proportionnelle à la distance
 * Plage de mesure: 2cm à 500cm
 * Fréquence max: 30Hz
 */
#include <Arduino.h>

// Pin de connexion du capteur
const int ULTRASONIC_PIN = A2;  // GPIO 34 sur ESP32 Feather

// Variables pour les calculs
float voltage = 0.0;
float distance_cm = 0.0;
int raw_value = 0;

// Paramètres de calibration pour le URM09
const float VOLTAGE_REF = 3.3;      // Tension de référence ESP32
const int ADC_RESOLUTION = 4095;    // Résolution ADC 12-bit
const float MIN_DISTANCE = 2.0;     // Distance minimale en cm
const float MAX_DISTANCE = 500.0;   // Distance maximale en cm

void setup() {
  Serial.begin(921600);
  
  // Attendre que le port série soit prêt
  while (!Serial) {
    delay(10);
  }
  
  Serial.println("=== Test du capteur ultrasonique URM09 ===");
  Serial.println("Capteur connecté sur la pin A2 (GPIO 34)");
  Serial.println("Plage de mesure: 2-500 cm");
  Serial.println("Fréquence max: 30Hz");
  Serial.println();
  
  // Configuration de la pin analogique
  pinMode(ULTRASONIC_PIN, INPUT);
  
  Serial.println("Valeur_brute\tTension(V)\tDistance(cm)\tStatut");
  Serial.println("--------------------------------------------------------");
  
  delay(1000);  // Laisser le capteur s'initialiser
}

void loop() {
  // Lecture de la valeur analogique
  raw_value = analogRead(ULTRASONIC_PIN);
  
  // Conversion en tension
  voltage = (raw_value * VOLTAGE_REF) / ADC_RESOLUTION;
  
  // Conversion en distance basée sur les spécifications du URM09
  // Le URM09 fournit typiquement une sortie linéaire:
  // - Tension minimale (~0.3V) = distance minimale (2cm)
  // - Tension maximale (~3.0V) = distance maximale (500cm)
  
  // Formule de conversion approximative (à ajuster selon calibration)
  if (voltage < 0.2) {
    distance_cm = 0;  // Pas d'objet détecté ou erreur
  } else {
    // Conversion linéaire basée sur la plage de tension
    distance_cm = ((voltage - 0.3) / (3.0 - 0.3)) * (MAX_DISTANCE - MIN_DISTANCE) + MIN_DISTANCE;
    
    // Limiter aux valeurs valides
    if (distance_cm < MIN_DISTANCE) distance_cm = MIN_DISTANCE;
    if (distance_cm > MAX_DISTANCE) distance_cm = MAX_DISTANCE;
  }
  
  // Affichage des résultats
  Serial.print(raw_value);
  Serial.print("\t\t");
  Serial.print(voltage, 3);
  Serial.print("\t\t");
  Serial.print(distance_cm, 1);
  Serial.print("\t\t");
  
  // Affichage du statut
  if (voltage < 0.2) {
    Serial.println("Aucun objet");
  } else if (distance_cm <= MIN_DISTANCE) {
    Serial.println("Trop proche");
  } else if (distance_cm >= MAX_DISTANCE) {
    Serial.println("Trop loin");
  } else {
    Serial.println("OK");
  }
  
  delay(100);  // Délai pour respecter la fréquence max de 30Hz
}

// Fonction de calibration (optionnelle)
void calibrateURM09() {
  Serial.println("\n=== Mode Calibration ===");
  Serial.println("Placez un objet à une distance connue et notez les valeurs");
  
  for (int i = 0; i < 50; i++) {
    raw_value = analogRead(ULTRASONIC_PIN);
    voltage = (raw_value * VOLTAGE_REF) / ADC_RESOLUTION;
    
    Serial.print("Mesure ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(raw_value);
    Serial.print(" (");
    Serial.print(voltage, 3);
    Serial.println("V)");
    
    delay(200);
  }
  
  Serial.println("Calibration terminée\n");
}

// Fonction pour test de plage
void testRange() {
  Serial.println("\n=== Test de Plage ===");
  Serial.println("Bougez un objet devant le capteur...");
  
  float min_dist = MAX_DISTANCE;
  float max_dist = MIN_DISTANCE;
  
  for (int i = 0; i < 100; i++) {
    raw_value = analogRead(ULTRASONIC_PIN);
    voltage = (raw_value * VOLTAGE_REF) / ADC_RESOLUTION;
    
    if (voltage > 0.2) {
      distance_cm = ((voltage - 0.3) / (3.0 - 0.3)) * (MAX_DISTANCE - MIN_DISTANCE) + MIN_DISTANCE;
      
      if (distance_cm < min_dist) min_dist = distance_cm;
      if (distance_cm > max_dist) max_dist = distance_cm;
      
      Serial.print("Distance: ");
      Serial.print(distance_cm, 1);
      Serial.println(" cm");
    }
    
    delay(100);
  }
  
  Serial.print("Distance minimale détectée: ");
  Serial.print(min_dist, 1);
  Serial.println(" cm");
  Serial.print("Distance maximale détectée: ");
  Serial.print(max_dist, 1);
  Serial.println(" cm");
  Serial.println();
}