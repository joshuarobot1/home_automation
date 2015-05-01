# kookiePicFoodIR.py v1.0 by JWR on 1/27/15

# program accepts inputs from an arduino,
# takes picture and sends to openHAB server
# via MQTT protocol

# the program is protected under the creative
# commons license CC-BY-SA, https://creativecommons.org/licenses/by-sa/2.0/

# module loading
import time
import RPi.GPIO as GPIO
#import mosquitto
import picamera
import shutil
import tweepy
import random

# init sensor
GPIO.setmode(GPIO.BCM)

eat = 23
drink = 24

GPIO.setup(eat, GPIO.IN)
GPIO.setup(drink, GPIO.IN)

# init python-mosquitto vars
#def on_publish(mosq, userdata, mid):
#    step = 1
#    mosq.disconnect()

#client = mosquitto.Mosquitto("raspiA1")
#client.on_publish = on_publish
#client.connect("192.168.1.123")

# tweepy authorization
consumer_key = 'RU4wdre6gS7KteE58MUmshSZE'
consumer_secret = 'eIYzWkGsa1I6rXawmQ0JSWJ8KBFkZXIUZLEZfsEHibBUXNFBqz'
access_token = '3002829234-N24mCitCGgjnb2KrZ8SNbiL2vuxfzjREAL3vkfy'
access_token_secret = 'Lg3yoAiZirJdLhoPNsxZkZ0Jds7puffk2qbpuOVl3zXbg'

auth = tweepy.OAuthHandler(consumer_key, consumer_secret)
auth.set_access_token(access_token, access_token_secret)

api = tweepy.API(auth)

photo_path = '/home/pi/kookiePix/currPic/img.jpg'

# init program vars
step = 0
counter = 0
counter2 = 0
last_Count = 0
kookEat = 0
kookDrink = 0
print("setup complete")

# start program loop
try:
    while True:    
        if GPIO.input(eat)==1:
	    kookEat = 1
	    print('kookie eating')
	elif GPIO.input(eat)==0:
	    kookEat = 0
	    #print('kookie not eating')

	if GPIO.input(drink)==1:
	    kookDrink = 1
	    print('kookie drinking')
	elif GPIO.input(drink)==0:
	    kookDrink = 0
	    #print('kookie not drinking')
 	
	time.sleep(.5)

        # if cat detected
        if kookEat or kookDrink:

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
#           #f = open("/home/pi/kookiePix/currPic/img.jpg")
#           # imagestring = f.read()
#           # byteArray = bytes(imagestring)
#           # client.publish("photo",byteArray, 0)
            
#            client.loop_start()

            print("pic taken - ready to tweet")     
            
#            time.sleep(60)

            # tweet picture
	    n = random.randint(1,3)
	    if kookEat:
		if n == 1:
            	    status = 'i\'m eating food! #kookie #omnomnom #tweepy'
		if n == 2:
		    status = 'FOOOOOOOOOOD! #kookie #omnomnom #tweepy'
		if n == 3:
		    status = 'hey, there\'s food in here! #kookie #omnomnom #tweepy'

	    if kookDrink:
		if n == 1:
		    status = 'i\'m drinking water you guys! #kookie #refreshed #tweepy'
		if n == 2:
		    status = 'enjoy this closeup of me drinking! #kookie #refreshed #tweepy'
		if n == 3:
		    status = 'water again?! i guess that\'s cool since i\'m a cat. #kookie #refreshed #tweepy'

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
    



                
