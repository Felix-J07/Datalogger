#include <Wire.h>
#include <SPI.h>
#include <RTClib.h>
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
const int sdCard = 53;                  // This is the pin for the SD card reader (CS)
const char* dataFileName = "data.csv";  // Name of the file you want to save to.
File dataFile;

/* Analog pin for analog temperature sensor (Couldn't get it to work)
const int termistorPin = A0;
*/

// Digital temperature sensor config (DS18B20)
const int tempDigitalPin = 22;        // Data pin where the temperature sensor is connected
OneWire oneWire(tempDigitalPin);      // Setup a oneWire instance to communicate with any OneWire devices
DallasTemperature tempSensors(&oneWire);  // Pass our oneWire reference to Dallas Temperature sensor

// PH sensor config
const uint8_t PO = A0;
const uint8_t TO = A1;
PH4502C_Sensor ph4502c(PO, TO);

// UV sensor
Adafruit_VEML6075 uv = Adafruit_VEML6075();

// Pressure sensor
Adafruit_MPL3115A2 baro;

// Gas sensor A2 pin
const uint8_t gasSensorPin = A2;

void setup() {
  Serial.begin(9600);

  if (!SD.begin(sdCard)) {
    Serial.println("initialization failed!");
    while (1)
      ;
  }
  if (!SD.exists(dataFileName)) {
    // Prints the header to the sd card
    header();
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
  tempSensors.begin();

  // Initialize the PH sensor
  ph4502c.init();
}

void loop() {
  time();
  // Your timer logic can be implemented here
}

void time() {
  DateTime now = rtc.now();

  int startTime = now.minute();
  while ((startTime + delayInMinutes) % 60 != now.minute()) {
    now = rtc.now();  // Update the current time
    Serial.println("Waiting...");
    delay(1000);
  }

  // Keeps the time and date in separate strings
  String date = String(now.year()) + "/" + now.month() + "/" + now.day();
  String time = String(now.hour()) + ":" + now.minute() + ":" + now.second();
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
  Serial.print("Temperature: ");
  Serial.println(temperature);
  Serial.print("pH Value read: ");
  Serial.println(pHValue);
  Serial.print("UV Index Reading: ");
  Serial.println(uvValue);
  Serial.print("Pressure: ");
  Serial.println(pressureValue);
  Serial.print("Time: ");
  Serial.println(time);
  writeToSDCard(date + ";" + time + ";" + temperature + ";" + pHValue + ";" + uvValue + ";" + pressureValue + "\n");  // Can add Humidity, Soil Moisture, Light Intensity, Water Level
}

bool writeToSDCard(String data) {
  dataFile = SD.open(dataFileName, FILE_WRITE);
  Serial.println(data);
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
  while (!writeToSDCard("Date;Time;Temperature;pH Value;UV Index;Pressure (hPa)\n")) {
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
  tempSensors.requestTemperatures();
  float tempC = tempSensors.getTempCByIndex(0);

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

float Gas() {

}