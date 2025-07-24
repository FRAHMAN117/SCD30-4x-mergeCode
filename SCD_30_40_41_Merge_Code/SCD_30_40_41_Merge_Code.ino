#include <Adafruit_SCD30.h>
#include <Wire.h>
Adafruit_SCD30 scd30;              //SCD30
const int16_t SCD_ADDRESS = 0x62;  //SCD41
int selection = -1;

void scd30Setup();
void scd30Loop();
void scd4xSetup();
void scd4xLoop();


void setup() {
  Serial.begin(115200);
  delay(5000);
  Serial.println(__FILE__);

  Wire.begin();

  while (!Serial) delay(10);  // will pause Zero, Leonardo, etc until serial console opens
                              //MAKES SENSOR SELECTION
  while (selection != 0 && selection != 1) {
    Serial.println("Choose which sensor to test\n For Sensor SCD 30- type 1\n For Sensor SCD40/41- type 0");
    while (!Serial.available()) {
      delay(100);
    }
    selection = Serial.parseInt();
  }

  //SCD 30 Setup Code
  if (selection == 1) {
    scd30Setup();
  }

  else if (selection == 0) {
    scd4xSetup();  
  }
}

void loop() {
  if (selection == 1) {
    scd30Loop();
  }

  else if (selection == 0) {
    scd4xLoop();
  }

}
