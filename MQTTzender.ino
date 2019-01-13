#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"
 
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

// Change the credentials below, so your ESP8266 connects to your router
const char* ssid = "The Promised LAN";
const char* password = "*******";

// Change the variable to your Raspberry Pi IP address, so it connects to your MQTT broker
const char* mqtt_server = "192.168.2.165";

// Initializes the espClient. You should change the espClient name if you have multiple ESPs running in your home automation system
WiFiClient espServer;
PubSubClient client(espServer);

// DHT Sensor - GPIO 5 = D1 on ESP-12E NodeMCU board
const int DHTPin = 5;

// Initialize DHT sensor.
DHT dht(DHTPin, DHTTYPE);

// Timers auxiliar variables
long now = millis();
long lastMeasure = 0;

// Don't change the function below. This functions connects your ESP8266 to your router
void setup_wifi() {
  delay(10);
  // Connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Server")) {
      Serial.println("connected");  
      // Subscribe or resubscribe to a topic
//      client.subscribe("room/lamp");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


// The callback function is what receives messages
void setup() {

  dht.begin();
  
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
//  client.setCallback(callback);

}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  if(!client.loop())
    client.connect("ESP8266Server");

  now = millis();
  // Publishes new temperature and humidity every 10 seconds
  if (now - lastMeasure > 10000) {
    lastMeasure = now;
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    float f = dht.readTemperature(true);

    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t) || isnan(f)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    // Computes temperature values in Celsius
    float hic = dht.computeHeatIndex(t, h, false);
    static char temperatureTemp[7];
    dtostrf(hic, 6, 2, temperatureTemp);
    
    // Uncomment to compute temperature values in Fahrenheit 
    // float hif = dht.computeHeatIndex(f, h);
    // static char temperatureTemp[7];
    // dtostrf(hic, 6, 2, temperatureTemp);
    
    static char humidityTemp[7];
    dtostrf(h, 6, 2, humidityTemp);

    // Publishes Temperature and Humidity values
    client.publish("room/temperature", temperatureTemp);
    client.publish("room/humidity", humidityTemp);
    
    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print(" %\t Temperature: ");
    Serial.print(t);
    Serial.print(" *C ");
    Serial.print(f);
    Serial.print(" *F\t Heat index: ");
    Serial.print(hic);
    Serial.println(" *C ");
    // Serial.print(hif);
    // Serial.println(" *F");
  }
} 
