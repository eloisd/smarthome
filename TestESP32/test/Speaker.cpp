/*
 * Test du Speaker DFRobot FIT0449 avec ESP32 HUZZAH32
 * 
 * Connexions:
 * Speaker VCC -> ESP32 3.3V
 * Speaker GND -> ESP32 GND  
 * Speaker Signal -> ESP32 GPIO 21 (ou tout autre pin digital)
 * 
 * Le potentiomètre sur le module speaker contrôle le volume
 */
#include <Arduino.h>

// Pin de connexion du speaker
const int SPEAKER_PIN = 21;  // Utilise GPIO 21 (pin D21)

// Définition des notes musicales (fréquences en Hz)
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523

// Mélodie simple (Do, Ré, Mi, Fa, Sol, La, Si, Do)
int melody[] = {
  NOTE_C4, NOTE_D4, NOTE_E4, NOTE_F4, 
  NOTE_G4, NOTE_A4, NOTE_B4, NOTE_C5
};

// Durées des notes (en millisecondes)
int noteDurations[] = {
  500, 500, 500, 500, 
  500, 500, 500, 1000
};


void testSimpleTones() {
  Serial.println("Test 1: Sons simples");
    
  // Série de bips courts
  for (int i = 0; i < 3; i++) {
    tone(SPEAKER_PIN, 1000, 100);  // 1kHz pendant 100ms
    delay(200);
  }
    
  delay(500);
    
  // Sons de différentes fréquences
  int frequencies[] = {200, 400, 800, 1200, 1600};
  for (int i = 0; i < 5; i++) {
    Serial.println("Fréquence: " + String(frequencies[i]) + " Hz");
    tone(SPEAKER_PIN, frequencies[i], 300);
    delay(500);
  }
}
  
void testMelody() {
  Serial.println("Test 2: Mélodie (gamme de Do)");
    
  for (int i = 0; i < 8; i++) {
    tone(SPEAKER_PIN, melody[i], noteDurations[i]);
    // Petite pause entre les notes
    delay(noteDurations[i] + 50);
  }
}
  
void testSiren() {
  Serial.println("Test 3: Effet sirène");
    
  // Sirène montante et descendante
  for (int j = 0; j < 3; j++) {
    // Montée
    for (int freq = 200; freq <= 800; freq += 10) {
      tone(SPEAKER_PIN, freq, 20);
      delay(20);
    }
      
    // Descente
    for (int freq = 800; freq >= 200; freq -= 10) {
      tone(SPEAKER_PIN, freq, 20);
      delay(20);
    }
  }
    
  noTone(SPEAKER_PIN);  // Arrêter le son
}
  
  // Fonction utilitaire pour jouer une note avec une durée
void playNote(int frequency, int duration) {
  tone(SPEAKER_PIN, frequency, duration);
  delay(duration + 50);  // Petite pause après chaque note
}
  
  // Fonction pour jouer la mélodie de Super Mario (extrait)
void playMarioTheme() {
  Serial.println("Bonus: Thème Super Mario (extrait)");
    
  int mario[] = {
    NOTE_E4, NOTE_E4, 0, NOTE_E4, 0, NOTE_C4, NOTE_E4, 0,
    NOTE_G4, 0, 0, 0, NOTE_G4/2, 0, 0, 0
  };
    
  int marioDurations[] = {
    150, 150, 150, 150, 150, 150, 150, 150,
    150, 150, 150, 150, 150, 150, 150, 150
  };
    
  for (int i = 0; i < 16; i++) {
    if (mario[i] == 0) {
      delay(marioDurations[i]);  // Pause
    } else {
      tone(SPEAKER_PIN, mario[i], marioDurations[i]);
      delay(marioDurations[i] + 25);
    }
  }
}

void setup() {
  Serial.begin(921600);
  Serial.println("=== Test Speaker DFRobot FIT0449 ===");
  Serial.println("Pin utilisé: GPIO " + String(SPEAKER_PIN));
  
  // Attendre un peu avant de commencer
  delay(2000);
  
  // Test 1: Sons simples
  testSimpleTones();
  
  delay(1000);
  
  // Test 2: Mélodie
  testMelody();
  
  delay(1000);
  
  // Test 3: Sirène
  testSiren();
  
  Serial.println("Tests terminés !");
}

void loop() {
  // Test continu: bip toutes les 3 secondes
  Serial.println("Bip!");
  tone(SPEAKER_PIN, NOTE_A4, 200);  // Bip de 200ms
  delay(3000);
}
