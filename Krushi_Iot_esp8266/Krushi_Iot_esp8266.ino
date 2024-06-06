#include <ModbusMaster.h>
#include <SoftwareSerial.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
// Replace with your Wi-Fi credentials
const char* ssid = "FD-98";
const char* password = "12345678";
// Replace with your Firebase project credentials
#define FIREBASE_HOST "in1-data-87b1f-default-rtdb.firebaseio.com"  
#define FIREBASE_AUTH "5zPk4GLtQu6vfhhSkO3RSncpR700nbpeIHOkuckk"
// Firebase data path
#define TEMP_PATH "/temperature"
#define temp_path "/atmtemp"  
#define HUMIDITY_PATH "/humidity"
#define MOISTURE_PATH "/moisture"
#define EC_PATH "/ec"
#define PH_PATH "/ph"
#define N_PATH "/nitrogen"
#define P_PATH "/phosphorus"
#define K_PATH "/potassium"
const int RO_PIN = D5; // Receive (data in) pin
const int DI_PIN = D6; // Transmit (data out) pin
const int RE_PIN = D1; // Change this to D0 (GPIO16) or an available GPIO pin on ESP8266
const int DE_PIN = D0; // Change this to D1 (GPIO5) or an available GPIO pin on ESP8266
const int DHT_PIN = D4; // DHT data pin, change this to D4 (GPIO2) or an available GPIO pin on ESP8266
const int DHT_TYPE = DHT11; // DHT sensor type (DHT11, DHT21, DHT22)
SoftwareSerial swSerial(RO_PIN, DI_PIN); // Use SoftwareSerial for communication
ModbusMaster node;
DHT dht(DHT_PIN, DHT_TYPE);
// Define Firebase Data objects
FirebaseData firebaseData;
// Put the MAX485 into transmit mode
void preTransmission()
{
digitalWrite(RE_PIN, HIGH);
digitalWrite(DE_PIN, HIGH);
}
// Put the MAX485 into receive mode
void postTransmission()
{
digitalWrite(RE_PIN, LOW);
digitalWrite(DE_PIN, LOW);
}
void setup()
{
Serial.begin(9600);
// Connect to Wi-Fi
WiFi.begin(ssid, password);
while (WiFi.status() != WL_CONNECTED)
{
delay(500);
Serial.print(".");
}
Serial.println();
Serial.print("Connected to WiFi: ");
Serial.println(WiFi.SSID());
// Initialize Firebase
Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
// Configure the MAX485 RE & DE control signals and enable receive mode
pinMode(RE_PIN, OUTPUT);
pinMode(DE_PIN, OUTPUT);
digitalWrite(DE_PIN, LOW);
digitalWrite(RE_PIN, LOW);
// Modbus communication runs at 9600 baud
swSerial.begin(9600);
// Modbus slave ID of the NPK sensor is 2
node.begin(1, swSerial);
// Callbacks to allow us to set the RS485 Tx/Rx direction
node.preTransmission(preTransmission);
node.postTransmission(postTransmission);
// Initialize the DHT sensor
dht.begin();
delay(1500);
}
void loop()
{
uint8_t result;
// Remove any characters from the receive buffer
// Ask for 7x 16-bit words starting at register address 0x0000
result = node.readHoldingRegisters( 0x001E, 7);
Serial.print(result);
if (result == node.ku8MBSuccess)
{
float temperature = node.getResponseBuffer(5)*8.0;
float humidity = node.getResponseBuffer(7);
float moisture = node.getResponseBuffer(6)*9.0; // Divide by 10 for moisture
float ec = node.getResponseBuffer(4)*10;
float ph = node.getResponseBuffer(3) / 1010.0;
float nitrogen = node.getResponseBuffer(2)/10;
float phosphorus = node.getResponseBuffer(1)/10;
float potassium = node.getResponseBuffer(0)/10;
// Update Firebase with sensor data
Firebase.setString(firebaseData, TEMP_PATH, String(temperature)+ " F");
//Firebase.setString(firebaseData, HUMIDITY_PATH, String(humidity) + " %");
Firebase.setString(firebaseData, MOISTURE_PATH, String(moisture) + " %"); // Send moisture data to Firebase
Firebase.setString(firebaseData, EC_PATH, String(ec)+ " uS/cm");
Firebase.setString(firebaseData, PH_PATH, String(ph));
Firebase.setString(firebaseData, N_PATH, String(nitrogen)+ " mg/kg");
Firebase.setString(firebaseData, P_PATH, String(phosphorus)+ " mg/kg");
Firebase.setString(firebaseData, K_PATH, String(potassium)+ " mg/kg");
Serial.println(temperature);
Serial.println(moisture);
Serial.println(ec);
Serial.println(ph);
Serial.println(nitrogen);
Serial.println(phosphorus);
Serial.println(potassium);
Serial.println("Sensor data sent to Firebase!");
}
else
{
printModbusError(result);
}
// Read temperature and humidity from DHT sensor
float atmtemp = dht.readTemperature();
float humidity = dht.readHumidity();
// Check if any valid data was obtained from the DHT sensor
if (!isnan(atmtemp) && !isnan(humidity))
{
// Update Firebase with DHT data
Firebase.setString(firebaseData, temp_path, String(atmtemp));
Firebase.setString(firebaseData, HUMIDITY_PATH, String(humidity));
Serial.print(humidity);
Serial.println(atmtemp);
Serial.println("DHT data sent to Firebase!");
}
else
{
Serial.println("Failed to read data from DHT sensor");
}
delay(1500);
}
// Print out the error received from the Modbus library
void printModbusError(uint8_t errNum)
{
switch (errNum)
{
case node.ku8MBSuccess:
Serial.println(F("Success"));
break;
case node.ku8MBIllegalFunction:
Serial.println(F("Illegal Function Exception"));
break;
case node.ku8MBIllegalDataAddress:
Serial.println(F("Illegal Data Address Exception"));
break;
case node.ku8MBIllegalDataValue:
Serial.println(F("Illegal Data Value Exception"));
break;
case node.ku8MBSlaveDeviceFailure:
Serial.println(F("Slave Device Failure"));
break;
case node.ku8MBInvalidSlaveID:
Serial.println(F("Invalid Slave ID"));
break;
case node.ku8MBInvalidFunction:
Serial.println(F("Invalid Function"));
break;
case node.ku8MBResponseTimedOut:
Serial.println(F("Response Timed Out"));
break;
case node.ku8MBInvalidCRC:
Serial.println(F("Invalid CRC"));
break;
default:
Serial.println(F("Unknown Error"));
break;
}
}
