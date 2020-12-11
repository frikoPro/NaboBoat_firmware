#include "Boat.h"

Boat *Boat::instance = nullptr;

Boat *Boat::getInstance()
{
    if (!instance)
    {
        instance = new Boat();
    }
    return instance;
}

void Boat::deleteInstance()
{
    if (instance)
    {
        delete instance;
        instance = nullptr;
    }
}

Boat::Boat() : isUnlocked(false), latitude("none"), longitude("none") {}

bool Boat::getStatus()
{
    return isUnlocked;
}

void Boat::setStatus(bool status)
{
    isUnlocked = status;
}

String Boat::getLongitude()
{
    return longitude;
}

void Boat::setLongitude(String newVal)
{
    longitude = newVal;
}

String Boat::getLatitude()
{
    return latitude;
}

void Boat::setLatitude(String newVal)
{
    latitude = newVal;
}
