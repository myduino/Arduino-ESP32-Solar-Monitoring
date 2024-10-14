#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <BH1750.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define ONE_WIRE_BUS 13      
#define ANALOG_IN_PIN 32     

const char* ssid = "Myduino Team";  
const char* password = "InspireToInvent@2009";  

const char* mqtt_server = "broker.hivemq.com";  
const char* mqtt_topic = "dududududu"; 

WiFiClient espClient;
PubSubClient client(espClient);

BH1750 lightMeter;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

int numberOfDevices; 
DeviceAddress tempDeviceAddress;

float adc_voltage = 0.0;
float in_voltage = 0.0;

float R1 = 30000.0;  
float R2 = 7500.0;   
float ref_voltage = 3.3; 
int adc_value = 0;

void setup_wifi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    // Attempt to connect
    if (client.connect("ESP32Client")) {
      Serial.println("Connected to MQTT broker");
    } else {
      Serial.print("MQTT connection failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5 seconds");
      delay(3000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("DC Voltage, Temperature, and Light Measurement");

  setup_wifi();  
  client.setServer(mqtt_server, 1883); 

  sensors.begin();
  numberOfDevices = sensors.getDeviceCount();
  
  Serial.print("Found ");
  Serial.print(numberOfDevices, DEC);
  Serial.println(" DS18B20 temperature devices.");

  for (int i = 0; i < numberOfDevices; i++) {
    if (sensors.getAddress(tempDeviceAddress, i)) {
      Serial.print("Found device ");
      Serial.print(i, DEC);
      Serial.print(" with address: ");
      printAddress(tempDeviceAddress);
      Serial.println();
    } else {
      Serial.println("Could not detect address. Check connections.");
    }
  }

  Wire.begin(21, 22);  
  if (lightMeter.begin()) {
    Serial.println(F("BH1750 light sensor initialized"));
  } else {
    Serial.println(F("Failed to initialize BH1750!"));
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  adc_value = analogRead(ANALOG_IN_PIN);

  adc_voltage = (adc_value * ref_voltage) / 4095.0;  
  
  in_voltage = adc_voltage / (R2 / (R1 + R2));
  
  Serial.print("Input Voltage = ");
  Serial.print(in_voltage, 2);
  Serial.println(" V");

  sensors.requestTemperatures();
  String sensorData = "";

  for (int i = 0; i < numberOfDevices; i++) {
    if (sensors.getAddress(tempDeviceAddress, i)) {
      float tempC = sensors.getTempC(tempDeviceAddress);
      Serial.print("Temp C: ");
      Serial.print(tempC);
      Serial.print(" °C  |  ");

      Serial.print("Temp F: ");
      Serial.print(DallasTemperature::toFahrenheit(tempC));
      Serial.println(" °F");

      sensorData += "Temp C: " + String(tempC) + "°C | ";
      sensorData += "Temp F: " + String(DallasTemperature::toFahrenheit(tempC)) + "°F | ";
    }
  }

  float lux = lightMeter.readLightLevel();
  Serial.print("LIGHT: ");
  Serial.print(lux);
  Serial.println(" lx");

  sensorData += "Light: " + String(lux) + " lx | ";
  sensorData += "Voltage: " + String(in_voltage, 2) + " V";

  client.publish(mqtt_topic, sensorData.c_str());

  Serial.println("= - = - = - = - = - = - = - = - = - = - =");
  Serial.print(" ");
  delay(1000); 
}

void printAddress(DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i++) {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}