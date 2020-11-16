#include "SIM7600.h"

// int sendAndReadResponse(String command);

SIM7600 *sim = SIM7600::getInstance();

int counter = 0;
int updateDweet = false;

int particlePubData(String command);
int initSim(String command);
int readDweet(String command);

void setup()
{
  pinMode(D7, OUTPUT);
  Particle.function("pubData", particlePubData);
  Particle.function("initSim", initSim);
  Particle.function("readDweet", readDweet);

  Serial.begin(115200);
  Serial1.begin(115200);
  sim->sendAndReadResponse("AT+IPR=19200");
  Serial1.begin(19200);
}

int particlePubData(String command)
{
  Vector<String> cords = sim->getCords();
  if (cords.isEmpty())
    return -1;
  sim->postDweet(cords.first(), cords.last());
  return 1;
}

int initSim(String command)
{
  sim->initSim();
  return 1;
}

int readDweet(String command)
{
  sim->readDweet();
  Serial.println("\n\tFinish");
}

void loop()
{

  sim->checkInput();

  if (counter % 1000 == 0)
  {
    digitalWrite(D7, !digitalRead(D7));
    counter = 1;
  }
  counter++;
}