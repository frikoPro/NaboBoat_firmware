#include "SIM7600.h"
#include "Boat.h"
#include <ParticleSoftSerial.h>

#define BT bluetooth

// bluetooth
ParticleSoftSerial bluetooth(D2, D3);

SIM7600 *sim = SIM7600::getInstance();
Boat *boat = Boat::getInstance();
int counter = 0;

int countLinefeed;
String messagePayload = "";

void setup()
{
  pinMode(D7, OUTPUT);
  Particle.function("pubData", publishData);
  Particle.function("initSim", initSim);
  Particle.function("subMqtt", subMqtt);
  Particle.function("changeBaud", changeBaud);

  Serial.begin(9600);
  Serial1.begin(19200);
  BT.begin(19200);

  // sim->getCords();
}

int subMqtt(String data)
{
  sim->subData();
  return 1;
}

int publishData(String command)
{
  String status = boat->getStatus() ? "true" : "false";
  String longitude = boat->getLatitude();
  String latitude = boat->getLongitude();

  sim->publishData("{\"latitude\": " + latitude + ", \"longitude\": " + longitude + ", \"unlock\": " + status + "}");
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

  String incomingMessage = "";
  String mqttStringCheck = "+CMQTTRXSTART: ";
  int countChar = 0;
  if (Serial1.available() > 0)
  {
    Serial.print(":");
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

  if (sim->getMqttStatus())
  {
    sim->readMqttMessage();
  }
  else
  {
    checkIO();

    if (counter % 1000 == 0)
    {
      if (sim->checkIfPinRequired())
      {
        Serial.println("\nSIM PIN required, starting initialization");
        delay(1000);
        sim->initSim();
      }

      digitalWrite(D7, !digitalRead(D7));
      if (counter % 60000 == 0)
      {
        sim->getCords();
        publishData("ok");
      }
    }
  }
  counter++;
}