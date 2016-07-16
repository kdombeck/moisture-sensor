#include <RH_RF95.h>
#include <Adafruit_SleepyDog.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "secrets.h"

// Radio Config
#ifdef ARDUINO_ARCH_SAMD
  #include <Adafruit_GPS.h>

  // for feather m0
  #define RFM95_CS 8
  #define RFM95_RST 4
  #define RFM95_INT 3

  // GPS setup
  Adafruit_GPS GPS(&Serial1);
#else
  // for feather32u4
  #define RFM95_CS 8
  #define RFM95_RST 4
  #define RFM95_INT 7
#endif

#define RF95_FREQ 915.0

RH_RF95 rf95(RFM95_CS, RFM95_INT);

// Blinky on receipt
#define LED 13

#define VOLTAGE_BATTERY_PIN A7

// Oled Config
// buttons a = 9, b = 6, c = 5
#define SEND_DATA_BUTTON_PIN 9
int sendDataLastButtonState = 0;

#define GPS_BUTTON_PIN 6
int gpsLastButtonState = 0;

#define SLEEP_BUTTON_PIN 5
int sleepLastButtonState = 0;

int nbrOfSentData = 0;
bool deepSleep = false;

Adafruit_SSD1306 oled = Adafruit_SSD1306();

uint32_t oledRefreshTimer = millis();

// Sensor config
#define SENSOR_POWER_PIN A0
const int SENSOR_PINS[] = {A1, A2, A3};

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
  oled.println(F("B - send GPS"));
#endif
  oled.println(F("C - deep sleep"));
  oled.display();

  delay(2000);

  pinMode(SENSOR_POWER_PIN, OUTPUT);
  digitalWrite(SENSOR_POWER_PIN, LOW);
  pinMode(SEND_DATA_BUTTON_PIN, INPUT_PULLUP);
  pinMode(GPS_BUTTON_PIN, INPUT_PULLUP);
  pinMode(SLEEP_BUTTON_PIN, INPUT_PULLUP);
}

void loop() {
  // check to see if sensor data should be sent
  int sendDataButtonState = digitalRead(SEND_DATA_BUTTON_PIN);
  if (sendDataButtonState != sendDataLastButtonState) {
    if (sendDataButtonState == LOW) {
      // if the current state is LOW then the button was pressed
      readAndSendSensorData();
      readAndSendBatteryData();
    }
  }

#ifdef ARDUINO_ARCH_SAMD
  // read gps and check to see if the gps should be sent
  if (!deepSleep) {
    char c = GPS.read();
    if (GPS.newNMEAreceived()) {
      //Serial.println(GPS.lastNMEA());
      if (GPS.parse(GPS.lastNMEA())) {
        Serial.print(F("Fix: ")); Serial.print((int)GPS.fix); Serial.print(F(" quality: ")); Serial.println((int)GPS.fixquality);
        if (GPS.fix) {
          Serial.print(F("lat ")); Serial.print(GPS.latitudeDegrees, 5); Serial.print(F(" lon ")); Serial.print(GPS.longitudeDegrees, 5);
          Serial.print(F(" Satellites: ")); Serial.println((int)GPS.satellites);
        }
      }
    }
  }
  int gpsButtonState = digitalRead(GPS_BUTTON_PIN);
  if (gpsButtonState != gpsLastButtonState) {
    if (gpsButtonState == LOW) {
      // if the current state is LOW then the button was pressed
      sendGpsData();
    }
  }

  gpsLastButtonState = gpsButtonState;
#endif

  // check to see if the device should be put into deep sleep
  int sleepButtonState = digitalRead(SLEEP_BUTTON_PIN);
  if (sleepButtonState != sleepLastButtonState) {
    if (sleepButtonState == LOW) {
      // if the current state is LOW then the button was pressed
      deepSleep = !deepSleep;
      Serial.print(F("sleep ")); Serial.println(deepSleep);
    }
  }

  // if millis() or timer wraps around, we'll just reset it
  if (oledRefreshTimer > millis()) oledRefreshTimer = millis();

  // print out the current stats only so often
  // if you print out the stats every time it will not allow the GPS enough time to read the next reading
  if (millis() - oledRefreshTimer > 500) {
    oledRefreshTimer = millis(); // reset the timer
    oled.clearDisplay();
    oled.setCursor(0,0);
    oled.print(F("nbr sent ")); oled.print(nbrOfSentData); oled.print(F(" dsleep ")); oled.println(deepSleep);
    oled.print(F("feather id: ")); oled.println(FEATHER_ID);
#ifdef ARDUINO_ARCH_SAMD
    oled.print(F("lt")); oled.print(GPS.latitudeDegrees, 4); oled.print(F(" ln")); oled.println(GPS.longitudeDegrees, 4);
    oled.print(F("fix ")); oled.print(GPS.fix); oled.print(F(" qual ")); oled.print(GPS.fixquality); oled.print(F(" sats ")); oled.println(GPS.satellites);
#endif
    oled.display();
  }

  sendDataLastButtonState = sendDataButtonState;
  sleepLastButtonState = sleepButtonState;

  if (deepSleep) {
    Watchdog.sleep(1000);
  }
}

void readAndSendSensorData() {
  // turn on the power to the sensors
  digitalWrite(SENSOR_POWER_PIN, HIGH);
  delay(100); // warm them up

  // collect and send the data for each one of the sensors
  for (int sensorNbr = 0; sensorNbr < sizeof(SENSOR_PINS) / sizeof(int); sensorNbr++) {
    analogRead(SENSOR_PINS[sensorNbr]); // throw this one away so that we get a good reading on the next one
    int reading = analogRead(SENSOR_PINS[sensorNbr]);
    char data[16] = "FI- -SN- ,     ";
    data[3] = FEATHER_ID;
    itoa(sensorNbr + 1, data + 8, 10);
    data[9] = ',';
    itoa(reading, data + 10, 10);
    data[15] = 0;
    sendData(data, 16);
  }

  // turn the power off to the sensors to not wear them out
  digitalWrite(SENSOR_POWER_PIN, LOW);
}

void readAndSendBatteryData() {
//  delay(500); // if a button was pressed time is needed to make sure they let go
  float measuredvbat = analogRead(VOLTAGE_BATTERY_PIN);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage
  Serial.print(F("VBat: ")); Serial.println(measuredvbat);

  String data = String(F("FI-"));
  data.concat(FEATHER_ID);
  data.concat(F("-BAT,"));
  data.concat(measuredvbat);

  char dataBuf[data.length() + 1];
  data.toCharArray(dataBuf, data.length() + 1);

  sendData(dataBuf, data.length() + 1);

  // reset the send data pin back to digital so the A button works
  pinMode(SEND_DATA_BUTTON_PIN, INPUT_PULLUP);
}

#ifdef ARDUINO_ARCH_SAMD
void sendGpsData() {
  char outstr[10];

  String data = String(F("gps/csv,FI-"));
  data.concat(FEATHER_ID);
  data.concat(F(','));
  sprintf(outstr, "%f", GPS.latitudeDegrees);
  data.concat(outstr);
  data.concat(F(','));
  sprintf(outstr, "%f", GPS.longitudeDegrees);
  data.concat(outstr);
  data.concat(F(','));
  sprintf(outstr, "%f", GPS.altitude);
  data.concat(outstr);

  char dataBuf[data.length() + 1];
  data.toCharArray(dataBuf, data.length() + 1);

  sendData(dataBuf, data.length() + 1);
}
#endif

void sendData(char* data, int dataLength) {
  nbrOfSentData++;
  Serial.print(F("Sending ")); Serial.println(data);

  oled.clearDisplay();
  oled.setCursor(0,0);
  oled.print(F("Send ")); oled.println(data);
  oled.display();

  delay(10);
  rf95.send((uint8_t *)data, dataLength);

  oled.println(F("Waiting for response"));
  oled.display();

  Serial.println(F("Waiting to complete"));
  rf95.waitPacketSent();

  // Now wait for a reply
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  Serial.println(F("Waiting for reply"));
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

