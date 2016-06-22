#include <Adafruit_FeatherOLED_WiFi.h>

const int aButtonPin = 9; // a = 9, b = 6, c = 5
int aButtonPushCounter = 0;
int aButtonState = 0;
int aLastButtonState = 0;

Adafruit_FeatherOLED_WiFi  oled = Adafruit_FeatherOLED_WiFi();

void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println("in setup");
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oled.init();
  oled.clearDisplay();

  oled.clearMsgArea();
  oled.println("setting up");
  oled.display();
  Serial.println("set up oled");

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
      Serial.print("number of a button pushes:  ");
      Serial.println(aButtonPushCounter);
    }
  }

  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(WHITE);
  oled.setCursor(0,0);
  oled.print("A pushed ");
  oled.println(aButtonPushCounter);
  oled.display();

  aLastButtonState = aButtonState;
}
