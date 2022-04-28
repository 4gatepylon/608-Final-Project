// General definitions and constants...
#define BACKGROUND TFT_GREEN
#define BALL_COLOR TFT_BLUE

#define DT 40

const int MAX_APS = 20;

enum Direction
{
    // None is identical to speed 0 in any direction
    NONE,
    UP,
    DOWN,
    LEFT,
    RIGHT
};

const char PREFIX[] = "{\"wifiAccessPoints\": [";                 // beginning of json body
const char SUFFIX[] = "]}";                                       // suffix to POST request
const char API_KEY[] = "AIzaSyAQ9SzqkHhV-Gjv-71LohsypXUH447GWX8"; // don't change this and don't share this

uint8_t channel = 1;                                 // network channel on 2.4 GHz
byte bssid[] = {0x04, 0x95, 0xE6, 0xAE, 0xDB, 0x41}; // 6 byte MAC address of AP you're targeting.

esp_now_peer_info_t peerInfo;

struct Vec
{
    float x;
    float y;
};

struct WireData
{
    Vec tilt;
    float angle;
    float speed;
    float lat;
    float lon;
    Direction direction;
};

// Wifi library functions!
// NOTE this will be copied once per file, so do NOT include lib.h in more than one location for now
// We can seperate into multiple files and then use guards once we have a better build tool.
const int RESPONSE_TIMEOUT = 6000;     // ms to wait for response from host
const int POSTING_PERIOD = 6000;       // periodicity of getting a number fact.
const uint16_t IN_BUFFER_SIZE = 3500;  // size of buffer to hold HTTP request
const uint16_t OUT_BUFFER_SIZE = 3500; // size of buffer to hold HTTP response
const uint16_t JSON_BODY_SIZE = 3000;

char network[] = "MIT GUEST";
char password[] = "";
const char user[] = "";
char request_buffer[IN_BUFFER_SIZE];   // char array buffer to hold HTTP request
char response_buffer[OUT_BUFFER_SIZE]; // char array buffer to hold HTTP response

/*----------------------------------
 * char_append Function:
 * Arguments:
 *    char* buff: pointer to character array which we will append a
 *    char c:
 *    uint16_t buff_size: size of buffer buff
 *
 * Return value:
 *    boolean: True if character appended, False if not appended (indicating buffer full)
 */
