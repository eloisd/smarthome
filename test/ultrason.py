import RPi.GPIO as GPIO
import time

TRIG = 23
ECHO = 24

GPIO.setmode(GPIO.BCM)
GPIO.setup(TRIG, GPIO.OUT)
GPIO.setup(ECHO, GPIO.IN)

def get_distance():
    # Assure un signal bas au début
    GPIO.output(TRIG, False)
    time.sleep(0.05)

    # Envoie une impulsion de 10µs
    GPIO.output(TRIG, True)
    time.sleep(0.00001)  # 10 µs
    GPIO.output(TRIG, False)

    # Attente du front montant
    while GPIO.input(ECHO) == 0:
        pulse_start = time.time()

    # Attente du front descendant
    while GPIO.input(ECHO) == 1:
        pulse_end = time.time()

    pulse_duration = pulse_end - pulse_start
    distance = pulse_duration * 17150  # cm
    distance = round(distance, 2)
    return distance

try:
    while True:
        dist = get_distance()
        print(f"Distance : {dist} cm")
        time.sleep(1)
except KeyboardInterrupt:
    GPIO.cleanup()
