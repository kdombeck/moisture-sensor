/*
 *  WiFi OLED Display Badge
 *  guide: https://learn.adafruit.com/digital-display-badge/
 *  based on Home Automation sketch by M. Schwartz
 *  with contributions from Becky Stern and Tony Dicola
 */

#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "secrets.h"

// Adafruit IO
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883

// Functions
void connect();

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// or... use WiFiFlientSecure for SSL
//WiFiClientSecure client;

// Store the MQTT server, client ID, username, and password in flash memory.
// This is required for using the Adafruit MQTT library.
const char MQTT_SERVER[] PROGMEM    = AIO_SERVER;
// Set a unique MQTT client ID using the AIO key + the date and time the sketch
// was compiled (so this should be unique across multiple devices for a user,
// alternatively you can manually set this to a GUID or other random value).
const char MQTT_CLIENTID[] PROGMEM  = AIO_KEY __DATE__ __TIME__;
const char MQTT_USERNAME[] PROGMEM  = AIO_USERNAME;
const char MQTT_PASSWORD[] PROGMEM  = AIO_KEY;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, AIO_SERVERPORT, MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD);

/****************************** Feeds ***************************************/

// Setup a feed called 'hightemp' for subscribing to changes.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
const char hightemp_FEED[] PROGMEM = AIO_USERNAME "/feeds/hightemp";
Adafruit_MQTT_Subscribe hightemp = Adafruit_MQTT_Subscribe(&mqtt, hightemp_FEED);

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET 3
Adafruit_SSD1306 display(OLED_RESET);

#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH  16
static const unsigned char PROGMEM logo16_glcd_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 };

#if (SSD1306_LCDHEIGHT != 32)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

// this constant won't change:
const int  buttonPin = 2;    // the pin that the pushbutton is attached to

// Variables will change:
int buttonPushCounter = 0;   // counter for the number of button presses
int buttonState = 0;         // current state of the button
int lastButtonState = 0;     // previous state of the button

void setup() {
  Serial.begin(115200);
  delay(100);

  // initialize the button pin as a input:
  pinMode(buttonPin, INPUT_PULLUP);

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  // init done

  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.display();
  delay(2000);

  // Clear the buffer.
  display.clearDisplay();

  // We start by connecting to a WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print("Connected to ");
  display.println(ssid);
  display.setCursor(0,16);
  display.print("IP address: ");
  display.println(WiFi.localIP());
  display.display();
  delay(2000);

  // listen for events on the weather feed
  mqtt.subscribe(&hightemp);

  // connect to adafruit io
  connect();
}

int temp = 151;

void loop() {
  Adafruit_MQTT_Subscribe *subscription;

  // ping adafruit io a few times to make sure we remain connected
  if(! mqtt.ping(3)) {
    // reconnect to adafruit io
    if(! mqtt.connected())
      connect();
  }

  // read the pushbutton input pin:
  buttonState = digitalRead(buttonPin);

  // compare the buttonState to its previous state
  if (buttonState != lastButtonState) {
    // if the state has changed, increment the counter
    if (buttonState == LOW) {
      // if the current state is LOW then the button was pressed
      buttonPushCounter++;
      Serial.print("number of button pushes:  ");
      Serial.println(buttonPushCounter);
    }
  }

  // this is our 'wait for incoming subscription packets' busy subloop
  if (subscription = mqtt.readSubscription(1000)) {
    // we only care about the weather events
    if (subscription == &hightemp) {
      temp = atoi((char*)hightemp.lastread);
      Serial.print(F("Received: "));
      Serial.println(temp);
    }
  }

  //A button position - display today's high temperature
  if (buttonPushCounter % 3 == 0) {
    if (temp == 151) {
      display.clearDisplay();
      display.setCursor(0,16);
      display.println("Waiting for temp data");
      display.display();
      Serial.println(F("Printed: Waiting for temp data"));
    } else {
      display.clearDisplay();
      display.setCursor(0,16);
      display.print("Today's high: ");
      display.print(temp);
      display.println(" F");
      display.display();
      Serial.print(F("Printed: "));
      Serial.println(temp);
    }
  }

  //B button position - David Bowie quote
  if (buttonPushCounter % 3 == 1) {
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("I'm an instant star. Just add water and ");
    display.println("stir.");
    display.println("       - David Bowie");
    display.display();
  }

  //C button position - nametag
  if (buttonPushCounter % 3 == 2) {
      display.clearDisplay();
      display.setCursor(0,0);
      display.println("Your Name");
      display.println();
      display.println("Your Title");
      display.display();
  }

  // save the current state as the last state,
  //for next time through the loop
  lastButtonState = buttonState;
}

// connect to adafruit io via MQTT
void connect() {
  Serial.print(F("Connecting to Adafruit IO... "));

  int8_t ret;

  while ((ret = mqtt.connect()) != 0) {
    switch (ret) {
      case 1: Serial.println(F("Wrong protocol")); break;
      case 2: Serial.println(F("ID rejected")); break;
      case 3: Serial.println(F("Server unavail")); break;
      case 4: Serial.println(F("Bad user/pass")); break;
      case 5: Serial.println(F("Not authed")); break;
      case 6: Serial.println(F("Failed to subscribe")); break;
      default: Serial.println(F("Connection failed")); break;
    }

    if(ret >= 0)
      mqtt.disconnect();

    Serial.println(F("Retrying connection..."));
    delay(5000);
  }

  Serial.println(F("Adafruit IO Connected!"));
}
