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

Boat::Boat() : isUnlocked(false) {}

bool Boat::getStatus()
{
    return isUnlocked;
}

void Boat::setStatus(bool status)
{
    isUnlocked = status;
}

Vector<String> Boat::getCords()
{
    return cords;
}

void Boat::setCords(Vector<String> newCords)
{
    cords = newCords;
}
