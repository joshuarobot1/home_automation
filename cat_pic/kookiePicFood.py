# *************************************************************************************
# Creative Commons Attrib Share-Alike License
# You are free to use/extend this code/library but please abide with the CCSA license:
# http://creativecommons.org/licenses/by-sa/4.0/
# **********************************************************************************

# kookiePic.py v1.0 by JWR on 1/27/15

# program evaluates ultrasonic sensor,
# takes picture and sends to openHAB server
# via MQTT protocol

# ultrasonic sensor pinout:
# VCC - 5V
# GND - GND
# TRIG - GPIO23
# ECHO - GPIO24 (through voltage divider)



# module loading
import time
import RPi.GPIO as GPIO
import mosquitto
import picamera
import shutil
import tweepy

# init sensor
GPIO.setmode(GPIO.BCM)

TRIG = 23
ECHO = 24

GPIO.setup(TRIG, GPIO.OUT)
GPIO.setup(ECHO, GPIO.IN)

GPIO.output(TRIG, False)
print("initializing sensor")
time.sleep(2)

# init python-mosquitto vars
def on_publish(mosq, userdata, mid):
    step = 1
    mosq.disconnect()

client = mosquitto.Mosquitto("raspiA1")
client.on_publish = on_publish
client.connect("192.168.1.123")

# tweepy authorization
consumer_key = # removed for privacy
consumer_secret = # removed for privacy
access_token = # removed for privacy
access_token_secret = # removed for privacy

auth = tweepy.OAuthHandler(consumer_key, consumer_secret)
auth.set_access_token(access_token, access_token_secret)

api = tweepy.API(auth)

photo_path = '/home/pi/kookiePix/currPic/img.jpg'

# init program vars
step = 0
counter = 0
counter2 = 0
last_Count = 0
trigDist = 2000
distArray = [0,0,0]
print("setup complete")

# start program loop
try:
    while True:    
        # take initial dist readings
        while step==0:

            GPIO.output(TRIG, True)
            time.sleep(0.00001)
            GPIO.output(TRIG,False)

            while GPIO.input(ECHO)==0:
                pulse_start = time.time()

            while GPIO.input(ECHO)==1:    
                pulse_end= time.time()

            time.sleep(0.1)
            pulse_duration = pulse_end - pulse_start
            distance = pulse_duration * 17150
            distance = round(distance, 2)

            if distance>trigDist:
                step = 1
                loop1 = 1

            counter += 1    
#            print(distance)
            time.sleep(3)    

        # if trigDist detected    

        # take 3 dist readings (ensure no false readings)
        for loop1 in range(0,3):
            #print(loop1)
            GPIO.output(TRIG, True)
            time.sleep(0.00001)
            GPIO.output(TRIG,False)

            while GPIO.input(ECHO)==0:
                pulse_start = time.time()

            while GPIO.input(ECHO)==1:    
                pulse_end= time.time()

    #        print("inloop2")
            time.sleep(0.05)
            pulse_duration = pulse_end - pulse_start
            distance = pulse_duration * 17150
    #        print("inloop3")
            distance = round(distance, 2)
            distArray[loop1] = distance
    #        print("dist: ", distArray[loop1])
    #        loop1 += 1

            time.sleep(0.1)

        distance =  sum(distArray)/3
        #print(distance)
        
        # if cat detected
        if distance>trigDist:

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

            # begin mqtt transfer
#            f = open("/home/pi/kookiePix/currPic/img.jpg")
#            imagestring = f.read()
#            byteArray = bytes(imagestring)
#            client.publish("photo",byteArray, 0)
            
#            client.loop_start()

            print("pic taken - ready to tweet")     
            
#            time.sleep(60)

            # tweet picture
            #if counter==last_Count:
            #    status = 'im still up here you guys! #kookie #probablyasleep #tweepy'
            #else:
            status = 'im eating or drinking something! #kookie #omnomnom #tweepy'

            #print("test1")   
            api.update_with_media(photo_path, status=status)
            #print("test2")
            #update last counter
            last_Count = counter

            #pause for 20 mins
            time.sleep(1200)
            
            step = 0

        else:
            step = 0
            time.sleep(1)
        
# end of loop

except KeyboardInterrupt:
    print("user init escape")

except:
    print("exception has occurred")

finally:
    GPIO.cleanup()
    #client.loop_stop()
    


