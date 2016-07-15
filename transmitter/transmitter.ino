#include <RH_RF95.h>
#include <Adafruit_SleepyDog.h>

#ifdef ARDUINO_ARCH_SAMD
#endif

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Radio Config
#ifdef ARDUINO_ARCH_SAMD
  #include <Adafruit_GPS.h>

  // for feather m0
  #define RFM95_CS 8
  #define RFM95_RST 4
  #define RFM95_INT 3

  // GPS setup
  Adafruit_GPS GPS(&Serial1);
  float lastLatitudeDegrees, lastLongitudeDegrees;
#else
  // for feather32u4
  #define RFM95_CS 8
  #define RFM95_RST 4
  #define RFM95_INT 7
#endif

// !!! Change this value for each Feather that you have !!!
const char featherId = '1';

#define RF95_FREQ 915.0

RH_RF95 rf95(RFM95_CS, RFM95_INT);

// Blinky on receipt
#define LED 13

// Oled Config
// buttons a = 9, b = 6, c = 5
const int sendDataButtonPin = 9;
int sendDataLastButtonState = 0;

const int gpsButtonPin = 6;
int gpsLastButtonState = 0;

const int sleepButtonPin = 5;
int sleepLastButtonState = 0;

int nbrOfSentData = 0;
bool deepSleep = false;

Adafruit_SSD1306 oled = Adafruit_SSD1306();

// Sensor config
const int sensorPowerPin = A0;
const int sensorPins[] = {A1, A2, A3};

void setup() {
//  while ( ! Serial ) { delay( 10 ); } // wait for serial connection
  Serial.begin(115200);

  // radio setup
  pinMode(LED, OUTPUT);
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init()) {
    Serial.println(F("LoRa radio init failed"));
    while (1);
  }
  Serial.println(F("LoRa radio init OK!"));

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println(F("setFrequency failed"));
    while (1);
  }
  Serial.print(F("Set Freq to: ")); Serial.println(RF95_FREQ);

  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);

#ifdef ARDUINO_ARCH_SAMD
  // GPS setup
  // 9600 NMEA is the default baud rate for Adafruit MTK GPS's- some use 4800
  GPS.begin(9600);
  // uncomment this line to turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  // uncomment this line to turn on only the "minimum recommended" data
  //GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  // For parsing data, we don't suggest using anything but either RMC only or RMC+GGA since
  // the parser doesn't care about other sentences at this time
  // Set the update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate
  // For the parsing code to work nicely and have time to sort thru the data, and
  // print it out we don't suggest using anything higher than 1 Hz

  // Request updates on antenna status, comment out to keep quiet
  GPS.sendCommand(PGCMD_ANTENNA);

  // Ask for firmware version
  delay(1000);
  Serial1.println(PMTK_Q_RELEASE);
#endif

  // oled setup
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oled.setTextSize(1);
  oled.setTextColor(WHITE);
  oled.clearDisplay();
  oled.setCursor(0,0);
  oled.println(F("A - send sensor data"));
#ifdef ARDUINO_ARCH_SAMD
  oled.println(F("B - send GPS (TODO)"));
#endif
  oled.println(F("C - deep sleep"));
  oled.display();

  delay(2000);

  pinMode(sensorPowerPin, OUTPUT);
  digitalWrite(sensorPowerPin, LOW);
  pinMode(sendDataButtonPin, INPUT_PULLUP);
  pinMode(gpsButtonPin, INPUT_PULLUP);
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

#ifdef ARDUINO_ARCH_SAMD
  // read gps and check to see if the gps should be sent
  if (!deepSleep) {
    char c = GPS.read();
    if (GPS.newNMEAreceived()) {
      Serial.println(GPS.lastNMEA());
      if (GPS.parse(GPS.lastNMEA())) {
        Serial.println(GPS.milliseconds);
        if (GPS.fix) {
          lastLatitudeDegrees = GPS.latitudeDegrees;
          lastLongitudeDegrees = GPS.longitudeDegrees;
          Serial.print(F("last lat ")); Serial.print(lastLatitudeDegrees); Serial.print(F(" lon ")); Serial.println(lastLongitudeDegrees);
        }
      }
    }
  }
  int gpsButtonState = digitalRead(gpsButtonPin);
  if (gpsButtonState != gpsLastButtonState) {
    if (gpsButtonState == LOW) {
      // if the current state is LOW then the button was pressed
      Serial.print(F("send gps lat ")); Serial.print(lastLatitudeDegrees); Serial.print(F(" lon ")); Serial.println(lastLongitudeDegrees);
    }
  }

  gpsLastButtonState = gpsButtonState;
#endif

  // check to see if the device should be put into deep sleep
  int sleepButtonState = digitalRead(sleepButtonPin);
  if (sleepButtonState != sleepLastButtonState) {
    if (sleepButtonState == LOW) {
      // if the current state is LOW then the button was pressed
      deepSleep = !deepSleep;
      Serial.print(F("sleep ")); Serial.println(deepSleep);
    }
  }

  oled.clearDisplay();
  oled.setCursor(0,0);
  oled.print(F("nbr sent: ")); oled.println(nbrOfSentData);
  oled.print(F("deep sleep: ")); oled.println(deepSleep);
  oled.print(F("Feather Id: ")); oled.println(featherId);
#ifdef ARDUINO_ARCH_SAMD
  oled.print(F("lat: ")); oled.print(lastLatitudeDegrees); oled.print(F(" lon: ")); oled.println(lastLongitudeDegrees);
#endif
  oled.display();

  sendDataLastButtonState = sendDataButtonState;
  sleepLastButtonState = sleepButtonState;

  if (deepSleep) {
    Watchdog.sleep(1000);
  }
}

void readAndSendSensorData() {
  // turn on the power to the sensors
  digitalWrite(sensorPowerPin, HIGH);
  delay(100); // warm them up

  // collect and send the data for each one of the sensors
  for (int i = 0; i < sizeof(sensorPins) / sizeof(int); i++) {
    readAndSendSensorData(i);
  }

  // turn the power off to the sensors to not wear them out
  digitalWrite(sensorPowerPin, LOW);
}

void readAndSendSensorData(int sensorNbr) {
  nbrOfSentData++;

  analogRead(sensorPins[sensorNbr]); // throw this one away so that we get a good reading on the next one
  int reading = analogRead(sensorPins[sensorNbr]);
  char radiopacket[16] = "FI: ,SN: ,     ";
  radiopacket[3] = featherId;
  itoa(sensorNbr + 1, radiopacket + 8, 10);
  radiopacket[9] = ',';
  itoa(reading, radiopacket + 10, 10);
  Serial.print(F("Sending ")); Serial.println(radiopacket);
  radiopacket[15] = 0;

  oled.clearDisplay();
  oled.setCursor(0,0);
  oled.print(F("Send ")); oled.println(radiopacket);
  oled.display();

  delay(10);
  rf95.send((uint8_t *)radiopacket, 20);

  oled.println(F("Waiting for response"));
  oled.display();

  Serial.println(F("Waiting to complete")); delay(10);
  rf95.waitPacketSent();

  // Now wait for a reply
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  Serial.println(F("Waiting for reply")); delay(10);
  if (rf95.waitAvailableTimeout(1000)) {
    // Should be a reply message for us now
    if (rf95.recv(buf, &len)) {
      digitalWrite(LED, HIGH);
      Serial.print(F("Got reply: ")); Serial.println((char*)buf);
      Serial.print(F("RSSI: ")); Serial.println(rf95.lastRssi(), DEC);
      oled.print(F("Reply: ")); oled.println((char*)buf);
      oled.print(F("RSSI: ")); oled.println(rf95.lastRssi(), DEC);
      oled.display();
    } else {
      Serial.println(F("Receive failed"));
      oled.println(F("Receive failed"));
      oled.display();
    }
  } else {
    Serial.println(F("No reply"));
    oled.println(F("No reply"));
    oled.display();
  }

  rf95.sleep();
  digitalWrite(LED, LOW);
  delay(1000);
}

