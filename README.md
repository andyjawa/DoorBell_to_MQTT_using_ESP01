# DoorBell_to_MQTT_GoogleHome
Connect ESP-01 to existing doorbell to communicate to MQTT server with announcement to google home / pushbullet 
Note that this project is not replacing the doorbell switch nor the chime.  It still uses regular mechanical switch.

This project adds ESP-01 module into existing 16VAC doorbell system adding MQTT functionality to send message to
MQTT broker (when doorbell switch is pressed).  MQTT broker would relay the message to NodeRED.
Upon receiving MQTT message, NodeRED would send announcement to multiple google home devices and also optionally
send message to cellphone/browser via pushbullet

I have a passion for DIY home automation, and slowly adding IOT to my home.  Google home has been one of the central
function in my home automation.  My son gave me the idea for this project when he asked me if I can make the google
home announce each time somebody ring the doorbell.  We have 3 stories home, and lots of time we can't hear the 
doorbell when we're in our basement or upstairs in bedroom with door closed or with TV on.  We have 4 google home mini
in our house from basement to upstairs. By making google homes announce a doorbell, we know immediately anywhere in 
the house if somebody ring the doorbell.

References:
Doorbell wiring diagrams:
   https://diyhousehelp.com/how-to/doorbell-wiring-diagrams
