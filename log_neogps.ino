#include <avr/sleep.h>

#include <SPI.h>
#include <SdFat.h>

#include <Wire.h>

#define CHIP_SELECT 10
//#define LED_PIN 13

#define RED 0x1
#define YELLOW 0x3
#define GREEN 0x2
#define TEAL 0x6
#define BLUE 0x4
#define VIOLET 0x5
#define WHITE 0x7

#define MAXMODE 2
#define LCD
//#define IMU

#define IMU_SF

SdFat SD;
File logfile;

#ifdef LCD
  // https://github.com/lincomatic/LiquidTWI2
  #include <LiquidTWI2.h>
  LiquidTWI2 lcd(0x20);
#endif

#ifdef IMU
  #include <Adafruit_LSM9DS1.h>
  #include <Adafruit_Sensor.h>

  Adafruit_LSM9DS1 lsm = Adafruit_LSM9DS1();
  
  #define LSM9DS1_SCK A5
  #define LSM9DS1_MISO 12
  #define LSM9DS1_MOSI A4
  #define LSM9DS1_XGCS 6
  #define LSM9DS1_MCS 5

#endif

#ifdef IMU_SF
  #include <SparkFunLSM9DS1.h>
  #define LSM9DS1_M   0x1E // Would be 0x1C if SDO_M is LOW
  #define LSM9DS1_AG  0x6B // Would be 0x6A if SDO_AG is LOW

  LSM9DS1 imu;
  
#endif

#include <NeoSWSerial.h>
NeoSWSerial gpsPort(8, 7);

#include <NMEAGPS.h>
NMEAGPS gps;

#ifdef IMU
void setupIMU()
{
  // 1.) Set the accelerometer range
  lsm.setupAccel(lsm.LSM9DS1_ACCELRANGE_2G);
  //lsm.setupAccel(lsm.LSM9DS1_ACCELRANGE_4G);
  //lsm.setupAccel(lsm.LSM9DS1_ACCELRANGE_8G);
  //lsm.setupAccel(lsm.LSM9DS1_ACCELRANGE_16G);
  
  // 2.) Set the magnetometer sensitivity
  lsm.setupMag(lsm.LSM9DS1_MAGGAIN_4GAUSS);
  //lsm.setupMag(lsm.LSM9DS1_MAGGAIN_8GAUSS);
  //lsm.setupMag(lsm.LSM9DS1_MAGGAIN_12GAUSS);
  //lsm.setupMag(lsm.LSM9DS1_MAGGAIN_16GAUSS);

  // 3.) Setup the gyroscope
  lsm.setupGyro(lsm.LSM9DS1_GYROSCALE_245DPS);
  //lsm.setupGyro(lsm.LSM9DS1_GYROSCALE_500DPS);
  //lsm.setupGyro(lsm.LSM9DS1_GYROSCALE_2000DPS);
}
#endif

void setup() {

  #ifdef LCD
  /*lcd.begin(16, 2);
  lcd.setBacklight(WHITE);
  lcd.setCursor(0,0);*/

  lcd.setMCPType(LTI_TYPE_MCP23017); // must be called before begin()
   lcd.begin(16,2);
   lcd.setBacklight(WHITE); // see LiquidTWI2.h for color options
  lcd.print("DreiD Logger 0.1");
  #endif

  // connect at 115200 so we can read the GPS fast enough and echo without dropping chars
  // also spit it out
  Serial.begin(115200);
  //Serial.println( F("\r\nDreiD Logger 0.1") ); // F macro saves RAM!
  //pinMode(LED_PIN, OUTPUT);

  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);

  #ifdef IMU
  if (!lsm.begin())
  {
    Serial.println("Oops ... unable to initialize the LSM9DS1. Check your wiring!");
    while (1);
  }
  Serial.println("Found LSM9DS1 9DOF");

  // helper to just set the default scaling we want, see above!
  setupIMU();
  #endif

  #ifdef IMU_SF
    imu.settings.device.commInterface = IMU_MODE_I2C; // Set mode to I2C
    imu.settings.device.mAddress = LSM9DS1_M; // Set mag address to 0x1E
    imu.settings.device.agAddress = LSM9DS1_AG; // Set ag address to 0x6B
    imu.settings.accel.scale=2;
    imu.settings.gyro.scale = 245;
    imu.settings.mag.scale = 4;
    imu.begin();
  #endif
  
  // see if the card is present and can be initialized:
  if (!SD.begin(CHIP_SELECT)) {      // if you're using an UNO, you can use this line instead
    Serial.println( F("Card init. failed!") );

    #ifdef LCD
    lcd.clear();
    lcd.print("Card init failed");
    delay(5000);
    lcd.clear();
    #endif
  }

  char filename[15] = "GPSLOG00.CSV";
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = '0' + i/10;
    filename[7] = '0' + i%10;
    // create if does not exist, do not open existing, write, sync after write
    if (! SD.exists(filename)) {
      break;
    }
  }

  logfile = SD.open(filename, FILE_WRITE);
  if( ! logfile.isOpen() ) {
    Serial.print( F("Couldnt create ") );
    Serial.println(filename);
    #ifdef LCD
      lcd.print("Cant create file");
      delay(5000);
    #endif
  }

  Serial.print( F("Writing to ") );
  Serial.println(filename);

  gpsPort.begin( 9600 );

  gps.send_P( &gpsPort, F("PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0") ); // RMC_GGA
  //gps.send_P( &gpsPort, F("PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0") );  // RMC only
  gps.send_P( &gpsPort, F("PMTK220,200") );  // 5Hz output rate
  gps.send_P( &gpsPort, F("PMTK300,200,0,0,0,0") ); // 5 Hz update rate
  gps.send_P( &gpsPort, F("PGCMD,33,0") );   // No antenna status messages needed

  Serial.println("Ready!");
  #ifdef LCD
  lcd.clear();
  #endif
}


uint32_t timer = 0;//millis();
uint32_t debounceDelay=400;
uint8_t mode=1;

void loop() {
  
  #ifdef LCD
  // handling buttons
  uint8_t buttons = lcd.readButtons();
  if (buttons) 
  {
    //timer=millis();
    if(millis()-timer>debounceDelay)
    { 
      timer=millis();     
      if (buttons & BUTTON_LEFT) {
        mode--;
        if(mode<1) mode=MAXMODE;
        //lcd.setBacklight(BLUE);
        lcd.clear();
      }
      if (buttons & BUTTON_RIGHT) {
        mode++;
        if(mode>MAXMODE) mode=1;
        //lcd.setBacklight(WHITE);
        lcd.clear();
      }
      //Serial.println(mode);
    }
  }
  //delay(70);
  #endif

  // Parse GPS characters until a complete fix has been assembled.
  if (gps.available( gpsPort )) {

    // A complete fix structure is finally ready, get it.
    gps_fix fix = gps.read();

    /*Serial.print(fix.dateTime_cs);
    Serial.print(" ");
    Serial.print(fix.valid.location);
    Serial.print(" ");
    Serial.println(fix.altitude());*/

    //if (timer > millis())  timer = millis();
  
    // approximately every 2 seconds or so, print out the current stats
    //if (millis() - timer > 1000) { 
      //timer = millis(); // reset the timer
      if(fix.valid.location)
      {
        #ifdef LCD
        lcd.setBacklight(GREEN);
        if(mode==1)
        {
          lcd.setCursor(0,0);
          lcd.print("lat ");          
          lcd.setCursor(4,0);
          lcd.print(fix.latitude(), 8);
          lcd.setCursor(0,1);
          lcd.print("lon ");          
          lcd.setCursor(4,1);
          lcd.print(fix.longitude(), 8);
        }
        else if(mode==2)
        {
          lcd.setCursor(0,0);
          lcd.print("spd");          
          lcd.setCursor(4,0);
          lcd.print(fix.speed_kph(), 1);
          lcd.setCursor(0,1);
          lcd.print("hdg");          
          lcd.setCursor(4,1);
          lcd.print(fix.heading(), 1);         
        }
        #endif
      }
      else
      {
        #ifdef LCD
        lcd.setBacklight(RED);
        lcd.setCursor(0,0);
        //lcd.clear();
        lcd.print("No fix");
        #endif
        return;
      }
    //}         

    /*Serial.print(fix.dateTime_cs);
    Serial.print(" ");
    Serial.print(fix.altitude());
    Serial.print(" ");*/
    //Serial.println(fix.geoidHeight());
    
    logfile.print(fix.dateTime.hours);
    logfile.print(':');
    logfile.print(fix.dateTime.minutes);
    logfile.print(':');
    logfile.print(fix.dateTime.seconds);
    logfile.print('.');
    if (fix.dateTime_cs < 10)
      logfile.print( '0' );
    logfile.print(fix.dateTime_cs);
    logfile.print(',');

    logfile.print(fix.dateTime.date);
    logfile.print('/');
    logfile.print(fix.dateTime.month);
    logfile.print(',');
    logfile.print(fix.latitude(), 8);
    logfile.print(", ");
    logfile.print(fix.longitude(), 8);
    logfile.print(", ");
    logfile.print(fix.altitude() );
    logfile.print(',');
    logfile.print(fix.speed_kph());
    logfile.print(',');
    logfile.print(fix.heading());

    #ifdef IMU
      lsm.read();  /* ask it to read in the data */ 
    
      /* Get a new sensor event */ 
      sensors_event_t a, m, g, temp;
    
      lsm.getEvent(&a, &m, &g, &temp); 
    
      logfile.print(',');
      logfile.print(a.acceleration.x);
      logfile.print(',');
      logfile.print(a.acceleration.y);
      logfile.print(',');
      logfile.print(a.acceleration.z);
      logfile.print(',');
      logfile.print(m.magnetic.x);
      logfile.print(',');
      logfile.print(m.magnetic.y);
      logfile.print(',');
      logfile.print(m.magnetic.z);
      logfile.print(',');
      logfile.print(g.gyro.x);
      logfile.print(',');
      logfile.print(g.gyro.y);
      logfile.print(',');
      logfile.print(g.gyro.z);
    #endif

    #ifdef IMU_SF
      imu.readAccel();
      logfile.print(',');
      logfile.print(imu.calcAccel(imu.ax));
      logfile.print(',');
      logfile.print(imu.calcAccel(imu.ay));
      logfile.print(',');
      logfile.print(imu.calcAccel(imu.az));

      imu.readMag();
      logfile.print(',');
      logfile.print(imu.calcMag(imu.mx));
      logfile.print(',');
      logfile.print(imu.calcMag(imu.my));
      logfile.print(',');
      logfile.print(imu.calcMag(imu.mz));
      
      imu.readGyro();
      logfile.print(',');
      logfile.print(imu.calcGyro(imu.gx));
      logfile.print(',');
      logfile.print(imu.calcGyro(imu.gy));
      logfile.print(',');
      logfile.print(imu.calcGyro(imu.gz));
      
    #endif

    logfile.println();
    logfile.flush();
  }
}
