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

// Oled Config
Adafruit_FeatherOLED_WiFi oled = Adafruit_FeatherOLED_WiFi();

// Blinky on receipt
#define LED 13

int received = 0;

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
}

uint32_t timer = millis();

void loop() {
  if (rf95.available()) {
    // Should be a message for us now
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    if (rf95.recv(buf, &len)) {
      received++;
      timer = millis(); // reset the timer
      digitalWrite(LED, HIGH);
      RH_RF95::printBuffer("Received: ", buf, len);
      Serial.print("Got: ");
      Serial.println((char*)buf);
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);

      oled.clearDisplay();
      oled.setCursor(0,0);
      oled.print("Got: "); oled.println((char*)buf);
      oled.display();

      // Send a reply
      uint8_t data[] = "Got it";
      rf95.send(data, sizeof(data));
      rf95.waitPacketSent();
      Serial.println("Sent a reply");
      oled.println("Sent a rely");
      digitalWrite(LED, LOW);
    } else {
      Serial.println("Receive failed");
      oled.println("Receive failed");
    }
  }

  // if millis() or timer wraps around, we'll just reset it
  if (timer > millis()) timer = millis();

  // display old data for a period of time so that you can see it
  if (millis() - timer > 2000) {
    oled.clearDisplay();
    oled.setCursor(0,0);
    oled.print("received: "); oled.println(received);
    oled.display();
  }
}
