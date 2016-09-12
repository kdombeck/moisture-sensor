#include <SPI.h>
#include <RH_RF95.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include <Adafruit_WINC1500.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "secrets.h"

// LoRa breakout setup
#define RFM95_CS 10
#define RFM95_RST 11
#define RFM95_INT 6
#define RF95_FREQ 915.0

RH_RF95 rf95(RFM95_CS, RFM95_INT);

// MQTT and WIFI setup
#define WINC_CS   8
#define WINC_IRQ  7
#define WINC_RST  4
#define WINC_EN   2

Adafruit_WINC1500 WiFi(WINC_CS, WINC_IRQ, WINC_RST);

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883

Adafruit_WINC1500Client client;

int status = WL_IDLE_STATUS;

// Store the MQTT server, client ID, username, and password in flash memory.
// This is required for using the Adafruit MQTT library.
const char MQTT_SERVER[] PROGMEM    = AIO_SERVER;
// Set a unique MQTT client ID using the AIO key + the date and time the sketch
// was compiled (so this should be unique across multiple devices for a user,
// alternatively you can manually set this to a GUID or other random value).
const char MQTT_CLIENTID[] PROGMEM  = "WINC1500-client";
const char MQTT_USERNAME[] PROGMEM  = AIO_USERNAME;
const char MQTT_PASSWORD[] PROGMEM  = AIO_KEY;

Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, AIO_SERVERPORT, MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD);

// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
const char FEED_NAME_PREFIX[] PROGMEM = AIO_USERNAME "/feeds/";

// Oled Config
Adafruit_SSD1306 oled = Adafruit_SSD1306();

uint32_t oledRefreshTimer = millis();

int nbrMessagesReceived = 0;
int nbrMqttSuccessfulSent = 0;
int nbrMqttFailedToSend = 0;
int nbrInvalidMessages = 0;

void setup() {
//  while ( ! Serial ) { delay( 10 ); } // wait for serial connection
  Serial.begin(115200);

  Serial.println("LoRa setup");
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    while (1);
  }
  Serial.println("LoRa radio init OK!");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (true);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);

  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);

  Serial.println("WIFI setup");
  pinMode(WINC_EN, OUTPUT);
  digitalWrite(WINC_EN, HIGH);

  // check for the presence of the breakout
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WINC1500 not present");
    while (true);
  }
  Serial.println("ATWINC OK!");

  // oled setup
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oled.setTextSize(1);
  oled.setTextColor(WHITE);
  oled.clearDisplay();
  oled.setCursor(0,0);
  oled.println("LoRa Gateway");
  oled.display();
}

void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  if (rf95.available()) {
    // Should be a message for us now
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    if (rf95.recv(buf, &len)) {
      nbrMessagesReceived++;
      //RH_RF95::printBuffer("Received: ", buf, len);
      oled.clearDisplay();
      oled.setCursor(0,0);
      oled.print("rvd: "); oled.println((char*)buf);
      oled.display();
      Serial.print("Got:  "); Serial.println((char*)buf);
      Serial.print("RSSI: "); Serial.println(rf95.lastRssi(), DEC);

      String message = String((char*)buf);
      char reply[16];

      if (isValidMessage(message)) {
        // the feed name is first part of message prior to the first ',' (comma)
        String feedName = String(FEED_NAME_PREFIX);
        feedName.concat(message.substring(0, message.indexOf(',')));
        char feedNameBuf[feedName.length() + 1];
        feedName.toCharArray(feedNameBuf, feedName.length() + 1);

        oled.print("feedName: "); oled.println(feedNameBuf);
        oled.display();

        // the payload is everything after the first ',' (comma)
        String payload = String(message.substring(message.indexOf(',') + 1));
        char payloadBuf[payload.length() + 1];
        payload.toCharArray(payloadBuf, payload.length() + 1);

        Serial.print("feed name: "); Serial.println(feedNameBuf);
        Serial.print("payload:   "); Serial.println(payloadBuf);
        oled.print("MQTT: "); oled.println(payloadBuf);
        oled.display();

        if (mqtt.publish(feedNameBuf, payloadBuf)) {
          nbrMqttSuccessfulSent++;
          strncpy(reply, "MQTT OK", 7);
          oled.println("MQTT OK");
          oled.display();
        } else {
          nbrMqttFailedToSend++;
          Serial.println("MQTT failed");
          strncpy(reply, "MQTT failed", 11);
        }
      } else {
          strncpy(reply, "Invalid message", 15);
      }

      // Send a reply
      oled.clearDisplay();
      oled.setCursor(0,0);
      oled.print("reply: "); oled.println(reply);
      oled.display();
      rf95.send((uint8_t *)reply, sizeof(reply));
      rf95.waitPacketSent();
      Serial.print("Sent reply: "); Serial.println(reply);
    } else {
      Serial.println("Receive failed");
    }
  }

  // if millis() or timer wraps around, we'll just reset it
  if (oledRefreshTimer > millis()) oledRefreshTimer = millis();

  // print out the current stats only so often
  if (millis() - oledRefreshTimer > 2000) {
    oledRefreshTimer = millis(); // reset the timer
    oled.clearDisplay();
    oled.setCursor(0,0);
    oled.print("nbr received     "); oled.println(nbrMessagesReceived);
    oled.print("nbr MQTT success "); oled.println(nbrMqttSuccessfulSent);
    oled.print("nbr MQTT failed  "); oled.println(nbrMqttFailedToSend);
    oled.print("nbr invalid msg  "); oled.println(nbrInvalidMessages);
    oled.display();
  }
}

bool isValidMessage(const String& message) {
  // check to see that all characters are ASCII
  int messageLength = message.length();
  byte messageBuf[messageLength + 1];
  message.getBytes(messageBuf, messageLength + 1);
  for (int byteNbr = 0; byteNbr < messageLength; byteNbr++) {
    if (messageBuf[byteNbr] < 32 || messageBuf[byteNbr] > 127) {
      nbrInvalidMessages++;
      Serial.print("Invalid byte nbr: "); Serial.println(byteNbr); 
      Serial.print("Invalid byte: "); Serial.println(messageBuf[byteNbr]);
      Serial.print("Invalid message: "); Serial.println(message);
      return false;
    }
  }

  return true;
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // attempt to connect to Wifi network:
  while (WiFi.status() != WL_CONNECTED) {
    oled.clearDisplay();
    oled.setCursor(0,0);
    oled.println("connecting to WIFI");
    oled.println(WIFI_SSID);
    oled.display();
    Serial.print("Attempting to connect to SSID: "); Serial.println(WIFI_SSID);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // wait 10 seconds for connection:
    uint8_t timeout = 10;
    while (timeout && (WiFi.status() != WL_CONNECTED)) {
      timeout--;
      delay(1000);
    }
  }

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  oled.clearDisplay();
  oled.setCursor(0,0);
  oled.println("connecting to MQTT");
  oled.display();
  Serial.println("Connecting to MQTT... ");

  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
      oled.clearDisplay();
      oled.setCursor(0,0);
      oled.println("Failed to conn MQTT");
      oled.println(mqtt.connectErrorString(ret));
      oled.display();
      Serial.println(mqtt.connectErrorString(ret));
      Serial.println("Retrying MQTT connection in 5 seconds...");
      mqtt.disconnect();
      delay(5000);  // wait 5 seconds
  }

  oled.println("connected to MQTT");
  oled.display();
  Serial.println("MQTT Connected!");
}

