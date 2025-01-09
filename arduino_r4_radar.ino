// Libraries

// NTP
#include  <ezTime.h>

// Wifi connection
#include  <WiFi.h>

// Sending HTTP packets
#include  <ArduinoHttpClient.h>  

// Dedicated file containing my sensitive data (SSID, Wifi key, IP address of Splunk server, and authentication token to this server)
#include  "arduino_secrets.h"

// Wifi parameters retrieved from the arduino_secrets.h file
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

// Splunk connection parameters retrieved from arduino_secrets.h
char splunkindexer[] = SPLUNK_IDX;
char collectorToken[] = SPLUNK_TOK;
int port = 8088;

// Initialize the variables we'll need later on

// For Wifi connection
WiFiClient wifi;
int status = WL_IDLE_STATUS;

// For NTP
Timezone France;
// For the Splunk connection
HttpClient client = HttpClient(wifi, splunkindexer, port);

// For pin and sensor status
int sensorPin = 2;
int sensorState = LOW;
int sensorValue = 0;

// For information to be sent to Splunk
String date;
String eventData;
String host;

// Function used for sending to Splunk
void  splunkpost(String collectorToken,String PostData, String Host)
{
  // Display data to be sent
  Serial.println(PostData);

  // Inclusion of events to be sent in the HTTP packet
  String postData = "{ \"event\": \"" + PostData + "\"}";

  // Token authentication
  String tokenValue="Splunk " + collectorToken;

  // Client initialization
  client.beginRequest();

  // Definition of the URL to contact for HTTP POST transmissions
  client.post("/services/collector/event");

  // Definition of the various HTTP packet headers
  client.sendHeader("Content-Type", "application/application/json");
  client.sendHeader("Content-Length", postData.length());
  client.sendHeader("Host", Host);
  client.sendHeader("Authorization", tokenValue);

  // Data inclusion
  client.beginBody();
  client.print(postData);

  // End of request transmission
  client.endRequest();

  // Read and display return code and response body
  int statusCode = client.responseStatusCode();
  String response = client.responseBody();
  Serial.print("POST Status code: ");
  Serial.println(statusCode);
  Serial.print("POST Response: ");
  Serial.println(response);
}

// Setup part
void  setup() {
  // Set the data rate in bits per second (baud) for data transmission on the board's serial monitor.
  Serial.begin(9600);

  // Wifi connection with serial monitor display
  while  ( status != WL_CONNECTED)  {
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
  }

  // Allocated IP address retrieval and display
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // Use last byte to set unique host name
  host="sensor"+String(ip[3]);

  // Set the NTP server to contact, synchronization frequency and display synchronization events.
  setServer("ntp.unice.fr");
  setInterval(60);
  waitForSync();
  setDebug(INFO);

  // Setting the timezone, essential for getting the right time
 France.setLocation("Europe/Paris");

  // Set sensor pin to input mode and send first data to Splunk to validate initial communication
  pinMode(sensorPin, INPUT);
  eventData="Initializing System...";
  splunkpost(collectorToken,eventData,host);

  // 5s pause to give sensor time to initialize
  delay(5000);
}

// loop part
void  loop() {
  // Recurrent NTP synchronization at set frequency
  events();

  // Read sensor value
  sensorValue = digitalRead(sensorPin);

  // If the sensor has detected motion
  if  (sensorValue == HIGH) {
    // If the sensor had not detected anything before
    if  (sensorState == LOW) {
      // Forge the data to be sent with the date in milliseconds
      eventData=France.dateTime("d-m-Y H:i:s.v") + " - Motion detected !";
      // The data is transmitted using the created function
      splunkpost(collectorToken,eventData,host);
      // We set the sensor status to detection
      sensorState = HIGH;
      // Wait 1s for start of timer
      delay(1000);
    }
  }

  // If the sensor status variable is in detection mode
  if  (sensorState == HIGH) {
    // We wait another 1s
    delay(1000);
    // We set the state to non-detection mode to take the following reading
    sensorState = LOW;
  }
}
