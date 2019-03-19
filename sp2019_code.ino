#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_LSM9DS1.h>
#include <Adafruit_Sensor.h>  // not used in this demo but required!

#include <servo.h>
#include <Adafruit_MPL3115A2.h>
#include <Adafruit_BMP280.h>


// Links for the MPL3115A2 sensor
'''
https://www.adafruit.com/product/1893
https://github.com/adafruit/Adafruit_MPL3115A2_Library
'''

// Links for the BMP280
'''
https://www.adafruit.com/product/2652
'''

#define UNIVERSAL_SCK 9
#define UNIVERSAL_MISO 12
#define UNIVERSAL_MOSI 10
#define LSM9DS1_XGCS 6
#define LSM9DS1_MCS 5

#define BMPaddr1 0x76
#define BMPaddr2 0x77

#define Turbine1Pin A1
#define Turbine2Pin A2
#define Turbine2PinBackUp A3

#define GreenLED 8

#define VBATPIN A7

//set Delay Between Data Points
int CollectDelay = 1000;

// Baromiter sensor definition
Adafruit_MPL3115A2 baro = Adafruit_MPL3115A2();

// First BMP280 sensor
Adafruit_BMP280 bmeA;

// Second BMP280 sensor
Adafruit_BMP280 bmeB;

uint32_t timer;
char Filename[] = "18F000.csv";
int first0Index = 3;
int second0Index = 4;
int third0Index = 5;

void setup() {
  Serial.begin(115200);
  timer = millis();
  pinMode(GreenLED, OUTPUT);

  if (!bmeA.begin()) {
    Serial.println("Could not find a valid BMP280 A sensor, check wiring!");
    while (1);
  }
  if (!bmeB.begin()) {
    Serial.println("Could not find a valid BMP280 B sensor, check wiring!");
    while (1);
  }

  //Init the SD card and format the columns
  if (!SD.begin(4))
    {
        Serial.println("SD card initialization failed!");
        //digitalWrite(redLED, HIGH);
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
              "AccelX1(m/s^2), AccelY1(m/s^2), AccelZ1(m/s^2),"
              "MagX1(gauss), MagY1(gauss), MagZ1(gauss),"
              "GyroX1(dps), GyroY1(dps), GyroZ1(dps),"
              "AccelX2(m/s^2), AccelY2(m/s^2), AccelZ2(m/s^2),"
              "MagX2(gauss), MagY2(gauss), MagZ2(gauss),"
              "GyroX2(dps), GyroY2(dps), GyroZ2(dps),"
              "Temp(C), Pressure(Pa), Altitude(m),"
              "Turbine1Voltage(V),"
              "Turbine2Voltage(V)");
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
      digitalWrite(GreenLED, HIGH);
    }

    float measuredvbat = analogRead(VBATPIN);
    measuredvbat *= 2;    // we divided by 2, so multiply back
    measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
    measuredvbat /= 4095; // convert to voltage
    Serial.print("VBat: " );
    Serial.println(measuredvbat);

    '''
    Record Data for the BME A senor
    '''
    RecordData(dataFile, " Temperature (C): ", bmeA.readTemperature());
    RecordData(dataFile, " Pressure (Pa): ", bmeA.readPressure());
    RecordData(dataFile, " Altitude (m): ", bmeA.readAltitude(1017)); // 1017 = local PHX hPa
    Serial.println();

    '''
    Record Data for the BME B senor
    '''
    RecordData(dataFile, " Temperature (C): ", bmeB.readTemperature());
    RecordData(dataFile, " Pressure (Pa): ", bmeB.readPressure());
    RecordData(dataFile, " Altitude (m): ", bmeB.readAltitude(1017)); // 1017 = local PHX hPa
    Serial.println();

    long turbineValue = analogRead(Turbine1Pin);
    RecordData(dataFile, " Turbine 1 Volt (V): ", (turbineValue*3.3)/4095);

    Serial.println();

    turbineValue = analogRead(Turbine2Pin);
    RecordData(dataFile, " Turbine 2 Volt (V): ", (turbineValue*3.3)/4095);

    timer = millis(); // reset the timer
    Serial.println();
    digitalWrite(GreenLED, LOW);
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
