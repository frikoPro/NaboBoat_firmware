#include "SIM7600.h"

// int sendAndReadResponse(String command);

SIM7600 *sim = SIM7600::getInstance();

int counter = 0;
int updateDweet = false;

int particlePubData(String command);

void setup()
{
  pinMode(D7, OUTPUT);
  Particle.function("pubData", particlePubData);

  Serial.begin(115200);

  sim->initSim();
}

int particlePubData(String command)
{
  Vector<String> cords = sim->getCords();
  sim->postDweet("asdfsd", "asdfgsegw");
  return 1;
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
