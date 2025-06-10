import RPi.GPIO as GPIO
import time

# Définition des broches
CAPTEUR_PIN = 27  # GPIO17 (Pin 11)
LED_PIN = 17      # GPIO18 (Pin 12)

# Configuration des GPIO
GPIO.setmode(GPIO.BCM)
GPIO.setup(CAPTEUR_PIN, GPIO.IN)
GPIO.setup(LED_PIN, GPIO.OUT)

led_state = False
last_input = False

try:
    while True:
        input_state = GPIO.input(CAPTEUR_PIN)
        if input_state == GPIO.HIGH and last_input == False:
            led_state = not led_state  # Inverse l'état de la LED
            GPIO.output(LED_PIN, led_state)
            time.sleep(0.3)  # anti-rebond (évite les doubles pressions rapides)
        last_input = input_state
        time.sleep(0.05)

except KeyboardInterrupt:
    GPIO.cleanup()

