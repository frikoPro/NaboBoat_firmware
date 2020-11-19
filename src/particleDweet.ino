#include "SIM7600.h"

// int sendAndReadResponse(String command);

SIM7600 *sim = SIM7600::getInstance();

int counter = 0;
int updateDweet = false;

void setup()
{
  pinMode(D7, OUTPUT);
  Particle.function("pubData", publishData);
  Particle.function("initSim", initSim);
  Particle.function("testMqtt", connectMqtt);
  Particle.function("testSubMqtt", subMqtt);
  Particle.function("changeBaud", changeBaud);

  Serial.begin(9600);
  Serial1.begin(19200);
}

int subMqtt(String data)
{
  sim->subData();
  return 1;
}

int publishData(String command)
{
  Vector<String> cords = sim->getCords();
  if (cords.isEmpty())
    return -1;
  sim->publishData(cords.first(), "latitude");
  sim->publishData(cords.last(), "longitude");
  return 1;
}

int initSim(String command)
{
  sim->initSim();
  return 1;
}

int changeBaud(String command)
{
  Serial1.begin(command.toInt());
  return 1;
}

int connectMqtt(String data)
{
  sim->publishData(data, "testing");
  return 1;
}

void loop()
{

  if (sim->getMqttStatus())
  {
    sim->readMqttMessage();
  }
  else
  {
    sim->checkIO();

    if (counter % 1000 == 0)
    {
      if (sim->checkIfPinRequired())
      {
        Serial.println("\nSIM PIN required, starting initialization");
        delay(1000);
        sim->initSim();
      }

      digitalWrite(D7, !digitalRead(D7));
      counter = 1;
    }
    counter++;
  }
}