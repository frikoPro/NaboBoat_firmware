#include "SIM7600.h"
#include "Boat.h"
#include <ParticleSoftSerial.h>

#define BT bluetooth

// bluetooth
ParticleSoftSerial bluetooth(D2, D3);

SIM7600 *sim = SIM7600::getInstance();
Boat *boat = Boat::getInstance();
int counter = 0;

String incomingMessage = "";

void setup()
{
  WiFi.off();
  pinMode(D7, OUTPUT);
  pinMode(D4, OUTPUT);
  Particle.function("pubData", publishData);
  Particle.function("initSim", initSim);
  Particle.function("subMqtt", subMqtt);
  Particle.function("readDweet", readDweet);
  Particle.function("changeBaud", changeBaud);
  Particle.function("connectMqtt", connectMqtt);

  Serial.begin(9600);
  Serial1.begin(19200);
  BT.begin(115200);
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
  String status = boat->getStatus() ? "true" : "false";
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

void loop()
{
  if (counter % 1000 == 0)
  {
    digitalWrite(D7, !digitalRead(D7));
    if (sim->checkIfPinRequired())
    {
      Serial.println("\nSIM PIN required, starting initialization");
      sim->initSim();
    }
  }

  if (sim->getMqttStatus())
  {
    sim->readMqttMessage();
  }
  else
  {

    if (boat->getStatus())
    {
      digitalWrite(D4, HIGH);
    }
    else
    {
      digitalWrite(D4, LOW);
    }

    checkIO();
    if (counter % 60000 == 0)
    {

      sim->getCords();
      publishData("ok");
      // check if missed message
      sim->readDweet();
    }

    counter++;
  }
}