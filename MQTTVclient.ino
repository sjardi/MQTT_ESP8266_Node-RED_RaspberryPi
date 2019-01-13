#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Stepper values
///////////////////////////////////
#include <AccelStepper.h>        //
#define motorPin1  16            //
#define motorPin2  5             //
#define motorPin3  4             //
#define motorPin4  0             //
int stepsPerRevolution = 64;     //
int degreePerRevolution = 5.625; //
int huidigePositie;              //
///////////////////////////////////
AccelStepper stepper(AccelStepper::HALF4WIRE, motorPin1, motorPin3, motorPin2, motorPin4);

// Temperature
//////////////////////////////////
float huidigeTemperatuur;       //
float desiredTemp;              //
//////////////////////////////////

// Network ESP and RSP Broker
///////////////////////////////////////////////////////////////////
const char* ssid = "The Promised LAN";                           //
const char* password = "********";                             //
// Raspberry Pi IP address, it connects to your MQTT broker      //
const char* mqtt_server = "192.168.2.165";                       //
// Initializes the espClient.                                    //
//Change the espClient name if you have multiple ESPs running    //
WiFiClient espClient;                                            //
PubSubClient client(espClient);                                  //
///////////////////////////////////////////////////////////////////


// Lock variables
//////////////////////////////
int SjardiConnected = 0;    //
int doorUnlocked;           //
int huidigSlotPositie;      //
// Lamp - LED - GPIO 4 = D2 //
const int lamp = 2;         //
//////////////////////////////

// Connects your ESP8266 to your router
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
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


void callback(String topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
  // Listens to room/lock and checks what the message is
  if (topic == "room/lock") {
    Serial.print("Changing Doorlock to ");
    if (messageTemp == "open") {
      digitalWrite(lamp, HIGH);
      Serial.print("open");
      doorUnlocked = 1;
    }
    else if (messageTemp == "closed") {
      digitalWrite(lamp, LOW);
      Serial.print("closed");
      doorUnlocked = 0;
    }

    // Topic: room/GewensteTemp
  } else if (topic == "room/GewensteTemp") {
    desiredTemp = messageTemp.toFloat();
    setRoomTemp();

    // Topic: room/temperature
  } else if (topic ==  "room/temperature") {
    huidigeTemperatuur = messageTemp.toFloat();
    Serial.print("Huidge kamer temperatuur = " );    Serial.println(messageTemp);
    setRoomTemp();

    Serial.println();
  }
  else if (topic == "room/SjardiConnected"); {
    Serial.print("sjardiconnected : " ); Serial.print(messageTemp); Serial.print(" Is door unlocked: " ); Serial.println(doorUnlocked);
    SjardiConnected = messageTemp.toInt();
  }
   presenceDetection();
}

void presenceDetection() {
  if (SjardiConnected == 1) {
    Serial.print("Sjardi wel aanwezig = "); Serial.println(SjardiConnected);
    Serial.print(" Door unlocked = ");Serial.print(doorUnlocked);Serial.print(" Huidig Slot Positie = "); Serial.println(huidigSlotPositie);
    if ((doorUnlocked == 1) && (stepper.distanceToGo() == 0) && (huidigSlotPositie != 120 )) {

      Serial.print("DEUR GAAT OPEN " ); Serial.print("STAPPEN MOTOR NAAR POSITIE "); Serial.println(degToSteps(120));
      stepper.moveTo(degToSteps(120));
      huidigSlotPositie = 120;

    } else if (stepper.distanceToGo() == 0 && doorUnlocked == 0 && (huidigSlotPositie != -120)) {
      Serial.print("DEUR GAAT OP SLOT " ); Serial.print("STAPPEN MOTOR NAAR POSITIE "); Serial.println(degToSteps(-120));
      stepper.moveTo(degToSteps(-120));
      huidigSlotPositie = -120;
    }

  } else if (SjardiConnected == 0) {
    Serial.println("Sjard Niet aanwezig.");
  }
}

void setRoomTemp() {
  Serial.println("setRoomTemp function called. ");
  Serial.print("  De gewenste Kamer Temperatuur is: "); Serial.println(desiredTemp);
  Serial.print("De HUIDIGE Kamer Temperatuur is: ");  Serial.println(huidigeTemperatuur);
  Serial.print("distance to go : " );  Serial.println(stepper.distanceToGo());
  if (stepper.distanceToGo() == 0) {
    Serial.print("  Huidige Positie namens stepper : " );    Serial.println(stepper.currentPosition());
    if (desiredTemp > huidigeTemperatuur) {
      if ((desiredTemp >= (huidigeTemperatuur + 5)) && huidigePositie != 180) {
        Serial.print("Verwarming gaat helemaal open"); Serial.print(" MOTOR NAAR POSITIE "); Serial.println(degToSteps(180));
        // zet verwarming volledig open
        stepper.moveTo(degToSteps(180));
        huidigePositie = 180;
      }
      else if ((desiredTemp >= (huidigeTemperatuur + 2))&& (desiredTemp < (huidigeTemperatuur + 5)) && huidigePositie != 90) {
        Serial.print("Verwarming gaat voor 50% open"); Serial.print(" STAPPEN MOTOR NAAR POSITIE "); Serial.println(degToSteps(90));
        stepper.moveTo(degToSteps(90));
        huidigePositie = 90;
      } else if ((huidigePositie != 45) && (desiredTemp < (huidigeTemperatuur + 2))) {
        Serial.print("Verwarming gaat voor 25% open"); Serial.print(" STAPPEN MOTOR NAAR POSITIE "); Serial.println(degToSteps(45));
        stepper.moveTo(degToSteps(45));
        huidigePositie = 45;


      }
    } else if(huidigePositie != 0 && huidigeTemperatuur > desiredTemp) {
      Serial.println("Gewenste temperatuur is lager dan de kamer temp, verwarming gaat uit");
      stepper.moveTo(0);
      huidigePositie = 0;
    }
//    stepper.run();
  }

}

// This functions reconnects your ESP8266 to your MQTT broker
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Verwarming")) {
      Serial.println("connected");
      // Subscribe or resubscribe to a topic
      // You can subscribe to more topics (to control more LEDs in this example)
      client.subscribe("room/lock");
      client.subscribe("room/temperature");
      client.subscribe("room/GewensteTemp");
      client.subscribe("room/SjardiConnected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


// Sets your mqtt broker and sets the callback function
// The callback function is what receives messages and actually controls the LEDs
void setup() {
 

  stepper.setMaxSpeed(1000.0);      // stel de maximale motorsnelheid in
  stepper.setAcceleration(100.0);   // stel de acceleratie in
  stepper.setSpeed(200);            // stel de huidige snelheid in
  Serial.print("huidige positie bij setup: " ); Serial.println(stepper.currentPosition());
  delay(10000);

  stepper.runToNewPosition(degToSteps(0));
  huidigePositie = 0;
  delay(10000);
  pinMode(lamp, OUTPUT);


  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

}
 int counter = 1;
void loop() {

  if (!client.connected()) {
    reconnect();
  }
  if (!client.loop()) {
    client.connect("ESP8266Verwarming");
  }

  if (counter == 400) {
    Serial.print("Huidige Positie volgens accelstepper: ");
    Serial.print( stepper.currentPosition());
    Serial.print("  |  Huidige Positie: ");
    Serial.print( huidigePositie);
    Serial.print(" |   Target position : " );
    Serial.println(stepper.targetPosition());
    //  delay(1000);
    counter = 0;
  }
  counter += 1;

  stepper.run();
  delay(1);
}


/*
   Rekent het aantal graden om naar het aantal stappen
   28BYJ-48 motor heeft 5,625 graden per stap
   360 graden / 5,625 = 64 stappen per omwenteling
   Voorbeeld van degToSteps(45):
   (64 / 5,625) * 45 = 512 stappen
*/
float degToSteps(float deg) {
  return (stepsPerRevolution / degreePerRevolution) * deg;
}

// Check what variable type it is with polymorpism
void types(String a) {
  Serial.println("it's a String");
}
void types(int a)   {
  Serial.println("it's an int");
}
void types(char* a) {
  Serial.println("it's a char*");
}
void types(float a) {
  Serial.println("it's a float");
}
