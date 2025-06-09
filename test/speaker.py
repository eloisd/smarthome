#!/usr/bin/env python3
"""
Test simple d'un haut-parleur connecté aux GPIO d'une Raspberry Pi 4B
Connexions recommandées :
- GPIO 18 (pin 12) -> Haut-parleur (+)
- GND (pin 6 ou 14) -> Haut-parleur (-)
"""

import RPi.GPIO as GPIO
import time
import math

# Configuration
SPEAKER_PIN = 18
SAMPLE_RATE = 8000  # Hz
DURATION = 2        # secondes

def setup_gpio():
    """Configure les GPIO"""
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(SPEAKER_PIN, GPIO.OUT)
    return GPIO.PWM(SPEAKER_PIN, SAMPLE_RATE)

def play_tone(pwm, frequency, duration):
    """Joue une tonalité à la fréquence donnée"""
    if frequency > 0:
        pwm.ChangeFrequency(frequency)
        pwm.start(50)  # Duty cycle de 50%
        time.sleep(duration)
        pwm.stop()
    else:
        time.sleep(duration)

def test_speaker():
    """Test principal du haut-parleur"""
    print("Démarrage du test du haut-parleur...")
    print(f"Broche GPIO utilisée : {SPEAKER_PIN}")
    
    try:
        # Configuration PWM
        pwm = setup_gpio()
        
        # Test 1: Son continu
        print("\n1. Test son continu (440 Hz - La)")
        play_tone(pwm, 440, 1.0)
        time.sleep(0.5)
        
        # Test 2: Gamme ascendante
        print("2. Test gamme ascendante")
        frequencies = [261, 294, 329, 349, 392, 440, 493, 523]  # Do à Do
        for freq in frequencies:
            print(f"   Fréquence: {freq} Hz")
            play_tone(pwm, freq, 0.5)
            time.sleep(0.1)
        
        # Test 3: Sirène
        print("3. Test effet sirène")
        for i in range(50):
            freq = 200 + (200 * math.sin(i * 0.2))
            pwm.ChangeFrequency(int(freq))
            pwm.start(30)
            time.sleep(0.05)
        pwm.stop()
        
        # Test 4: Bips courts
        print("4. Test bips courts")
        for i in range(5):
            play_tone(pwm, 800, 0.2)
            time.sleep(0.3)
        
        print("\nTest terminé avec succès!")
        
    except KeyboardInterrupt:
        print("\nArrêt du test par l'utilisateur")
    except Exception as e:
        print(f"Erreur : {e}")
    finally:
        GPIO.cleanup()
        print("GPIO nettoyés")

def simple_beep():
    """Test basique - juste un bip"""
    print("Test basique - bip simple")
    try:
        GPIO.setmode(GPIO.BCM)
        GPIO.setup(SPEAKER_PIN, GPIO.OUT)
        
        pwm = GPIO.PWM(SPEAKER_PIN, 1000)  # 1000 Hz
        pwm.start(50)
        time.sleep(0.5)
        pwm.stop()
        
    finally:
        GPIO.cleanup()

if __name__ == "__main__":
    print("=== Test Haut-parleur Raspberry Pi ===")
    print("Appuyez sur Ctrl+C pour arrêter")
    
    choice = input("\nChoisir le test (1=complet, 2=simple): ")
    
    if choice == "2":
        simple_beep()
    else:
        test_speaker()
