#include <RH_RF95.h>
#include <Adafruit_FeatherOLED_WiFi.h>
#include <Adafruit_SleepyDog.h>

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
// buttons a = 9, b = 6, c = 5
const int sendDataButtonPin = 9;
int sendDataLastButtonState = 0;
const int sleepButtonPin = 5;
int sleepLastButtonState = 0;

int nbrOfSentData = 0;
bool deepSleep = false;

Adafruit_FeatherOLED_WiFi oled = Adafruit_FeatherOLED_WiFi();

// Sensor config
int sensorPin = A0;

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

  oled.clearDisplay();
  oled.setCursor(0,0);
  oled.println("A - send sensor data");
  oled.println("B - send GPS (TODO)");
  oled.println("C - deep sleep");
  oled.display();

  delay(2000);

  pinMode(sendDataButtonPin, INPUT_PULLUP);
  pinMode(sleepButtonPin, INPUT_PULLUP);
}

void loop() {
  // check to see if sensor data should be sent
  int sendDataButtonState = digitalRead(sendDataButtonPin);
  if (sendDataButtonState != sendDataLastButtonState) {
    if (sendDataButtonState == LOW) {
      // if the current state is LOW then the button was pressed
      readAndSendSensorData();
    }
  }

  // check to see if the device should be put into deep sleep
  int sleepButtonState = digitalRead(sleepButtonPin);
  if (sleepButtonState != sleepLastButtonState) {
    if (sleepButtonState == LOW) {
      // if the current state is LOW then the button was pressed
      deepSleep = !deepSleep;
      Serial.print("sleep "); Serial.println(deepSleep);
    }
  }

  oled.clearDisplay();
  oled.setCursor(0,0);
  oled.print("nbr sent: "); oled.println(nbrOfSentData);
  oled.print("deep sleep: "); oled.println(deepSleep);
  oled.display();

  sendDataLastButtonState = sendDataButtonState;
  sleepLastButtonState = sleepButtonState;

  if (deepSleep) {
    Watchdog.sleep(1000);
  }
}

void readAndSendSensorData() {
  nbrOfSentData++;

  analogRead(sensorPin); // throw this one away so that we get a good reading on the next one
  int reading = analogRead(sensorPin);
  char radiopacket[14] = "sensor:      ";
  itoa(reading, radiopacket+7, 10);
  Serial.print("Sending "); Serial.println(radiopacket);
  radiopacket[13] = 0;

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

  rf95.sleep();
  digitalWrite(LED, LOW);
  delay(1000);
}

