#!/usr/bin/python

#--------------------
# Kamera Servo steuerung
# Autor: Jan-Tarek Butt
# Datum: 12.03.2014
#--------------------
import RPi.GPIO as GPIO
import time
import argparse

value = argparse.ArgumentParser(description='Servo Steuerung')
value.add_argument('integers', metavar='N', type=int,help='Wert fuer Servo postion')
args = value.parse_args()

# Pin 26 als Ausgang deklarieren
#GPIO.setwarnings(False)
GPIO.setmode(GPIO.BOARD)
GPIO.setup(26, GPIO.OUT)
# PWM mit 50Hz an Pin 26 starten
Servo = GPIO.PWM(26, 50)
Servo.start(0)
Servo.ChangeDutyCycle(args.integers)
time.sleep(0.5)
# PWM stoppen
Servo.stop()
GPIO.cleanup()
