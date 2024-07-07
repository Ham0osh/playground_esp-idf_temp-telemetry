//
// ROOM TEMP TELEMETRY
// Author: Hamish
//

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

// Configs
#include "../include/config.h"
#include <OneWire.h>
#include <Arduino.h>

// temp probe
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 2

// telemetry
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

// Set timezone string according to https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
#define TZ_INFO "PST8PDT"

// InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
#define DEVICE "TestTemp"

// Data point
Point sensor("device_status");
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// init our values
int count = 0;
int delay_val = 0;
float temperature = 0.0000;

void setup() {
  // put your setup code here, to run once:
  // pinMode(16, OUTPUT);
  sensors.begin();
  Serial.begin(115200);
  delay(10);
  Serial.println('\n');
  
  // Connect to network from config
  WiFi.begin(ssid, password);             // Connect to the network
  Serial.print("Connecting to ");
  Serial.print(ssid);
  Serial.println(" ...");

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(1000);
    Serial.print(++i); Serial.print(' ');
  }

  Serial.println('\n');
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());         // Send the IP address of the ESP8266 to the computer

  // One wire probe setup
  sensors.setResolution(12);
  Serial.print("One wire resolution set:\t");
  Serial.println(sensors.getResolution());

  // NEW STUFF
  sensor.addTag("device", DEVICE);
  sensor.addTag("SSID", WiFi.SSID());
  // Accurate time is necessary for certificate validation and writing in batches
  // For the fastest time sync find NTP servers in your area: https://www.pool.ntp.org/zone/
  // Syncing progress and the time will be printed to Serial.
  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

  // Check server connection
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }
}


void loop() {
  Serial.println();  // Newline per sweep
  sensors.requestTemperatures();
  temperature = sensors.getTempCByIndex(0);
  Serial.print("Temp: ");
  Serial.println(temperature);

  // Clear fields for reusing the point. Tags will remain untouched
  sensor.clearFields();
  sensor.addField("probe", temperature);
  // Print what are we exactly writing
  Serial.print("Writing to InfluxDB: ");
  Serial.println(sensor.toLineProtocol());
  
  // If no Wifi signal, let us know
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected!");
    return;
  }
  
  // Check write to DB
  if (!client.writePoint(sensor)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }

  delay(1000*60*6); // every 6 mins
}
