#include "SIM7600.h"
#include "Boat.h"

// int sendAndReadResponse(String command);

SIM7600 *sim = SIM7600::getInstance();
Boat *boat = Boat::getInstance();
int counter = 0;

void setup()
{
  pinMode(D7, OUTPUT);
  Particle.function("pubData", publishData);
  Particle.function("initSim", initSim);
  Particle.function("subMqtt", subMqtt);
  Particle.function("changeBaud", changeBaud);

  Serial.begin(9600);
  Serial1.begin(19200);

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
      if (boat->getStatus())
        Serial.println("boat is unlocked");
      if (sim->checkIfPinRequired())
      {
        Serial.println("\nSIM PIN required, starting initialization");
        delay(1000);
        sim->initSim();
      }

      digitalWrite(D7, !digitalRead(D7));
      if (counter % 10000 == 0)
        sim->getCords();
    }
  }
  counter++;
}