# ***************************************************************************************
# Creative Commons Attrib Share-Alike License
# You are free to use/extend this code/library but please abide with the CCSA license:
# http://creativecommons.org/licenses/by-sa/4.0/
# **********************************************************************************

import os
import glob
import time
import RPi.GPIO as GPIO
import mosquitto
import picamera
import shutil
import tweepy

# init US and Level sensor
GPIO.setmode(GPIO.BCM)

TRIG = 23
ECHO = 24
LEVEL = 11
BUTTON = 9
LED = 10

GPIO.setup(LED, GPIO.OUT)
GPIO.setup(TRIG, GPIO.OUT)
GPIO.setup(ECHO, GPIO.IN)
GPIO.setup(LEVEL, GPIO.IN)
GPIO.setup(BUTTON, GPIO.IN)

GPIO.output(TRIG, False)
print("initializing us and level sensors")
time.sleep(2)

# tweepy authorization
consumer_key = # removed for privacy
consumer_secret = # removed for privacy
access_token = # removed for privacy
access_token_secret = # removed for privacy

auth = tweepy.OAuthHandler(consumer_key, consumer_secret)
auth.set_access_token(access_token, access_token_secret)

api = tweepy.API(auth)

photo_path = '/home/pi/kookiePix/currPic/img.jpg'

# temp sensor 
os.system('modprobe w1-gpio')
os.system('modprobe w1-therm')
 
base_dir = '/sys/bus/w1/devices/'
device_folder = glob.glob(base_dir + '28*')[0]
device_file = device_folder + '/w1_slave'
 
def read_temp_raw():
    f = open(device_file, 'r')
    lines = f.readlines()
    f.close()
    return lines
 
def read_temp():
    lines = read_temp_raw()
    while lines[0].strip()[-3:] != 'YES':
        time.sleep(0.2)
        lines = read_temp_raw()
    equals_pos = lines[1].find('t=')
    if equals_pos != -1:
        temp_string = lines[1][equals_pos+2:]
        temp_c = float(temp_string) / 1000.0
        temp_f = temp_c * 9.0 / 5.0 + 32.0
        return temp_c, temp_f

#mqtt setup
client = mosquitto.Mosquitto("raspi B+")
client.connect("192.168.1.123")
client.loop_start()

# program variables
temp_c = 0
temp_f = 0
counter1 = 0
counter2 = 0
distance1 = 0
distance2 = 0
distance3 = 0
bad_reading = 0
distance_avg = 0
last_reading = 0
dist_check1 = 0
dist_check2 = 0
dist_check3 = 0
detect = 1

try:

    while True:

        if counter1 < 1:
            # get temp reading
            [temp_c, temp_f] = read_temp()
            print(temp_f)	
            time.sleep(.01)

            # send temp to server
            client.publish("yosh_temp", temp_f,0)

            time.sleep(5)

            # get level sensor reading
            if GPIO.input(LEVEL)==0:
                print("level off")
                client.publish("yosh_level", 0, 0)

            if GPIO.input(LEVEL)==1: # this means that the level is low
                print("level on") 
                client.publish("yosh_level", 1, 0)

        if counter2 < 1:
            
            # yoshi pic logic
            if bad_reading < 1:
                last_reading = distance_avg
                
            bad_reading = 0
            
            # take distance reading 1
            GPIO.output(TRIG, True)
            time.sleep(0.00001)
            GPIO.output(TRIG,False)

            while GPIO.input(ECHO)==0:
                pulse_start = time.time()

            while GPIO.input(ECHO)==1:    
                pulse_end= time.time()

            time.sleep(0.1)
            pulse_duration = pulse_end - pulse_start
            distance1 = pulse_duration * 17150
            distance1 = round(distance1, 2)

            print(distance1)
            time.sleep(1)
            
            # take distance reading 2
            GPIO.output(TRIG, True)
            time.sleep(0.00001)
            GPIO.output(TRIG,False)

            while GPIO.input(ECHO)==0:
                pulse_start = time.time()

            while GPIO.input(ECHO)==1:    
                pulse_end= time.time()

            time.sleep(0.1)
            pulse_duration = pulse_end - pulse_start
            distance2 = pulse_duration * 17150
            distance2 = round(distance2, 2)

            print(distance2)
            time.sleep(1)

            # take distance reading 3
            GPIO.output(TRIG, True)
            time.sleep(0.00001)
            GPIO.output(TRIG,False)

            while GPIO.input(ECHO)==0:
                pulse_start = time.time()

            while GPIO.input(ECHO)==1:    
                pulse_end= time.time()

            time.sleep(0.1)
            pulse_duration = pulse_end - pulse_start
            distance3 = pulse_duration * 17150
            distance3 = round(distance3, 2)

            print(distance3)
            time.sleep(1)

            # ensure no bad readings
            distance_avg = (distance1 + distance2 + distance3)/3
            dist_check1 = abs(distance_avg - distance1)
            dist_check2 = abs(distance_avg - distance2)
            dist_check3 = abs(distance_avg - distance3)
            
            if dist_check1 > 1:
                bad_reading = 1

            if dist_check2 > 1:
                bad_reading = 1

            if dist_check3 > 1:
                bad_reading = 1

            print(bad_reading)

            if bad_reading < 1:
                if last_reading > 0:
                    if last_reading - distance_avg > detect:

                        # take pic and save to local dir
                        with picamera.PiCamera() as camera:
                            camera.start_preview()
                            time.sleep(2)
                            camera.capture('/home/pi/kookiePix/currPic/img.jpg')
                            camera.stop_preview()

                        time.sleep(5)

                        # copy pic to different dir (active one gets overwritten)    
                        timestr = time.strftime("%Y%m%d-%H%M%S")
                        fname = '/home/pi/kookiePix/img'
                        fname += timestr
                        fname += '.jpg'

                        time.sleep(0.1)
                        shutil.copy2('/home/pi/kookiePix/currPic/img.jpg', fname)

                        time.sleep(20)
                        # send tweet
                        status = 'time for a little fun in the sun! #yoshi #tweepy'
                        api.update_with_media(photo_path, status=status)
                        
                        time.sleep(3600)
                        counter1 = 780
                        counter2 = 26
  

        counter1 = counter1 + 1
        counter2 = counter2 + 1
        print(counter1)
        print(counter2)
        time.sleep(5)

        if counter1 > 780:
            counter1 = 0

        if counter2 > 26:
            counter2 = 0

	if GPIO.input(BUTTON)==1:
	    bad_reading = 1
	    last_reading = 0
            GPIO.output(LED, True)
            time.sleep(1)
            GPIO.output(LED,False)
	    time.sleep(1)
	    GPIO.output(LED, True)
            time.sleep(1)
            GPIO.output(LED,False)
	    time.sleep(1)
	    GPIO.output(LED, True)
            time.sleep(1)
            GPIO.output(LED,False)
	    time.sleep(1)
	    while GPIO.input(BUTTON)==0:
		GPIO.output(LED, True)
            	time.sleep(1)
            	GPIO.output(LED,False)
	    	time.sleep(1)

            
except KeyboardInterrupt:
    print("user init escape")

except:
    print("exception has occurred")

finally:
    GPIO.cleanup()
    #client.loop_stop()
