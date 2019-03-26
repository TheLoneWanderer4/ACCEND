#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_MPL3115A2.h>

//Helpful numbers
#define BMEA_ADDR 0x76
#define BMEB_ADDR 0x77
#define YELLOW_LED 10
#define SEALEVELPRESSURE_HPA (1013.8)
#define VBATPIN A7
#define GASPIN_A A0 // powered by solar panels
#define GASPIN_B A2 // bat powered

//set Delay Between Data Points
int CollectDelay = 1000;


//I2C Sensor
Adafruit_BME280 bmeA; // first sensor
Adafruit_BME280 bmeB; // second sensor
Adafruit_MPL3115A2 baro = Adafruit_MPL3115A2();

uint32_t timer;
char Filename[] = "19S000.csv";
int first0Index = 3;
int second0Index = 4;
int third0Index = 5;

void setup() {
  Serial.begin(9600);
  timer = millis();
  pinMode(YELLOW_LED, OUTPUT);

  if (!bmeA.begin(BMEA_ADDR)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }

  if (!bmeB.begin(BMEB_ADDR)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }

  if (!baro.begin()) {
    Serial.println("Could not find a valid MPL3115A2 sensor, check wiring!");
    while (1);
  }

  //Init the SD card and format the columns
  if (!SD.begin(4))
  {
    Serial.println("SD card initialization failed!");
    return;
  }
  Serial.println("SD card initialization done.");

  for (int i = 0; i < 1000; i++)
  {
    Filename[first0Index] = (i / 100) + '0';
    Filename[second0Index] = (i % 100) / 10 + '0';
    Filename[third0Index] = i % 10 + '0';
    if (!SD.exists(Filename))
    {
      File dataFile = SD.open(Filename, FILE_WRITE);

      //NOTE: SD formatting in the .csv file requires the headers have no whitespace
      dataFile.println(
        "Temp0(C), Pressure0(Pa), Altitude0(m),"
        "Temp1(C), Pressure1(Pa), Altitude1(m),"
        "Temp2(C), Pressure2(Pa), Altitude2(m),"
        "Gas1Voltage(V),"
        "Gas2Voltage(V)");
      dataFile.close();
      break; // leave the loop!
    }
  }

  Serial.print("Writing to ");
  Serial.println(Filename);


}

void loop() {
  if (timer > millis()) timer = millis(); //reset if it wraps

  // record data String after CollectDelay
  if (millis() - timer > CollectDelay)
  {

    File dataFile = SD.open(Filename, FILE_WRITE);

    //check if dataFile Opend
    if (!dataFile)
    {
      Serial.print("Could not open the SD file: ");
      Serial.println(Filename);
    } else {
      digitalWrite(YELLOW_LED, HIGH);
    }

    float measuredvbat = voltageFromADC(analogRead(VBATPIN) * 4, 3.3);
    Serial.print("VBat: " );
    Serial.println(measuredvbat);
    
    RecordData(dataFile, " Temperature 0 (C): ", baro.getTemperature());
    RecordData(dataFile, " Pressure 0 (Pa): ", baro.getPressure());
    RecordData(dataFile, " Altitude 0 (m): ", baro.getAltitude());
    Serial.println();

    RecordData(dataFile, " Temperature 1 (C): ", bmeA.readTemperature());
    RecordData(dataFile, " Pressure 1 (Pa): ", bmeA.readPressure());
    RecordData(dataFile, " Altitude 1 (m): ", bmeA.readAltitude(SEALEVELPRESSURE_HPA));
    Serial.println();

    RecordData(dataFile, " Temperature 2 (C): ", bmeB.readTemperature());
    RecordData(dataFile, " Pressure 2 (Pa): ", bmeB.readPressure());
    RecordData(dataFile, " Altitude 2 (m): ", bmeB.readAltitude(SEALEVELPRESSURE_HPA));
    Serial.println();

    long gasValue = analogRead(GASPIN_A);
    RecordData(dataFile, " Gas 1 Volt (V): ", voltageFromADC(gasValue, 5));

    Serial.println();

    gasValue = analogRead(GASPIN_B);
    RecordData(dataFile, " Gas 2 Volt (V): ", voltageFromADC(gasValue, 3.3));

    timer = millis(); // reset the timer
    Serial.println();
    digitalWrite(YELLOW_LED, LOW);
    dataFile.println();
    dataFile.close();
  }
}


void RecordData(File dataFile, char* Dataname, float data) {

  Serial.print(Dataname);
  Serial.print(data, 4);
  Serial.print(",");
  dataFile.print(data, 4);
  dataFile.print(",");

}

float voltageFromADC(float received_val, float reference) {
  float measured_val = received_val;
  measured_val *= reference;  // Multiply by reference voltage
  measured_val /= 4095; // convert to voltage
  return measured_val;
}
