// General definitions and constants...
#define BACKGROUND TFT_GREEN
#define BALL_COLOR TFT_BLUE

#define DT 40

enum Direction
{
    UP,
    DOWN,
    LEFT,
    RIGHT
};

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
    Direction direction;
};

// Wifi library functions!
// NOTE this will be copied once per file, so do NOT include lib.h in more than one location for now
// We can seperate into multiple files and then use guards once we have a better build tool.
const int RESPONSE_TIMEOUT = 6000;     // ms to wait for response from host
const int POSTING_PERIOD = 6000;       // periodicity of getting a number fact.
const uint16_t IN_BUFFER_SIZE = 2000;  // size of buffer to hold HTTP request
const uint16_t OUT_BUFFER_SIZE = 2000; // size of buffer to hold HTTP response

char network[] = "MIT";
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
uint8_t char_append(char *buff, char c, uint16_t buff_size)
{
    int len = strlen(buff);
    if (len > buff_size)
        return false;
    buff[len] = c;
    buff[len + 1] = '\0';
    return true;
}

/*----------------------------------
 * do_http_request Function:
 * Arguments:
 *    const char* host: null-terminated char-array containing host to connect to
 *    char* request: null-terminated char-arry containing properly formatted HTTP request
 *    char* response: char-array used as output for function to contain response
 *    uint16_t response_size: size of response buffer (in bytes)
 *    uint16_t response_timeout: duration we'll wait (in ms) for a response from server
 *    uint8_t serial: used for printing debug information to terminal (true prints, false doesn't)
 * Return value:
 *    void (none)
 */
void do_http_request(const char *host, char *request, char *response, uint16_t response_size, uint16_t response_timeout, uint8_t serial)
{
    WiFiClient client; // instantiate a client object
    if (client.connect(host, 80))
    { // try to connect to host on port 80
        if (serial)
            Serial.print(request); // Can do one-line if statements in C without curly braces
        client.print(request);
        memset(response, 0, response_size); // Null out (0 is the value of the null terminator '\0') entire buffer
        uint32_t count = millis();
        while (client.connected())
        { // while we remain connected read out data coming back
            client.readBytesUntil('\n', response, response_size);
            if (serial)
                Serial.println(response);
            if (strcmp(response, "\r") == 0)
            { // found a blank line!
                break;
            }
            memset(response, 0, response_size);
            if (millis() - count > response_timeout)
                break;
        }
        memset(response, 0, response_size);
        count = millis();
        while (client.available())
        { // read out remaining text (body of response)
            char_append(response, client.read(), OUT_BUFFER_SIZE);
        }
        if (serial)
            Serial.println(response);
        client.stop();
        if (serial)
            Serial.println("-----------");
    }
    else
    {
        if (serial)
            Serial.println("connection failed :/");
        if (serial)
            Serial.println("wait 0.5 sec...");
        client.stop();
    }
}