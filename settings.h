// Also update the latitude and longitude values and GMT offset at the top of NTP.h

const char *ssid = "xxxx";                               // Wifi SSID
const char *password = "yyy";                            // Wifi password
const char* mqtt_server = "192.168.1.xx";                // MQTT server address, leave empty to disable MQTT function
const char* mqtt_user = "xxx";                           // MQTT user id
const char* mqtt_password = "xxx";                       // MQTT password
const char* clientID = "mqtt_radio";                     // MQTT client ID
const char* topicStatus = "/mqttradio/status";           // MQTT topic where the device sends updates every 10 seconds
const char* topicVolume = "/mqttradio/volume";           // update volume over http 0..21
const char* topicRadio = "/mqttradio/radio";             // Change the radio station
const char* topicPlay = "/mqttradio/play";               // Send a mp3 URL to play

String stations[] = {
    "mr-stream.mediaconnect.hu/4741/mr3.mp3",
    "stream001.radio.hu:8080/mr2.mp3",
    "192.168.1.80:8080/voice/output_MP3WRAP.mp3",
    "stream.1a-webradio.de/deutsch/mp3-128/vtuner-1a",
    "mp3.ffh.de/radioffh/hqlivestream.aac", //  128k aac
    "www.antenne.de/webradio/antenne.m3u",
    "listen.rusongs.ru/ru-mp3-128",
    "edge.audio.3qsdn.com/senderkw-mp3",
    "macslons-irish-pub-radio.com/media.asx"};
String stationnames[] = {
    "Bartok Radio",
    "Petofi Radio",
    "Reggeli uzenet",
    "Test 2",
    "Test 3", //  128k aac
    "Test 4",
    "Test 5",
    "Test 6",
    "Test 7"};
int station_index = 0;
int station_count = sizeof(stations) / sizeof(stations[0]);
