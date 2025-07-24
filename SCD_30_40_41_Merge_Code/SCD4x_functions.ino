// calculate CRC according to datasheet section 5.17
uint8_t CalcCrc(uint8_t data[2]) {
  uint8_t crc = 0xFF;
  for (int i = 0; i < 2; i++) {
    crc ^= data[i];
    for (uint8_t bit = 8; bit > 0; --bit) {
      if (crc & 0x80) {
        crc = (crc << 1) ^ 0x31u;
      } else {
        crc = (crc << 1);
      }
    }
  }
  return crc;
}


void scd4xSetup() {
  float co2, temperature, humidity;
  //uint16_t calibration;
  uint8_t data[12], counter, repetition;
  uint8_t ret;


  // wait until sensors are ready, > 1000 ms according to datasheet
  delay(1000);

  // start scd measurement in periodic mode, will update every 5 s
  Wire.beginTransmission(SCD_ADDRESS);
  Wire.write(0x21);
  Wire.write(0xb1);
  Wire.endTransmission();
  /*
    if (chip != 0) {
      Serial.println("SCD 40/41 Chip not found");
      while (1) { delay(10); }
    }
    */

  // wait for 5 minutes to equilibrate sensor to ambient
  Serial.println("you selected SCD40/41 for testing");
  Serial.println("SCD40/41 CALIBRATION TESTING");
  delay(2000);
  int speed = -1;
  int calibration = -1;

  while (Serial.available()) {
    Serial.read();
  }
  while (speed < 0) {
    Serial.println("Select speed for equilibration in minutes (5 mins recommended)");
    while (!Serial.available()) {
      delay(100);
    }
    speed = Serial.parseInt();
  }
  Serial.print("# Waiting ");
  Serial.print(speed);
  Serial.println(" minutes for equilibration");

  delay((unsigned long)speed * 60 * 1000);

  Serial.println("# CO2 values before calibration");

  // measure 5 times
  for (repetition = 0; repetition < 5; repetition++) {
    // read measurement data: 2 bytes co2, 1 byte CRC,
    // 2 bytes T, 1 byte CRC, 2 bytes RH, 1 byte CRC,
    // 2 bytes sensor status, 1 byte CRC
    // stop reading after 12 bytes (not used)
    // other data like  ASC not included
    Wire.requestFrom(SCD_ADDRESS, 12);
    counter = 0;
    while (Wire.available()) {
      data[counter++] = Wire.read();
    }

    // floating point conversion according to datasheet
    co2 = (float)((uint16_t)data[0] << 8 | data[1]);
    // convert T in degC
    temperature = -45 + 175 * (float)((uint16_t)data[3] << 8 | data[4]) / 65536;
    // convert RH in %
    humidity = 100 * (float)((uint16_t)data[6] << 8 | data[7]) / 65536;

    Serial.print("# ");
    Serial.print(co2);
    Serial.print("\t");
    Serial.print(temperature);
    Serial.print("\t");
    Serial.print(humidity);
    Serial.println();

    delay(2000);
  }


  // stop scd measurement
  Wire.beginTransmission(SCD_ADDRESS);
  Wire.write(0x3f);
  Wire.write(0x86);
  ret = Wire.endTransmission();

  if (ret == 0) {
    Serial.println(" I2C transmission successfull");
  } else {
    Serial.print("error in transmission: ");
    Serial.println(ret);
  }


  // wait for sensor
  delay(20);

  // assuming an external reference shows 650 ppm
  // calculate CRC according to datasheet section 5.17

  //MODIFY THIS VALUE FOR CALIBRATION
  //calibration = 610;
  while (Serial.available()) {
    Serial.read();
  }
  while (calibration < 420) {
    Serial.println("Enter the value for CO2 in chamber from LI-850");
    delay(100);
    while (!Serial.available()) {
      delay(100);
    }
    calibration = Serial.parseInt();
  }
  Serial.print("# Calibrating with reference value [ppm]: ");
  Serial.println(calibration);

  // prepare buffer with data for calibration
  // calculate CRC for each 2 bytes of data
  data[0] = (calibration & 0xff00) >> 8;
  data[1] = calibration & 0x00ff;
  data[2] = CalcCrc(data);

  // send command for perform_forced_recalibration
  Wire.beginTransmission(SCD_ADDRESS);
  Wire.write(0x36);
  Wire.write(0x2F);
  // append data for calibration
  // 2 bytes calibraion, CRC
  Wire.write(data[0]);
  Wire.write(data[1]);
  Wire.write(data[2]);
  ret = Wire.endTransmission();
  Serial.println(ret);

  delay(400);

  // read data: 2 bytes correction, 1 byte CRC
  Wire.requestFrom(SCD_ADDRESS, 3);
  counter = 0;
  while (Wire.available()) {
    data[counter++] = Wire.read();
  }

  if (CalcCrc(data) != data[2]) {
    Serial.println("# ERROR: recalibration CRC return value");
  }
  calibration = ((uint16_t)data[0] << 8 | data[1]);

  Serial.print("# Value after recalibration\n# ");
  Serial.println(calibration - 32768);

  // output format
  Serial.println("CO2(ppm)\tTemperature(degC)\tRelativeHumidity(percent)");

  // start scd measurement again in periodic mode, will update every 2 s
  Wire.beginTransmission(SCD_ADDRESS);
  Wire.write(0x21);
  Wire.write(0xb1);
  Wire.endTransmission();

  // wait for first measurement to be finished (> 5 s)
  delay(10000);
}



void scd4xLoop() {
  float co2, temperature, humidity;
  uint8_t data[12], counter;

  // send read data command
  Wire.beginTransmission(SCD_ADDRESS);
  Wire.write(0xec);
  Wire.write(0x05);
  Wire.endTransmission();

  // read measurement data: 2 bytes co2, 1 byte CRC,
  // 2 bytes T, 1 byte CRC, 2 bytes RH, 1 byte CRC,
  // 2 bytes sensor status, 1 byte CRC
  // stop reading after 12 bytes (not used)
  // other data like  ASC not included
  Wire.requestFrom(SCD_ADDRESS, 12);
  counter = 0;
  while (Wire.available()) {
    data[counter++] = Wire.read();
  }

  // floating point conversion according to datasheet
  co2 = (float)((uint16_t)data[0] << 8 | data[1]);
  // convert T in degC
  temperature = -45 + 175 * (float)((uint16_t)data[3] << 8 | data[4]) / 65536;
  // convert RH in %
  humidity = 100 * (float)((uint16_t)data[6] << 8 | data[7]) / 65536;

  Serial.print(co2);
  Serial.print("\t");
  Serial.print(temperature);
  Serial.print("\t");
  Serial.print(humidity);
  Serial.println();

  // wait 5 s for next measurement
  delay(5000);
}
