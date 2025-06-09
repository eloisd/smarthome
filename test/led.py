import RPi.GPIO as GPIO
import time

GPIO.setmode(GPIO.BCM)
GPIO.setup(17, GPIO.OUT)  # GPIO 17

GPIO.output(17, GPIO.HIGH)  # Allumer LED
time.sleep(10)
GPIO.output(17, GPIO.LOW)   # Éteindre LED

GPIO.cleanup()
