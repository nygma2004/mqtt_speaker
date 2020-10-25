//Digital I/O used  //Makerfabs Audio V2.0
#define I2S_DOUT 27
#define I2S_BCLK 26
#define I2S_LRC 25

//SSD1306
#define MAKEPYTHON_ESP32_SDA 4
#define MAKEPYTHON_ESP32_SCL 5
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1    // Reset pin # (or -1 if sharing Arduino reset pin)
#define PIN_VOL_UP 39
#define PIN_VOL_DOWN 36
#define PIN_MUTE 35
#define PIN_PREVIOUS 15
#define PIN_PAUSE 33
#define PIN_NEXT 2
#define OLED_RSSI_X 18
#define OLED_RSSI_Y 5
#define CLOCK_MAX_X 65
#define CLOCK_MAX_Y 20

#define SCREEN_TIME 0
#define SCREEN_RADIO 1
#define SCREEN_PLAY 2

struct Music_info
{
    String name;
    int length;
    int runtime;
    int volume;
    int status;
    int mute_volume;
} music_info = {"", 0, 0, 0, 0, 0};

int volume = 21;
int mute_volume = 0;

int runtime = 0;
int length = 0;
unsigned long anicount = 0;
bool paused = false;

String NTPtime = "--:--";
String NTPsec = "--";
unsigned long lastNTP,lastStatus;
uint8_t screenmode = SCREEN_RADIO;
uint8_t lastmode = screenmode;
String mqttStat = "";

int clockx = 100;
int clocky = 10;
