
// Import required libraries
#include <WiFi.h>
#include "dups.h"
#include "secret.h"
#include "PubSubClient.h"

Inverter invctl;

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

void callback(char* topic, byte* payload, unsigned int length) {
  char pl[10] = {0,};
  int ipl = 0 ;
  //Serial.print("Message arrived [");
  /*Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }*/
  if (NULL != topic && NULL != payload && length > 0){
    strncpy (pl, (char *)payload, length);
    ipl = atoi (pl);
    if (0 == strcmp (topic, "dups/ctl/cmd")){
      //Serial.println(pl);
      invctl.sendCommand (ipl, true);
    }else if (0 == strcmp (topic, "dups/ctl/rl")){
      ipl = (((ipl-180)/10)+1);
      if (invctl.m_regulatorLevel != ipl){
        invctl.setRegulatorLevel (ipl) ;
      }
    }else if (0 == strcmp (topic, "dups/ctl/fcs")){
      invctl.setCutOffTime (ipl);
    }else if (0 == strcmp (topic, "dups/ctl/apl")){
      invctl.setApplianceMode (ipl);
    }else if (0 == strcmp (topic, "dups/ctl/ups")){
      invctl.setUPSMode (ipl);
    }else if (0 == strcmp (topic, "dups/ctl/tch")){
      invctl.setTurboCharging (ipl);
    }
  }
  //Serial.println();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "DUPS-dcbcca69-9c41-4a4d-ad38-eace60ca34db";
    //clientId += String(random(0xffff), HEX);
    //Serial.println("Client ID "+clientId);
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqtt_usr, mqtt_pwd)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("dups/rd/mv", "0");
      client.publish("dups/rd/bv", "0");
      client.publish("dups/rd/bp", "0");
      client.publish("dups/rd/rl", "0");
      client.publish("dups/rd/fcs", "0");
      client.publish("dups/rd/apl", "0");
      client.publish("dups/rd/ups", "0");
      client.publish("dups/rd/tch", "0");
      client.publish("dups/rd/bkt", "0");
      client.publish("dups/rd/st", "0");
      client.publish("dups/rd/als", "0");
      client.publish("dups/rd/lc", "0");
      client.publish("dups/rd/lp", "0");

      // ... and resubscribe
      client.subscribe("dups/ctl/ipsw"); //Inverter Power Switch
      client.subscribe("dups/ctl/rl"); //Regulator Load Level
      client.subscribe("dups/ctl/fcs"); //forced cuto off
      client.subscribe("dups/ctl/cmd"); //Run commands directly
      client.subscribe("dups/ctl/apl"); //Enable/Disable Appliance Mode
      client.subscribe("dups/ctl/ups"); //Enable/Disable UPS Mode
      client.subscribe("dups/ctl/tch"); //Enable/Disable Turbo Charging
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup()
{
  // Start Serial
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Print the IP address
  Serial.println(WiFi.localIP());
  invctl.attach ();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (invctl.refresh () && client.connected()){
    snprintf (msg, 50, "%d", invctl.m_backupMode);
    client.publish("dups/rd/bkm", msg);
    snprintf (msg, 50, "%.2f", invctl.m_mainsVoltage);
    client.publish("dups/rd/mv", msg);
    snprintf (msg, 50, "%.2f", invctl.m_batteryVoltage);
    client.publish("dups/rd/bv", msg);
    if (invctl.m_backupMode){
      snprintf (msg, 50, "%d", invctl.m_dischargingPercent);
    }
    else{
      snprintf (msg, 50, "%d", invctl.m_chargingPercent);
    }
    client.publish("dups/rd/bp", msg);
    snprintf (msg, 50, "%d", ((invctl.m_regulatorLevel-1)*10)+180);
    client.publish("dups/rd/rl", msg);
    snprintf (msg, 50, "%d", invctl.m_timeToResume);
    client.publish("dups/rd/fcs", msg);
    snprintf (msg, 50, "%d", invctl.m_applianceMode);
    client.publish("dups/rd/apl", msg);
    snprintf (msg, 50, "%d", invctl.m_UPSMode);
    client.publish("dups/rd/ups", msg);
    snprintf (msg, 50, "%d", invctl.m_turboCharge);
    client.publish("dups/rd/tch", msg);
    snprintf (msg, 50, "%.2f", invctl.m_backupTime);
    client.publish("dups/rd/bkt", msg);
    snprintf (msg, 50, "%.2f", invctl.m_sysTemperature);
    client.publish("dups/rd/st", msg);
    snprintf (msg, 50, "%d", invctl.m_alarmStatus);
    client.publish("dups/rd/als", msg);
    snprintf (msg, 50, "%.2f", invctl.m_loadCurrent);
    client.publish("dups/rd/lc", msg);
    snprintf (msg, 50, "%.2f", invctl.m_loadPercent);
    client.publish("dups/rd/lp", msg);
  }

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  delay(500);

  /*long now = millis();
  if (now - lastMsg > 6000) {
    lastMsg = now;
    //++value;
    //snprintf (msg, 50, "hello world #%ld", value);
    snprintf (msg, 50, "%.2f", invctl.m_mainsVoltage);
    //Serial.print("Publish message: ");
    //Serial.println(msg);
    client.publish("dups/rd/mains", msg);
  }*/
}