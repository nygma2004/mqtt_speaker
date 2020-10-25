# MQTT Speaker
This project started becasue I wanted to build my own Google Home, Alexa smart speaker without the microphone, so I can generate audio reports and announcements from Node-Red and have this device play them: like to announce if somebody pushed the doorbell, or weather forecast in the morning as a wake up alarm. And since the base hardware is an [ESP32 Audio Player module from Makerfabs](https://www.makerfabs.com/eps32-audio-player.html) which has web radio exmaple files, I figured I keep the functionality.
## Main Features
- Play web radio stations from pre-defined stations stored in the code
- Pause, play, next station, previous station buttons
- Volume up, down and mute buttons
- Change volume and stations over MQTT
- Send URL of an mp3 file via MQTT which gets played: these would be the announcements
## Customizing the Code
"All" the settings to be changes are in settings.h. There is sufficient explanation of what each setting if for. I will talk about the MQTT topic in a section below. The internet radio stations are stored in the stations[] and stationnames[] arrays. Please make sure that both arrays have the same number of entries. stationames[] gets printed on the OLED screen, size is limited.
As mentioned in the file, there is latitude, longitude and GMT Offset values in the NTP.h. Latitude and longitude may be required in the future, not yet. It calculates sunrise and sunset, which is not yet used
## Installing the Sketch
Libraries required:
- Time by Michael Margolis: also install this from the Arduino Library Manager
- SunMoon: sunrise and sunset calculation: https://github.com/sfrwmaker/sunMoon
- [ESP32-audioI2S](https://github.com/schreibfaul1/ESP32-audioI2S)("Audio.h")
- "Basic" libraries like ESP32 libraries, Pubsub for MQTT, Adafruit GFX for the OLED screen.
IMPORTANT: when the new firmware is being uploaded to the ESP, push the station selector button to right (towards the audio jack) and keep it like that until the flash upload is completed:
![Flashing firmware](https://github.com/Makerfabs/Project_ESP32-Web-Radio/raw/master/md_pic/Without_plug.png)
## MQTT Topics
The speaker uses a few MQTT topics to communicate with Node-Red. These topics can be configured in settings.h:
- topicStatus: device reports status on this topic every minute.
- topicVolume: change the volume on the device, values accepted from 0 to 21.
- topicRadio: change to the internet radio station that are stored in the stations[] array in settings.h. Values can be 0 to count-1, if -1 is sent it stops the currently playing internet radio station.
- topicPlay: send a URL to an MP3 on this topic without the http:// and that will get played immediately.
