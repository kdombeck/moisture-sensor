#include <RH_RF95.h>
#include <Adafruit_FeatherOLED_WiFi.h>

// Radio Config
// for feather32u4
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 7

// for feather m0
//#define RFM95_CS 8
//#define RFM95_RST 4
//#define RFM95_INT 3

#define RF95_FREQ 915.0

RH_RF95 rf95(RFM95_CS, RFM95_INT);

// Blinky on receipt
#define LED 13

// Oled Config
const int aButtonPin = 9; // a = 9, b = 6, c = 5
int aButtonPushCounter = 0;
int aButtonState = 0;
int aLastButtonState = 0;

Adafruit_FeatherOLED_WiFi oled = Adafruit_FeatherOLED_WiFi();

void setup() {
  // radio setup
  pinMode(LED, OUTPUT);
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  Serial.println("Feather LoRa TX Test!");

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
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);

  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);

  // oled setup
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oled.init();

  pinMode(aButtonPin, INPUT_PULLUP);
}

void loop() {
  aButtonState = digitalRead(aButtonPin);

  // compare the buttonState to its previous state
  if (aButtonState != aLastButtonState) {
    // if the state has changed, increment the counter
    if (aButtonState == LOW) {
      // if the current state is LOW then the button was pressed
      aButtonPushCounter++;
      char radiopacket[24] = "A button pushed #      ";
      itoa(aButtonPushCounter, radiopacket+17, 10);
      Serial.print("Sending "); Serial.println(radiopacket);
      radiopacket[23] = 0;

      oled.clearDisplay();
      oled.setCursor(0,0);
      oled.print("Sending "); oled.println(radiopacket);
      oled.display();

      Serial.println("Sending..."); delay(10);
      rf95.send((uint8_t *)radiopacket, 20);

      oled.println("Waiting for response");
      oled.display();

      Serial.println("Waiting for packet to complete..."); delay(10);
      rf95.waitPacketSent();

      // Now wait for a reply
      uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
      uint8_t len = sizeof(buf);

      Serial.println("Waiting for reply..."); delay(10);
      if (rf95.waitAvailableTimeout(1000)) {
        // Should be a reply message for us now
        if (rf95.recv(buf, &len)) {
          digitalWrite(LED, HIGH);
          Serial.print("Got reply: ");
          Serial.println((char*)buf);
          Serial.print("RSSI: ");
          Serial.println(rf95.lastRssi(), DEC);
          oled.print("Reply: "); oled.println((char*)buf);
          oled.display();
        } else {
          Serial.println("Receive failed");
          oled.println("Receive failed");
          oled.display();
        }
      } else {
        Serial.println("No reply, is there a listener around?");
        oled.println("No reply");
        oled.display();
      }

      delay(1000);
      digitalWrite(LED, LOW);
    }
  }

  oled.clearDisplay();
  oled.setCursor(0,0);
  oled.print("A pressed: "); oled.println(aButtonPushCounter);
  oled.display();

  aLastButtonState = aButtonState;
}
