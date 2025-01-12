#include <Wire.h>
#include <RTClib.h>
#include <SPI.h>
#include <SD.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ph4502c_sensor.h>
#include <Adafruit_VEML6075.h>
#include <Adafruit_MPL3115A2.h>

RTC_DS1307 rtc;

// RTC config
const int rtcSDA = 20;
const int rtcSCL = 21;
const int delayInMinutes = 1;  // We desided to take a reading every 10 minutes, but if you want to change it this is the place to do it!

// SD card config
const int sdCard = 53;                  // This is the pin for the SD card reader
const char* dataFileName = "data.dat";  // Name of the file you want to save to.

/* Analog pin for analog temperature sensor (Couldn't get it to work)
const int termistorPin = A0;
*/

// Digital temperature sensor config (DS18B20)
const int tempDigitalPin = 22;        // Digital pin where the temperature sensor is connected
OneWire oneWire(tempDigitalPin);      // Setup a oneWire instance to communicate with any OneWire devices
DallasTemperature sensors(&oneWire);  // Pass our oneWire reference to Dallas Temperature sensor

// PH sensor config
const uint8_t PO = A0;
const uint8_t TO = A1;
PH4502C_Sensor ph4502c(PO, TO);

// UV sensor
Adafruit_VEML6075 uv = Adafruit_VEML6075();

// Pressure sensor
Adafruit_MPL3115A2 baro;

// Gas sensor A0 pin
// Gas sensor R0 avg value
// float R0_avg = 0;

File dataFile;

void setup() {
  Serial.begin(9600);

  // Initializes sd card
  if (!SD.begin(sdCard)) {
    Serial.println("initialization failed!");
    while (1)
      ;
  }
  if (SD.exists(dataFileName)) {
    if (!SD.remove(dataFileName)) {
      Serial.println("Error deleting the file.");
      return;
    }
  }
  Serial.println("initialization done.");

  // Initializing UV sensor
  if (! uv.begin()) {
    Serial.println("Failed to communicate with VEML6075 sensor, check wiring?");
    while (1) { delay(100); }
  }

  // Initializing barometric sensor
  if (!baro.begin()) {
    Serial.println("Could not find sensor. Check wiring.");
    while(1);
  }
  baro.setSeaPressure(1013.26);

  // Initializes the rtc module
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1)
      ;
  }

  DateTime now = rtc.now();
  if (now.year() < 2000) {
    Serial.println("RTC lost power, let's set the time!");
    // Following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  // This is the line that sets the time to the time the code was compiled, if you want to change it you can do it here!
  }

  // Initializes the digital temperature sensor
  sensors.begin();

  // Initialize the PH sensor
  ph4502c.init();

  // Prints the header to the sd card
  header();

  // Testing the gas sensor R0 value
  //for (int i = 0; i < 25; i++)
  //{
  //  R0_avg += gas_sensor_R0();
  //  delay(1000);
  //}
  //R0_avg /= 25;
}

void loop() {
  delay(1000);
  time();
  // Your timer logic can be implemented here
}

void time() {
  DateTime now = rtc.now();

  int startTime = now.minute();
  while ((startTime + delayInMinutes) % 60 == now.minute()) {
    now = rtc.now();  // Update the current time
    Serial.println("Waiting...");

    temp();

    delay(1000);
  }

  // Keeps the time and date in separate strings
  String date = String(now.year()) + "/" + String(now.month()) + "/" + String(now.day());
  String time = String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second());
  // Uses the time and date as arguments in the logdata function
  logdata(date, time);
}

void logdata(String date, String time) {
  Serial.println("Logging data...");
  // TODO: use the diffrent sensors function to log data
  // Gets data from temperature sensor and pH sensor
  float temperature = temp();
  float pHValue = pH();
  float uvValue = UV();
  float pressureValue = pressure();
  float gas_value = gas_sensor(R0_avg);
  Serial.print("Temperature: ");
  Serial.println(temperature);
  Serial.print("pH Value read: ");
  Serial.println(pHValue);
  Serial.print("UV Index Reading: ");
  Serial.println(uvValue);
  Serial.print("Pressure: ");
  Serial.println(pressureValue);
  Serial.print("Gas concentration: ");
  Serial.println(gas_value);
  Serial.print("Time: ");
  Serial.println(time);
  writeToSDCard(date + " " + time + "," + temperature + "," + pHValue + "\n");  // Can add Humidity, Soil Moisture, Light Intensity, Water Level
}

bool writeToSDCard(String data) {
  dataFile = SD.open(dataFileName, FILE_WRITE);
  // Check if the file opened successfully
  if (dataFile) {
    // Write data to the file
    dataFile.print(data);
    Serial.println("Data written to the file.");
    dataFile.flush();  // Flush the data to the SD card
    // Close the file
    dataFile.close();
    return true;
  } else {
    Serial.println("Error opening the file.");
    Serial.print("Filename: ");
    Serial.println(dataFileName);
    return false;
  }
}
// This prints the header to the SD card
void header() {
  int attempts = 0;
  while (!writeToSDCard("Date, Time, Temperature, Humidity, Soil Moisture, Light Intensity, Water Level\n")) {
    attempts++;
    // If the function could not print to the SD card after 10 tries, then it gives up
    if (attempts > 10) {
      Serial.println("Could not print the header to the SD card");
      return;  // Exits the function
    }
    delay(500);  // Adds a delay of 0.5 sec before retrying
  }
  Serial.println("Header printed successfully");
}

float temp() {
  // Failed analog temperature sensor
  /*
  // Microvolts input from the termistor
  int resistance = analogRead(termistorPin);
  // Conversion to Celsius
  float temperature = (-0.1637931 * resistance) + 169.67;
  Serial.println(resistance);
  Serial.println(temperature);
  */

  // Digital temp sensor
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);

  return tempC;
}

float pH() {
  /* Test
  float reading = 0;
  for(int i = 0; i < 10; i++) {
    reading += analogRead(PO);
    Serial.println("Raw reading: " + reading); 
    delayMicroseconds(100);
  }
  reading = 5.0 / 1024.0 * reading;
  Serial.print("1. reading edit: ");
  Serial.println(reading);
  reading /= 10;
  Serial.println("2. reading edit: " + reading);
  reading = 14.8 + ((2.5 - reading)) / 0.18;
  Serial.println("3. reading edit: " + reading);
  
  // Test
  float temp = ph4502c.read_temp();
  Serial.println("pH temp: " + temp); 
  End test */

  float pH = ph4502c.read_ph_level();
  return pH;
}

float UV() {
  return uv.readUVI();
}

float pressure() {
  return baro.getPressure();
}

float gas_sensor_R0() {
  float sensor_volt;
  float RS_air;
  float R0;
  float sensorValue = 0;

  for (int i = 0; i < 100; i++)
  {
    sensorValue = sensorValue + analogRead(A0);
  }
  sensorValue = sensorValue/100.0;

  sensor_volt = sensorValue/1024*5.0;
  RS_air = (5.0-sensor_volt)/sensor_volt;
  R0 = RS_air/6.5;

  Serial.print("sensor_volt = ");
  Serial.print(sensor_volt);
  Serial.println("V");

  Serial.print("R0 = ");
  Serial.println(R0);

  return R0;
}

float gas_sensor(float R0) {
  float sensor_volt;
  float RS_gas;
  float ratio;
  int sensorValue = analogRead(A0);
  sensor_volt = (float)sensorValue/1024*5.0;
  RS_gas = (5.0-sensor_volt)/sensor_volt;
  
  ratio = RS_gas/R0;

  Serial.print("sensor_volt = ");
  Serial.println(sensor_volt);
  Serial.print("RS_gas = ");
  Serial.println(RS_gas);
  Serial.print("ratio = ");
  Serial.println(ratio);

  return ratio;
}