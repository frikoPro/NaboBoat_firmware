#include "SIM7600.h"
#include "Boat.h"
#include <ParticleSoftSerial.h>

//bluetooth
ParticleSoftSerial BT(D2, D3);

SIM7600 *sim = SIM7600::getInstance();
Boat *boat = Boat::getInstance();
int counter = 0;

// incoming messages may be read from over multiple loops, keep it global
String incomingMessage = "";

void setup()
{
  // D7 status light
  pinMode(D7, OUTPUT);
  // Relay for opening boat
  pinMode(D4, OUTPUT);

  //cloud functions
  Particle.function("pubData", publishData);
  Particle.function("initSim", initSim);
  Particle.function("subMqtt", subMqtt);
  Particle.function("readDweet", readDweet);
  Particle.function("changeBaud", changeBaud);
  Particle.function("connectMqtt", connectMqtt);

  Serial.begin(9600);
  Serial1.begin(19200);
  BT.begin(19200);

  // check if GSM-module needs pincode
  if (sim->checkResponse("AT+CPIN?", "+CPIN: SIM PIN"))
  {
    // BT.println("\nSIM PIN required, starting initialization");
    Serial.println("\nSIM PIN required, starting initialization");
    sim->initSim();
  }
}

int readDweet(String para)
{
  sim->readDweet();
  return 1;
}

int connectMqtt(String para)
{
  sim->connectMqtt();
  return 1;
}

int subMqtt(String data)
{
  sim->subData();
  return 1;
}

int publishData(String command)
{
  String status = boat->getStatus() ? "1" : "0";
  String latitude = "\"" + boat->getLatitude() + "\"";
  String longitude = "\"" + boat->getLongitude() + "\"";
  sim->publishData("{\"latitude\": " + latitude + ", \"longitude\": " + longitude + ", \"unlock\": " + status + "}", "data");
  return 1;
}

int initSim(String command)
{
  sim->initSim();
  return 1;
}

int changeBaud(String command)
{
  sim->readResponse("AT+IPR=" + command);
  Serial1.begin(command.toInt());
  return 1;
}

//check input from Bluetooth and USB, read output from GSM-module
void checkIO()
{
  if (Serial.available() > 0)
  {
    Serial.print(">");
    delay(100);
    while (Serial.available())
    {
      char ch = Serial.read();
      Serial.print(ch);
      Serial1.print(ch);
    }
  }

  String mqttStringCheck = "+CMQTTRXSTART: ";
  int countChar = 0;
  if (Serial1.available() > 0)
  {
    delay(10);
    while (Serial1.available())
    {
      char ch = Serial1.read();
      if (ch)
      {
        Serial.print(ch);
        BT.print(ch);
        if (ch == mqttStringCheck.charAt(countChar))
        {
          incomingMessage += ch;
          countChar++;
          if (mqttStringCheck.equals(incomingMessage))
          {
            sim->setMqttStatus(true);
            incomingMessage = "";
            return;
          }
        }
        else
        {
          incomingMessage = "";
          countChar = 0;
        }
      }
    }
  }
}

void checkBluetooth()
{
  if (BT.available() > 0)
  {
    BT.print(">");
    delay(100);
    while (BT.available())
    {
      char ch = BT.read();
      Serial1.print(ch);
    }
  }
}

void loop()
{
  //check input from bluetooth and USB, and check output for sim
  checkBluetooth();
  checkIO();
  // blink D7 blue light for status on device
  if (counter % 1000 == 0)
  {
    digitalWrite(D7, !digitalRead(D7));
  }

  // check if incomming mqtt message
  if (sim->getMqttStatus())
  {
    sim->readMqttMessage();
  }

  // check if boat is unlocked or not
  if (boat->getStatus() != digitalRead(D4))
  {
    digitalWrite(D4, boat->getStatus() ? HIGH : LOW);
  }

  if (counter % 60000 == 0)
  {
    //   //Get coords occasionally
    sim->getCords();
    //   // update occasionally state
    publishData("ok");
    //   // check if missed message during publish, read state from Dweet.io
    sim->readDweet();
  }
  counter++;
}