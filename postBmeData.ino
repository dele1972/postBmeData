/*
    This sketch establishes a TCP(SSL) connection to my Server and POST BME Temperature Data to it.
    This is a proof of concept test sketch and will be used for public documentation too.

    NodeMCU + BME280 --> WLAN(WPA2) --> POST Homepage(TLS)
*/

#include <Dele72.h>           // provides some 'secret' constants @TODO: you can delete this line

#include <PolledTimeout.h>
#include <ESP8266WiFi.h>      // to create a WiFi Client (https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/readme.html)
#include <WiFiClientSecure.h> // see expl.: https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFiClientSecure/examples/WiFiClientSecure/WiFiClientSecure.ino
#include <Wire.h>             // to communicate via I2C (https://www.arduino.cc/en/Reference/Wire)
#include <Adafruit_Sensor.h>  // this is a base class where sensor 'drivers' connects on top for standarized use (https://github.com/adafruit/Adafruit_Sensor)
#include <Adafruit_BME280.h>  // to use the BME280 Sensor (Humidity, Barometric Pressure + Temp sensor)
#include <Adafruit_ADS1X15.h>

const uint16_t deviceID = 2;                                  // Device/Placement ID

/* WLAN Settings */
WiFiClientSecure client;       // ssl supporting client (ex. from https://arduino-esp8266.readthedocs.io/en/2.5.2/esp8266wifi/client-secure-examples.html)
IPAddress localIp;             // local WLAN IP of ESP-Device
char* usedSSID;                // takes the ssid which is used after iteration through available ap's
long rssi;                     // signal strength of current wlan connection in dBm
// const char* ssid        = WlanConstants::SSID_DEVA;
// const char* password    = WlanConstants::WIFIPASSWORD;


/* HOST Connection Settings */
const char* host                           = HostConstants::DATA_HOST;
const char* url                            = HostConstants::URL_DATAHOME;
const uint16_t port                        = 443;                                // Port to start with SSL (https) connection
// const char* fingerprint                    = HostConstants::MAIN_FINGERPRINT256; // fingerprint sha1 or sha-256 for SSL cert validation; @TODO: change this to your value (like `urlTest`)
//unsigned char MYROOT_CA[sizeof(ROOT_CA)-1] = {};  // this will be filled later with my root certificate (ssl)
#define MYROOT_CA ROOT_CA

// uint16_t       Unsigned      0 to +65,535                  // good to know :)
// uint32_t       Unsigned      0 to +4,294,967,295           // same


/* For A0 Battery Voltage reading                             // DEPRECATED, Voltage monitoring now by ADS1115 */
// uint16_t analogInPin = A0;
// uint16_t sensorValue;
// float voltage;
// const uint16_t MAXSENSORVALUES = 1024;
// const float CALIBRATION = 0.2; // 0.36
// int bat_percentage;


/* ADS1115 Definitions */
Adafruit_ADS1115 ads;
int16_t adc0;                   // sensor value of A0
int16_t adc1;                   // sensor value of A1
int16_t u1mv0     = 0;          // U1 of A0in in mV
int16_t u1mv1     = 0;          // U1 of A1in in mV
const uint32_t R1 = 100000;     // voltage divider R1 for A0in (Solarpanel)
const uint32_t R2 = 220000;     // voltage divider R2 for A0in (Solarpanel)
float uGes        = 0.0;        // Uges in mV
enum ADS115PINS { pin_A0, pin_A1, pin_A2, pin_A3 };


/* Some BME280 Definitions */
#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme; // I2C
float Humidity;
float Temperature_Cel;
float Pressure;
float ApproxAltitude;

// unsigned long delayTime;


void setup() {

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level), will be turned off by deepsleep

  // start serial monitor
  Serial.begin(115200);

  Serial.println("Start");

  Serial.println("\n\nStart INIT ... \n\n");


  // start W-LAN access to AP
  connectWifi();


  // init ADS1115
  ads.begin();
  ads.setGain(GAIN_ONE);
  ads.startComparator_SingleEnded(1, 1000);


  // init BME280 temperetature sensor
  initBme();


  Serial.println("\n\n... INIT end \n\n");
}


void loop() {

  Serial.println("*** LOOP START ***");


  /* Read BME DATA */
  readBmeData();

  // Check if any reads failed and exit early (to try again).
  if (isnan(Humidity) || isnan(Temperature_Cel) || isnan(Pressure)) {
    
    Serial.println("Failed to read from BME sensor!");
    return;
  }


  // 'First Shot' - at this point nonsense values
  getVoltage();

  // 'Second Shot' needed - at this point correct values
  getVoltage();

  
  connectWebserver();
  
  if (client.connected()) {
    sendDataToHost();
  }
  

  /* if the server disconnected, stop the client */
  if (!client.connected()) {
      Serial.println();
      Serial.println("Server disconnected");
      client.stop();
  }


  /* shut down to deepsleep mode (NodeMCU: GPIO16[labeledD0] connect to RST)) */
  Serial.println("\n\n   *** Shut Down *** \n\n");
  
  // delay(300000);                 // execute once every 5 minutes, don't flood remote service
  ESP.deepSleep(5 * 60 * 1000000);  // deepsleep for 5 Minutes
}


void connectWifi() {
//  for(int i=0; i < 2; i++) {
//    usedSSID = WlanConstants::SSIDS[i];
//    Serial.println(usedSSID);
//  }
//  Serial.println("END");
//  delay(1000);
//  return;




  // We start by connecting to a WiFi network
  Serial.println();
  for(int i=0; i < 2; i++) {
    usedSSID = WlanConstants::SSIDS[i];
    connect2SSID(usedSSID);

    // Measure Signal Strength (RSSI) of Wi-Fi connection
    rssi = WiFi.RSSI();

    Serial.println();
    Serial.print("   RSSID = "); Serial.println(rssi);

    if (rssi > -80) {
      Serial.println("\nSignal strength is worse, break...");
      break;
    }
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  localIp = WiFi.localIP();     // get the local IP
  Serial.println(localIp);
}


void connect2SSID(char* ssid) {
  Serial.println();
  Serial.print("Connecting to " + *ssid);

  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, WlanConstants::WIFIPASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
}


void connectWebserver() {

  /* Get current time */
  // Synchronize time useing SNTP. This is necessary to verify that
  // the TLS certificates offered by the server are currently valid.
  Serial.print("Setting time using SNTP");
  configTime(8 * 3600, 0, "0.de.pool.ntp.org", "3.de.pool.ntp.org");  // https://www.pool.ntp.org/zone/de
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));

  /* copy Dele72.ROOT_CA to local MYROOT_CA */
  /*
  for (int i = 0; i < sizeof(ROOT_CA); i++) {
    MYROOT_CA[i] = ROOT_CA[i]; //copies UserInput in reverse to TempInput
    // printf("%c", MYROOT_CA[i]);
  }
  MYROOT_CA[sizeof(ROOT_CA)] = '\0'; // adds NULL character at end  
  */

  /*
  // print copied certificate to Serial Monitor
  for (int i = 0; i < sizeof(MYROOT_CA); i++) {
    printf("%c", MYROOT_CA[i]);
  }
  */


  /* Connect to Server */
  Serial.println(String("\n\nStarting connection to server...") + host);
  
  // client.setInsecure();  // https://github.com/esp8266/Arduino/issues/4826#issuecomment-491813938 BUT THEN THE CONNECTION BECOMES INSECURE!
  // client.setFingerprint(fingerprint);  // The use of validating by Fingerprint isn't recommended because the fingerprint will be renewed more often than the certificate and then the fingerprint has to be updated
  client.setCACert(MYROOT_CA, sizeof(MYROOT_CA)-1); // https://stackoverflow.com/a/56203388
  
  if (!client.connect(host, port)) {
    
    Serial.println("connection failed");
    
    // if connection is failed try again in 5 Seconds
    delay(5000);
    return;
  }

  Serial.println("Connected to server!");


  /* Verify Server */

  // Verify validity of server's certificate
  if (client.verifyCertChain(host)) {

    Serial.println("Server certificate verified");
  } else {

    Serial.println("ERROR: certificate verification failed!");
    return;
  }
  /*
  if (client.verify(fingerprint, host)) {
  
    Serial.println("certificate matches");

  } else {

    Serial.println("certificate doesn't match");

  }
  */

}


void sendDataToHost() {
  
  /* Set Data to send */

  // Create manually a POST request (like https://developer.mozilla.org/en-US/docs/Web/HTTP/Methods/POST)
  // Instead the ESP8266HTTPClient.h could be used
  
  //    char* localip = (String) WiFi.localIP();
  //        + "&ip="            + (String) WiFi.localIP()
  String DataToSend = "DEVICE="           + (String) deviceID
                    + "&SSID="            + (String) usedSSID
                    + "&RSSI="            + (String) rssi
                    + "&HUMIDITY="        + (String) Humidity
                    + "&TEMPERATURE="     + (String) Temperature_Cel
                    + "&PRESSURE="        + (String) Pressure
                    + "&APPROXALTITUDE="  + (String) ApproxAltitude
                    + "&VOLTAGESOLAR="    + (String) uGes
                    + "&VOLTAGEACCU="     + (String) u1mv1;

  /* create HTTP POST request */
  
  client.println(String("POST ") + url + " HTTP/1.1");
  client.println(String("Host: ") + host);                 
  client.println("Content-Type: application/x-www-form-urlencoded");
  client.println("User-Agent: ESP8266/1.0");
  client.println(String("Content-Length: ") + DataToSend.length());
  client.println("Connection: close"); 
  client.println(""); 
  client.print(DataToSend);
  
  Serial.println("\n");
  Serial.println("My data string im POSTing looks like this: ");
  Serial.println(DataToSend);
  Serial.println("And it is this many bytes: ");
  Serial.println(DataToSend.length());       
  Serial.println("\n");


  /* wait for Server Response */
  /*
    while (client.connected()) {
      String line = client.readStringUntil('\n');
      Serial.println(line);
      if (line == "\r") {
        Serial.println("headers received");
        break;
      }
    }
  */

  Serial.print("Waiting for response:\r\n");
  while (!client.available()){
      Serial.print("z");
      delay(50); //
  }  

  
  /* read and check Server Response */
  /*
    String line = client.readStringUntil('\n');
    if (line.startsWith("{\"state\":\"success\"")) {
      Serial.println("esp8266/Arduino CI successfull!");
    } else {
      Serial.println("esp8266/Arduino CI has failed");
    Serial.println(line);
    }
  */
  // if data is available then receive and print to Terminal
  while (client.available()) {
      char c = client.read();
      // Serial.print("+");
      Serial.write(c);
  }
}


float mapfloat(float x, float in_min, float in_max, float out_min, float out_max) {

  return (x- in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


void getVoltage() {
  
  adc0 = ads.readADC_SingleEnded(pin_A0);     // read Value from pin A0 (Solarpanel)
  adc1 = ads.readADC_SingleEnded(pin_A1);     // read Value from pin A1 ('Accu')
  Serial.print("AIN-0: "); Serial.print(adc0); Serial.print("   ---   AIN-1: "); Serial.println(adc1);
  u1mv0 = adc0 * 0.125;
  u1mv1 = adc1 * 0.125;

  Serial.print("AIN-0 mv (measured): "); Serial.println(u1mv0);

  uGes = (float) u1mv0 / ( (float) R1 / ( R1 + R2 ) );
  Serial.print("AIN-0 mV (calc): "); Serial.print(uGes); Serial.print("   ***   AIN-1 mV: "); Serial.println(u1mv1);

/*
  // DEPRECATED (read sensor values by A0 of ESP)
  sensorValue = analogRead(analogInPin);
  voltage = (sensorValue * 3.3 / MAXSENSORVALUES + CALIBRATION);
  bat_percentage = mapfloat(voltage, 2.8, 3.6, 0, 100);
  if (bat_percentage >= 100) {
    bat_percentage = 100;
  }
  if (bat_percentage <= 0) {
    bat_percentage = 1;
  }

  Serial.print("\nAnalog Value = ");
  Serial.print(sensorValue);
  Serial.print("\t Output Voltage = ");
  Serial.print(voltage);
  Serial.print("\t Battery Percentage = ");
  Serial.print(bat_percentage);
*/
}


void readBmeData() {

  Humidity = bme.readHumidity();
  Temperature_Cel = bme.readTemperature();
  Pressure = bme.readPressure() / 100.0F;
  ApproxAltitude = bme.readAltitude(SEALEVELPRESSURE_HPA);

  Serial.print("Humidity: "); Serial.print(Humidity);
  Serial.print("   ---   Temperature: "); Serial.print(Temperature_Cel);
  Serial.print("   ---   Pressure: "); Serial.print(Pressure);
  Serial.print("   ---   ApproxAltitude: "); Serial.println(ApproxAltitude);
}


void initBme() {

  while(!Serial);    // time to get serial running
  Serial.println(F("BME280 test"));

  unsigned status;
  
  // default settings
  // status = bme.begin();  
  status = bme.begin(0x76);  
  // You can also pass in a Wire library object like &Wire2
  // status = bme.begin(0x76, &Wire2);
  if (!status) {
      Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
      Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(),16);
      Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
      Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
      Serial.print("        ID of 0x60 represents a BME 280.\n");
      Serial.print("        ID of 0x61 represents a BME 680.\n");
      while (1) delay(10);
  }
}
