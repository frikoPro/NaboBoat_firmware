/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "c:/Users/fredr/Desktop/NaboBoat_firmware/src/particleDweet.ino"
#include "SIM7600.h"

// int sendAndReadResponse(String command);

void setup();
void loop();
#line 5 "c:/Users/fredr/Desktop/NaboBoat_firmware/src/particleDweet.ino"
SIM7600 *sim = SIM7600::getInstance();

int counter = 0;
int updateDweet = false;

int particlePubData(String command);
int initSim(String command);
int readDweet(String command);
int connectMqtt(String data);

void setup()
{
  pinMode(D7, OUTPUT);
  Particle.function("pubData", particlePubData);
  Particle.function("initSim", initSim);
  Particle.function("readDweet", readDweet);
  Particle.function("connectMqtt", connectMqtt);

  Serial.begin(9600);
  Serial1.begin(115200);
}

int particlePubData(String command)
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

int readDweet(String command)
{
  sim->readDweet();
  return 1;
}

int connectMqtt(String data)
{
  sim->publishData(data, "testing");
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