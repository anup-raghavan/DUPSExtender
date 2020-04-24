
// Import required libraries
#include <WiFi.h>
#include <aREST.h>
#include "vguard.h"
#include "secret.h"

// Create aREST instance
aREST rest = aREST();

// Create an instance of the server
WiFiServer server(80);

// Declare functions to be exposed to the API
String inverterControl(String command);

Inverter invctl;

void setup()
{
  // Start Serial
  Serial.begin(115200);

  rest.variable("mainsVoltage", &invctl.m_mainsVoltage);
  rest.variable("batteryVoltage", &invctl.m_batteryVoltage);
  //rest.variable("batteryPercent", &invctl.m_dischargingPercent);
  //rest.variable("chargingProgress", &invctl.m_chargingPercent);
  rest.variable("sysTemperature", &invctl.m_sysTemperature);
  rest.variable("backupTime", &invctl.m_backupTime);
  rest.variable("loadCurrent", &invctl.m_loadCurrent);
  rest.variable("holidayMode", &invctl.m_hoildayMode);
  rest.variable("inverterMode", &invctl.m_UPSMode);
  rest.variable("regulatorLevel", &invctl.m_regulatorLevel);
  rest.variable("inverterSwitch", &invctl.m_inverterSwitch);


  // Function to be exposed
  rest.function((char*)"inverter",inverterControl);

  // Give name & ID to the device (ID should be 6 characters long)
  rest.set_id("1");
  rest.set_name((char *)"esp32");

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());
  invctl.attach ();
}

void loop() {
  invctl.refresh ();
  // Handle REST calls
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  Serial.println ("Connected "+client.remoteIP().toString());
  while(!client.available()){
    delay(1);
  }
  rest.handle(client);
  Serial.println("Disconnected\r\n");
}

String inverterControl(String command){
  invctl.sendCommand(command.toInt(), true);
  //Serial.println ("backupControl Command => " + command);
  return "{\"status\":\"ok\"}";
}