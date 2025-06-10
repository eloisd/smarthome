#!/usr/bin/env python3
"""
Lecteur audio pour module DFRobot Speaker (FIT0449)
Ce module a un amplificateur intégré et nécessite 3 connexions:
- VCC (alimentation 3.3V ou 5V)
- GND (masse)
- Signal (signal audio numérique)
"""

import RPi.GPIO as GPIO
import time
import pygame
import os
import subprocess
from pydub import AudioSegment

class DFRobotSpeakerPlayer:
    def __init__(self, signal_pin=18, vcc_pin=None):
        """
        signal_pin: GPIO pour le signal audio (ex: 18)
        vcc_pin: GPIO pour alimenter le module (optionnel si alimenté par 5V/3.3V fixe)
        """
        self.signal_pin = signal_pin
        self.vcc_pin = vcc_pin
        self.temp_wav_file = "/tmp/audio_converted.wav"
        
    def setup_gpio(self):
        """Configure les GPIO pour le module DFRobot"""
        GPIO.setmode(GPIO.BCM)
        GPIO.setup(self.signal_pin, GPIO.OUT)
        
        # Si on contrôle l'alimentation par GPIO
        if self.vcc_pin:
            GPIO.setup(self.vcc_pin, GPIO.OUT)
            GPIO.output(self.vcc_pin, GPIO.HIGH)  # Alimenter le module
            print(f"Module alimenté via GPIO {self.vcc_pin}")
        
        print(f"Signal audio sur GPIO {self.signal_pin}")
    
    def test_module_basic(self):
        """Test basique du module avec des tonalités"""
        print("=== Test du module DFRobot ===")
        self.setup_gpio()
        
        try:
            # Test avec PWM pour générer des tonalités
            pwm = GPIO.PWM(self.signal_pin, 1000)
            
            print("Test 1: Bip simple")
            pwm.start(50)  # 50% duty cycle
            time.sleep(0.5)
            pwm.stop()
            time.sleep(0.5)
            
            print("Test 2: Gamme de fréquences")
            frequencies = [440, 523, 659, 784, 880]  # La, Do, Mi, Sol, La
            
            for freq in frequencies:
                print(f"  Fréquence: {freq} Hz")
                pwm.ChangeFrequency(freq)
                pwm.start(50)
                time.sleep(0.8)
                pwm.stop()
                time.sleep(0.2)
            
            print("Test 3: Volume progressif")
            pwm.ChangeFrequency(1000)
            for volume in range(10, 101, 10):
                print(f"  Volume: {volume}%")
                pwm.start(volume)
                time.sleep(0.3)
            pwm.stop()
            
        except Exception as e:
            print(f"Erreur lors du test: {e}")
        finally:
            GPIO.cleanup()
    
    def convert_m4a_to_wav(self, input_file):
        """Convertit M4A en WAV optimisé pour le module"""
        print(f"Conversion de {input_file}...")
        
        try:
            # Conversion avec pydub (si disponible)
            audio = AudioSegment.from_file(input_file, format="m4a")
            
            # Optimisation pour module amplificateur:
            # - Mono pour simplifier
            # - 16kHz pour réduire la charge sur le GPIO
            # - Normalisation pour maximiser le volume
            audio = audio.set_channels(1)
            audio = audio.set_frame_rate(16000)
            audio = audio.set_sample_width(2)
            audio = audio.normalize()
            
            # Augmenter le gain si nécessaire
            audio = audio + 6  # +6dB
            
            audio.export(self.temp_wav_file, format="wav")
            print("✓ Conversion réussie!")
            return True
            
        except ImportError:
            print("pydub non disponible, utilisation de ffmpeg...")
            return self.convert_with_ffmpeg(input_file)
        except Exception as e:
            print(f"Erreur pydub: {e}")
            return self.convert_with_ffmpeg(input_file)
    
    def convert_with_ffmpeg(self, input_file):
        """Conversion avec ffmpeg"""
        try:
            cmd = [
                'ffmpeg', '-i', input_file,
                '-ac', '1',          # Mono
                '-ar', '16000',      # 16kHz
                '-vol', '512',       # Augmenter le volume
                '-y',                # Overwrite
                self.temp_wav_file
            ]
            result = subprocess.run(cmd, capture_output=True, text=True)
            
            if result.returncode == 0:
                print("✓ Conversion ffmpeg réussie!")
                return True
            else:
                print(f"Erreur ffmpeg: {result.stderr}")
                return False
                
        except Exception as e:
            print(f"Erreur ffmpeg: {e}")
            return False
    
    def play_with_pygame(self, wav_file):
        """Lecture avec pygame (méthode recommandée)"""
        try:
            # Initialiser pygame mixer avec paramètres optimisés
            pygame.mixer.pre_init(frequency=16000, size=-16, channels=1, buffer=512)
            pygame.mixer.init()
            
            print("Lecture avec pygame...")
            pygame.mixer.music.load(wav_file)
            pygame.mixer.music.play()
            
            # Attendre la fin de la lecture
            while pygame.mixer.music.get_busy():
                time.sleep(0.1)
            
            pygame.mixer.quit()
            print("✓ Lecture terminée!")
            return True
            
        except Exception as e:
            print(f"Erreur pygame: {e}")
            return False
    
    def play_with_aplay(self, wav_file):
        """Lecture avec aplay système"""
        try:
            print("Lecture avec aplay...")
            # Forcer la sortie sur les GPIO
            cmd = ['aplay', '-D', 'plughw:0,0', wav_file]
            subprocess.run(cmd, check=True)
            print("✓ Lecture terminée!")
            return True
            
        except Exception as e:
            print(f"Erreur aplay: {e}")
            return False
    
    def play_m4a_file(self, filename):
        """Lecture complète d'un fichier M4A"""
        if not os.path.exists(filename):
            print(f"Erreur: Fichier {filename} introuvable!")
            return False
        
        print(f"\n=== Lecture de {filename} sur module DFRobot ===")
        
        # Étape 1: Configuration GPIO
        self.setup_gpio()
        
        try:
            # Étape 2: Conversion
            if not self.convert_m4a_to_wav(filename):
                print("Échec de la conversion!")
                return False
            
            # Étape 3: Lecture (essayer plusieurs méthodes)
            success = False
            
            # Méthode 1: pygame
            print("\nTentative avec pygame...")
            success = self.play_with_pygame(self.temp_wav_file)
            
            # Méthode 2: aplay si pygame échoue
            if not success:
                print("\nTentative avec aplay...")
                success = self.play_with_aplay(self.temp_wav_file)
            
            if success:
                print("🔊 Audio joué avec succès!")
            else:
                print("❌ Échec de toutes les méthodes de lecture")
            
            return success
            
        finally:
            # Nettoyage
            if os.path.exists(self.temp_wav_file):
                os.remove(self.temp_wav_file)
            GPIO.cleanup()
    
    def check_connections(self):
        """Vérifier les connexions du module"""
        print("=== Vérification des connexions DFRobot Speaker ===")
        print("Connexions requises:")
        print("1. VCC (rouge) -> 5V ou 3.3V du Raspberry Pi")
        print("2. GND (noir) -> GND du Raspberry Pi") 
        print(f"3. Signal (jaune/blanc) -> GPIO {self.signal_pin} du Raspberry Pi")
        print("\nPotentiomètre sur le module:")
        print("- Tournez dans le sens horaire pour augmenter le volume")
        print("- Assurez-vous qu'il n'est pas au minimum")
        
        # Test de base
        input("\nAppuyez sur Entrée pour tester les connexions...")
        self.test_module_basic()

def main():
    # Configuration par défaut
    player = DFRobotSpeakerPlayer(signal_pin=18)
    
    while True:
        print("\n" + "="*50)
        print("LECTEUR AUDIO DFROBOT SPEAKER MODULE")
        print("="*50)
        print("1. Tester le module (tonalités)")
        print("2. Jouer Lumières.m4a")
        print("3. Jouer un autre fichier")
        print("4. Vérifier les connexions")
        print("5. Configuration du GPIO")
        print("0. Quitter")
        
        choice = input("\nChoisissez une option: ")
        
        if choice == "1":
            player.test_module_basic()
            
        elif choice == "2":
            # Chercher le fichier Lumières.m4a
            possible_files = [
                "Lumières.m4a",
                "Lumieres.m4a",
                "./Lumières.m4a",
                "/home/pi/Lumières.m4a"
            ]
            
            file_found = None
            for f in possible_files:
                if os.path.exists(f):
                    file_found = f
                    break
            
            if file_found:
                player.play_m4a_file(file_found)
            else:
                print("Fichier Lumières.m4a introuvable!")
                
        elif choice == "3":
            filename = input("Nom du fichier audio: ")
            player.play_m4a_file(filename)
            
        elif choice == "4":
            player.check_connections()
            
        elif choice == "5":
            pin = input(f"GPIO du signal (actuel: {player.signal_pin}): ")
            if pin.isdigit():
                player.signal_pin = int(pin)
                print(f"GPIO configuré sur {player.signal_pin}")
                
        elif choice == "0":
            break
            
        else:
            print("Option invalide")
        
        input("\nAppuyez sur Entrée pour continuer...")

if __name__ == "__main__":
    print("🔊 Lecteur audio pour module DFRobot Speaker")
    print("Assurez-vous que votre module est correctement connecté!")
    main()
