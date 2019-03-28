#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_MPL3115A2.h>
#include <Adafruit_GPS.h>

// what's the name of the hardware serial port?
#define GPSSerial Serial1

// Connect to the GPS on the hardware port
Adafruit_GPS GPS(&GPSSerial);
     
// Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console
// Set to 'true' if you want to debug and listen to the raw GPS sentences
#define GPSECHO false

//Helpful numbers
#define BMEA_ADDR 0x76
#define BMEB_ADDR 0x77
#define YELLOW_LED 10
#define SEALEVELPRESSURE_HPA (1013.8)
#define VBATPIN A7
#define GASPIN_A A0 // powered by solar panels
#define GASPIN_B A2 // bat powered
#define SOLAR_ALT_MIN 2791.66 // height of mt lemmon in meters
#define SOLAR_ALT_MAX 21330 // 70,000 ft to meters

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

bool panelsRaised = false;
bool stayClosed = false;

void setup() {
  Serial.begin(115200); // we want a fast baudrate now to listen to GPS
  // 9600 NMEA is the default baud rate for Adafruit MTK GPS's- some use 4800
  GPS.begin(9600);
  // turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  // Set the update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate
  // For the parsing code to work nicely and have time to sort thru the data, and
  // print it out we don't suggest using anything higher than 1 Hz
     
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
        "Time, Date, Hour, Minute, Seconds,"
        "Altitude, Speed(knots), Latitude(Deg), Longitude(Deg), Sat#,"
        "Temp0(C), Pressure0(Hg), Altitude0(m),"
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

//HELPER FUNCTIONS


void RecordData(File dataFile, char* Dataname, float data) {

  Serial.print(Dataname);
  Serial.print(data, 4);
  Serial.print(",");
  dataFile.print(data, 4);
  dataFile.print(",");

}

void RecordTimeDate(File dataFile) {

  int gps_hour = GPS.hour - 19; // we have an offset in our GPS
  int gps_day = GPS.day;

  //fix offset if it goes to the next day
  if (gps_hour <= 0) {
    gps_hour = 24 + gps_hour;
    gps_day --;
  }

  //keep it out of military time
  if (gps_hour > 12) {
    gps_hour -= 12;
  }
  Serial.print(" Date/Time ");
  Serial.print(GPS.month, DEC);
  Serial.print("/");
  Serial.print(gps_day, DEC);
  Serial.print(", ");
  Serial.print(gps_hour);
  Serial.print(":");
  Serial.print(GPS.minute);
  Serial.print(":");
  Serial.print(GPS.seconds);
  Serial.print(":");
  Serial.print(GPS.milliseconds);
  Serial.print(",");

  dataFile.print(millis(), DEC);
  dataFile.print(",");
  dataFile.print(GPS.month, DEC);
  dataFile.print("/");
  dataFile.print(gps_day, DEC);
  dataFile.print(",");
  dataFile.print(gps_hour);
  dataFile.print(",");
  dataFile.print(GPS.minute);
  dataFile.print(",");
  dataFile.print(GPS.seconds);
  dataFile.print(",");
}


void RecordGPS(File dataFile) {
  //GPS data
  Serial.print(" altitude ");
  Serial.print(GPS.altitude);
  Serial.print(",");
  Serial.print(" knots ");
  Serial.print(GPS.speed);
  Serial.print(",");
  Serial.print("Location degrees: ");
  Serial.print(GPS.latitudeDegrees, 4);
  Serial.print(", ");
  Serial.print(GPS.longitudeDegrees, 4);
  Serial.print(", ");
  Serial.print(" sat ");
  Serial.print((int)GPS.satellites);
  Serial.print(",");
  dataFile.print(GPS.altitude);
  dataFile.print(",");
  dataFile.print(GPS.speed);
  dataFile.print(",");
  dataFile.print(GPS.latitudeDegrees, 4);
  dataFile.print(", ");
  dataFile.print(GPS.longitudeDegrees, 4);
  dataFile.print(", ");
  dataFile.print((int)GPS.satellites);
  dataFile.print(",");

}


float voltageFromADC(float received_val, float reference) {
  float measured_val = received_val;
  measured_val *= reference;  // Multiply by reference voltage
  measured_val /= 4095; // convert to voltage
  return measured_val;
}

void loop() {
  if (timer > millis()) timer = millis(); //reset if it wraps
  // read data from the GPS in the 'main loop'
  char c = GPS.read();
  
  if (GPS.newNMEAreceived()) {
    // a tricky thing here is if we print the NMEA sentence, or data
    // we end up not listening and catching other sentences!
    // so be very wary if using OUTPUT_ALLDATA and trytng to print out data
    if (!GPS.parse(GPS.lastNMEA())) // this also sets the newNMEAreceived() flag to false
      return; // we can fail to parse a sentence in which case we should just wait for another
  }

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

    // Record GPS data first
    // read data from the GPS in the 'main loop'
    RecordTimeDate(dataFile);
    RecordGPS(dataFile);

    float measuredvbat = voltageFromADC(analogRead(VBATPIN) * 4, 3.3);
    Serial.print("VBat: " );
    Serial.println(measuredvbat);
    
    RecordData(dataFile, " Temperature 0 (C): ", baro.getTemperature());
    RecordData(dataFile, " Pressure 0 (Hg): ", baro.getPressure());
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

    float avgAltitude = (baro.getAltitude() + bmeA.readAltitude(SEALEVELPRESSURE_HPA) + bmeB.readAltitude(SEALEVELPRESSURE_HPA)) / 3.0;
    
    // handle panels
    if (!stayClosed && !panelsRaised && (avgAltitude > SOLAR_ALT_MIN)) {
      panelsRaised = true;
      // raise panels
    } else if (!stayClosed && panelsRaised && (avgAltitude > SOLAR_ALT_MAX)) {
      panelsRaised = false;
      stayClosed = true;
      //close panels
    }

    timer = millis(); // reset the timer
    Serial.println();
    digitalWrite(YELLOW_LED, LOW);
    dataFile.println();
    dataFile.close();
  }
}
