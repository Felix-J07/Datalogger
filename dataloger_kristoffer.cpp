#include <Wire.h>
#include <RTClib.h> // RTC library
#include <SPI.h> // SPI library
#include <SD.h> // SD card library
#include <ph4502c> // PH sensor library
#include <OneWire.h> // OneWire library
#include <DallasTemperature.h> // DallasTemperature library

// Some wired config stuff
RTC_DS1307 rtc;

// RTC config
const int rtcSDA = 20;
const int rtcSCL = 21;
const int delayInMinutes = 1; // We desided to take a reading every 10 minutes, but if you want to change it this is the place to do it!

// SD card config
const int sdCard = 53; // This is the pin for the SD card reader
const char* dataFileName = "data.txt"; // Name of the file you want to save to.
File dataFile;

// Sensor data config
// Digital temperature sensor config
const int tempDigitalPin = 22; // Digital pin where the temperature sensor is connected
OneWire oneWire(tempDigitalPin); // Setup a oneWire instance to communicate with any OneWire devices
DallasTemperature sensors(&oneWire); // Pass our oneWire reference to Dallas Temperature sensor

// PH sensor config(PO, TO)
PH4502C_Sensor ph4502c(A0, A1);

void setup() {
  // Initializes the digital temperature sensor
  sensors.begin();

  // Initialize the PH sensor
  ph4502c.init();

  Serial.begin(9600);
  if (!SD.begin(sdCard)) {
    Serial.println("initialization failed!");
    while (1);
  }
  if (SD.exists(dataFileName)) { // This code deletes the file it it alreddy exists, so when the device is turend off and on again it will start a new file. (We don't recomend useing it. )
    if (!SD.remove(dataFileName)) {
      Serial.println("Error deleting the file.");
      return;
    } 
  }
  Serial.println("initialization done.");
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  DateTime now = rtc.now();
  if (now.year() < 2000) {
    Serial.println("RTC lost power, let's set the time!");
    // Following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // This is the line that sets the time to the time the code was compiled, if you want to change it you can do it here!
  }
  header();
}

void loop() {
  delay(1);
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
  logdata();
  return true;
}

void logdata() {
  Serial.println("Logging data...");
  // TODO: use the diffrent sensors function to log data 
  // TOD: Make a header function that writes the header to the file
  //writeToSDCard("Time, Temperature, Humidity, Soil Moisture, Light Intensity, Water Level");
  temperatureSensor();
}

void temperatureSensor() {
    // Digital temp sensor
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);
  return tempC;
}

void writeToSDCard(char* data) { // 
  dataFile = SD.open(dataFileName, FILE_WRITE);
  // Check if the file opened successfully
  if (dataFile) {
    // Write data to the file
    dataFile.print(data);
    Serial.println("Data written to the file.");
    dataFile.flush(); // Flush the data to the SD card
    // Close the file
    dataFile.close();
  } else {
    Serial.println("Error opening the file.");
    Serial.print("Filename: ");
    Serial.println(dataFileName);
  }
}

void header() {
  writeToSDCard("Time, Temperature, Humidity, Soil Moisture, Light Intensity, Water Level");
}

void phSensor() {
  float pHValue = ph4502c.read_ph_level();
  return pHValue;
}