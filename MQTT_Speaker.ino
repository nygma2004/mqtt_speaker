// MQTT Web Radio Project
// using Makerfabs ESP32 Audio Player hardware: https://www.makerfabs.com/eps32-audio-player.html
// Main features:
//  - Playing pre-programmed web radio stations 
//  - Clock with time updated from NTP server
//  - Radio station selection over MQTT
//  - Start audio file from MQTT - e.g. announcements
// https://github.com/nygma2004/mqtt_speaker
// Csongor Varga: csongor.varga@gmail.com

#include "Arduino.h"
#include "Audio.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include "icons.h"
#include "globals.h"
#include "settings.h"
#include <WiFiUdp.h>                // Added for NTP functionality
#include <sunMoon.h>                // Sunrise, sunset calculation
#include "NTP.h"                    // NTP functions
#include <PubSubClient.h>           // MQTT support

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Audio audio;
WiFiClient espClient;
PubSubClient mqtt(mqtt_server, 1883, 0, espClient);

void setup()
{
    //IO mode init
    pinMode(PIN_VOL_UP, INPUT_PULLUP);
    pinMode(PIN_VOL_DOWN, INPUT_PULLUP);
    pinMode(PIN_MUTE, INPUT_PULLUP);
    pinMode(PIN_PREVIOUS, INPUT_PULLUP);
    pinMode(PIN_PAUSE, INPUT_PULLUP);
    pinMode(PIN_NEXT, INPUT_PULLUP);

    //Serial
    Serial.begin(115200);
    Serial.println("MQTT Speaker");

    //LCD
    Wire.begin(MAKEPYTHON_ESP32_SDA, MAKEPYTHON_ESP32_SCL);
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    { // Address 0x3C for 128x32
        Serial.println(F("SSD1306 allocation failed"));
        for (;;)
            ; // Don't proceed, loop forever
    }
    display.clearDisplay();
    DisplayLogo();

    //connect to WiFi
    Serial.printf("Connecting to %s ", ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.printf("Connected to %s\n", ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // Start UDP for NTP function
    Serial.println(F("Starting UDP"));
    udp.begin(localPort);
    Serial.print(F("Local port: "));
    Serial.println(localPort);
    requestNTPUpdate();
    // Initialize sunrise and sunset calculator
    sm.init(GMTOffset, sm_latitude, sm_longtitude); 

     // Set up the MQTT server connection
    if (mqtt_server!="") {
      mqtt.setServer(mqtt_server, 1883);
      mqtt.setCallback(MQTTcallback);
      reconnect();
    }

    //Audio(I2S)
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(21); // 0...21

    open_new_radio(stations[station_index]);
    randomSeed(millis());
}

uint run_time = 0;
uint button_time = 0;

void loop()
{
    audio.loop();
    print_song_time();

    //Display logic
    if (millis() - run_time > 500)
    {
        run_time = millis();
        UpdateDisplay();
    }

    //Button logic
    if (millis() - button_time > 300)
    {
        if (digitalRead(PIN_NEXT) == 0)
        {
            Serial.println("Button: Next");
            if (station_index < station_count - 1)
            {
                station_index++;
            }
            else
            {
                station_index = 0;
            }
            button_time = millis();
            open_new_radio(stations[station_index]);
        }
        if (digitalRead(PIN_PREVIOUS) == 0)
        {
            Serial.println("Button: Previous");
            if (station_index > 0)
            {
                station_index--;
            }
            else
            {
                station_index = station_count - 1;
            }
            button_time = millis();
            open_new_radio(stations[station_index]);
        }
        if (digitalRead(PIN_VOL_UP) == 0)
        {
            Serial.println("Button: Volume Up");
            if (volume < 21)
                volume++;
            audio.setVolume(volume);
            button_time = millis();
        }
        if (digitalRead(PIN_VOL_DOWN) == 0)
        {
            Serial.println("Button: Volume Down");
            if (volume > 0)
                volume--;
            audio.setVolume(volume);
            button_time = millis();
        }
        if (digitalRead(PIN_MUTE) == 0)
        {
            Serial.println("Button: Mute");
            if (volume != 0)
            {
                mute_volume = volume;
                volume = 0;
            }
            else
            {
                volume = mute_volume;
            }
            audio.setVolume(volume);
            button_time = millis();
        }
        if (digitalRead(PIN_PAUSE) == 0)
        {
            Serial.println("Button: Pause");
            audio.pauseResume();
            button_time = millis();
            paused = !paused;
            screenmode = paused ? 0 : 1;
        }
    }
  
  // Check if NTP update is due (every hour)
  if ((millis() - NTPUpdateMillis >= 60*60*1000) && (!NTPRequested)) {  
    requestNTPUpdate();
  }    
  // reset the NTPRequested flag if there is no response after 1 minute from requested
  if ((millis() - NTPRequestMillis >= 60*1000) && (NTPRequested)) {  
    NTPRequested = false;
  }    
  handleNTPResponse();

  // Handle MQTT connection/reconnection
  if (mqtt_server!="") {
    if (!mqtt.connected()) {
      reconnect();
    }
    mqtt.loop();
  }
  // Send status update message
  handleMQTTStatus();
}

void renderTime() {
  // Construct the current time string
  if (NTPUpdateMillis>0) {
    NTPtime  = "";
    unsigned long epoch2 = epoch+((millis()-NTPUpdateMillis) / 1000);
    if (((epoch2 % 86400L) / 3600)<10) {
      // add leading zero if hour is less than 10
      NTPtime  += " ";
    }
    NTPtime  += (epoch2 % 86400L) / 3600; // print the hour (86400 equals secs per day)
    NTPtime  += ":";
    if ( ((epoch2 % 3600) / 60) < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      NTPtime  += "0";
    }
    NTPtime  += (epoch2  % 3600) / 60; // print the minute (3600 equals secs per minute)
    NTPsec = "";
    if (epoch2 % 60 < 10) {
      NTPsec = "0";
    }
    NTPsec += epoch2 % 60;
  } else {
    NTPtime = "--:--";
    NTPsec = "--";
  }  
}

void handleNTPResponse() {
  // Check for NTP response
  int cb = udp.parsePacket();
  if (cb!=0) {
    NTPUpdateMillis = millis();
    NTPRequested = false;
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    const unsigned long seventyYears = 2208988800UL;
    epoch = secsSince1900 - seventyYears;
    if (summerTime(epoch)) {
      epoch += 3600; // add 1 hour DST
    }
    epoch += GMTOffset * 60;
    Serial.print(F("NTP Update (epoch): "));
    Serial.println(epoch);
    Serial.print(F("NTP Update (time): "));
    printDate(epoch);  Serial.println("");

    // Sunrise and Sunset calculations
    time_t sRise = sm.sunRise(epoch);
    time_t sSet  = sm.sunSet(epoch);
    if (summerTime(epoch)) {
      sRise += 3600; // add 1 hour DST
      sSet += 3600; // add 1 hour DST
    }
    renderTime();
    Serial.print(F("Today sunrise and sunset: "));
    printDate(sRise); Serial.print(F("; "));
    printDate(sSet);  Serial.println("");
  }  
}

void handleMQTTStatus() {
  if (millis() - lastStatus >= 60*1000) {  
    lastStatus = millis();
    refreshStats();
    if (mqtt_server!="") {
      mqtt.publish(topicStatus, mqttStat.c_str());
      Serial.print(F("Status: "));
      Serial.println(mqttStat);
    }
  }    
}

void print_song_time()
{
    runtime = audio.getAudioCurrentTime();
    length = audio.getAudioFileDuration();
}

void open_new_radio(String station)
{
    audio.connecttohost(station);
    runtime = audio.getAudioCurrentTime();
    length = audio.getAudioFileDuration();
    Serial.println("**********start a new radio************");
}

void UpdateDisplay()
{
    int line_step = 24;
    int line = 0;
    char buff[20];
    anicount++;
    // Paint the Radion screen
    if (screenmode == SCREEN_RADIO) {
      sprintf(buff, "%d:%02d",runtime/60,runtime % 60);
  
      display.clearDisplay();
      display.setFont();
      display.drawBitmap(0, 0, background, 128, 64, 1);
  
      display.setTextSize(1);              // Normal 1:1 pixel scale
      display.setTextColor(SSD1306_WHITE); // Draw white text
      display.setCursor(OLED_RSSI_X,OLED_RSSI_Y);
      int rssi = WiFi.RSSI();
      display.println(WiFi.RSSI());
      if (rssi<-80) {
        display.drawBitmap(0, 0, icon_wifi[0], 16, 16, 1);
      } else {
         if (rssi<-70) {
            display.drawBitmap(0, 0, icon_wifi[1], 16, 16, 1); 
         } else {
            if (rssi<-60) {
              display.drawBitmap(0, 0, icon_wifi[2], 16, 16, 1); 
            } else {
              display.drawBitmap(0, 0, icon_wifi[3], 16, 16, 1); 
            }
         }
      }
      display.drawBitmap(45, 0, icon_volume[volume], 16, 16, 1); 
      display.setCursor(70, OLED_RSSI_Y); // Start at top-left corner
      display.println(NTPtime);
      
      display.setCursor(35, 38); // Start at top-left corner
      display.println(stationnames[station_index]);
      line += line_step * 2;
  
      display.setCursor(35, line);
      display.println(buff);
      line += line_step;
  
      display.drawBitmap(0, 30, radio[anicount % 4], 32, 32, 1);
  
  
      display.display();
    }
    
    // Paint the time screen
    if (screenmode == SCREEN_TIME) {
      renderTime();
      if (anicount % 2 ==0) {
        // calculate random movement
        int randmove = random(4);
        // moving right
        if (randmove==0) {
          clockx--;
          if (clockx<0) { clockx = 1; }
        }
        // moving up
        if (randmove==1) {
          clocky--;
          if (clocky<0) { clocky = 1; }
        }
        // moving left
        if (randmove==2) {
          clockx++;
          if (clockx>CLOCK_MAX_Y) { clockx = CLOCK_MAX_X-1; }
        }
        // moving down
        if (randmove==3) {
          clocky++;
          if (clocky>CLOCK_MAX_Y) { clocky = CLOCK_MAX_Y-1; }
        }
      }
      display.clearDisplay();
      display.setFont(&FreeSans12pt7b);
      display.setTextColor(SSD1306_WHITE); // Draw white text
      display.setCursor(clockx, clocky+24); 
      display.println(NTPtime);
      display.setFont(&FreeSans9pt7b);
      display.setCursor(clockx+20, clocky+24+16); 
      display.println(NTPsec);      
      display.display();
    }

    // Paint the announcement screen
    if (screenmode == SCREEN_PLAY) {
      sprintf(buff, "%d:%02d",runtime/60,runtime % 60);
  
      display.clearDisplay();
      display.setFont();
      display.drawBitmap(0, 0, background, 128, 64, 1);
  
      display.setTextSize(1);              // Normal 1:1 pixel scale
      display.setTextColor(SSD1306_WHITE); // Draw white text
      display.setCursor(OLED_RSSI_X,OLED_RSSI_Y);
      int rssi = WiFi.RSSI();
      display.println(WiFi.RSSI());
      if (rssi<-80) {
        display.drawBitmap(0, 0, icon_wifi[0], 16, 16, 1);
      } else {
         if (rssi<-70) {
            display.drawBitmap(0, 0, icon_wifi[1], 16, 16, 1); 
         } else {
            if (rssi<-60) {
              display.drawBitmap(0, 0, icon_wifi[2], 16, 16, 1); 
            } else {
              display.drawBitmap(0, 0, icon_wifi[3], 16, 16, 1); 
            }
         }
      }
      display.drawBitmap(45, 0, icon_volume[volume], 16, 16, 1); 
      display.setCursor(70, OLED_RSSI_Y); // Start at top-left corner
      display.println(NTPtime);
      
      line += line_step * 2;
  
      display.setCursor(35, line);
      display.println(buff);
      line += line_step;
  
      display.drawBitmap(0, 30, speaker[anicount % 4], 32, 32, 1);
  
  
      display.display();

      // reset the last mode if announcement completed
      if (!audio.isRunning()) { 
        screenmode = lastmode; 
        // Also start the last station if required
        if (screenmode == SCREEN_RADIO) {
          open_new_radio(stations[station_index]);
        }
      }
    }

}

void DisplayLogo(void)
{
    display.clearDisplay();
    display.drawBitmap(0, 0, splashscreen, 128, 64, 1);
    display.display();
    delay(2000);
}

// MQTT reconnect logic
void reconnect() {
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt.connect(clientID, mqtt_user, mqtt_password)) {
      Serial.println(F("connected"));
      // ... and resubscribe
      mqtt.subscribe(topicVolume);
      Serial.print(F("Subscribed to "));
      Serial.println(topicVolume);
      mqtt.subscribe(topicRadio);
      Serial.print(F("Subscribed to "));
      Serial.println(topicRadio);
      mqtt.subscribe(topicPlay);
      Serial.print(F("Subscribed to "));
      Serial.println(topicPlay);
    } else {
      Serial.print(F("failed, rc="));
      Serial.print(mqtt.state());
      Serial.println(F(" try again in 5 seconds"));
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// This routing just puts the status string together which will be sent over MQTT
void refreshStats() {
  // Initialize the strings for MQTT 
  mqttStat = "{\"rssi\":";
  mqttStat += WiFi.RSSI();
  mqttStat += ",\"uptime\":";
  mqttStat += millis()/1000/60;
  mqttStat += "}";
}

// MQTT callback function
void MQTTcallback(char* topic, byte* payload, unsigned int length) {
  // Convert the incoming byte array to a string
  String strTopic = String((char*)topic);
  payload[length] = '\0'; // Null terminator used to terminate the char array
  String message = (char*)payload;

  Serial.print(F("Message arrived on topic: ["));
  Serial.print(strTopic);
  Serial.print(F("], "));
  Serial.println(message);

  if (strTopic==(String)topicVolume) {
    int newvolume = atoi((char *)payload);
    if ((newvolume>=0) && (newvolume<22)) {
      Serial.print(F("New volume: "));
      volume = newvolume;
      audio.setVolume(volume);
      Serial.println(volume);  
    }
  }
  if (strTopic==(String)topicRadio) {
    int newstation = atoi((char *)payload);
    if ((newstation>=0) && (newstation<=station_count)) {
      Serial.print(F("New station: "));
      station_index=newstation;
      open_new_radio(stations[station_index]);
      Serial.println(stationnames[newstation]);  
    }
    if (newstation==-1) {
      if (audio.isRunning()) {
        audio.pauseResume();
        Serial.print(F("Radio stopped."));
      }
    }
  }
  if (strTopic==(String)topicPlay) {
    Serial.println(F("Playing..."));
    String url;
    lastmode = screenmode;
    screenmode = SCREEN_PLAY;
    open_new_radio(String(url+(char *)payload));
  }
}


//**********************************************
// optional
void audio_info(const char *info)
{
    Serial.print("info        ");
    Serial.println(info);
}
void audio_id3data(const char *info)
{ //id3 metadata
    Serial.print("id3data     ");
    Serial.println(info);
}

void audio_eof_mp3(const char *info)
{ //end of file
    Serial.print("eof_mp3     ");
    Serial.println(info);
}
void audio_showstation(const char *info)
{
    Serial.print("station     ");
    Serial.println(info);
}
void audio_showstreaminfo(const char *info)
{
    Serial.print("streaminfo  ");
    Serial.println(info);
}
void audio_showstreamtitle(const char *info)
{
    Serial.print("streamtitle ");
    Serial.println(info);
}
void audio_bitrate(const char *info)
{
    Serial.print("bitrate     ");
    Serial.println(info);
}
void audio_commercial(const char *info)
{ //duration in sec
    Serial.print("commercial  ");
    Serial.println(info);
}
void audio_icyurl(const char *info)
{ //homepage
    Serial.print("icyurl      ");
    Serial.println(info);
}
void audio_lasthost(const char *info)
{ //stream URL played
    Serial.print("lasthost    ");
    Serial.println(info);
}
void audio_eof_speech(const char *info)
{
    Serial.print("eof_speech  ");
    Serial.println(info);
}
