#include "TramI.h"
#include <iostream>
#include <algorithm>

TramI::TramI(string stockNumber)
{
    this->stockNumber = stockNumber;
    this->status = SIP::TramStatus::OFFLINE;
}

void TramI::addTramStop(const struct StopInfo stopInfo)
{
    stopList.push_back(stopInfo);
}

void TramI::setProxy(shared_ptr<TramPrx> prx)
{
    selfPrx = prx;
}

void TramI::goNext()
{
    if (!line)
    {
        return;
    }

    StopList stops = line->getStops();
    size_t totalStops = stops.size();
    bool foundCurrentStop = false;
    size_t nextStopIndex = 0;
    for (size_t i = 0; i < totalStops; ++i)
    {
        if (currentStop->getName() == stops[i].stop->getName())
        {
            foundCurrentStop = true;
            nextStopIndex = (i + 1) % totalStops;
            break;
        }
    }

    if (!foundCurrentStop)
    {
        return;
    }
    currentStop->removeCurrentTram(selfPrx);
    currentStop = stops[nextStopIndex].stop;
    currentStop->addCurrentTram(selfPrx);
    string notification = "Tramwaj " + stockNumber + " dojechal do " + currentStop->getName();
    for (auto& passenger : passengers)
    {
        passenger->notifyPassenger(notification, Ice::Context());
    }
}

void TramI::setLine(shared_ptr<LinePrx> line, const Ice::Current &current)
{
    this->line = line;
    this->currentStop = this->line->getStops().at(0).stop;
}

shared_ptr<LinePrx> TramI::getLine(const Ice::Current &current)
{
    return line;
}

shared_ptr<TramStopPrx> TramI::getLocation(const Ice::Current &current)
{
    return currentStop;
}


StopList TramI::getNextStops(int howMany, const Ice::Current &current)
{
    StopList nextStops;
    StopList allStops = line->getStops();
    int currentStopIndex = -1;
        for (size_t i = 0; i < allStops.size(); ++i)
    {
        if (allStops[i].stop->getName() == currentStop->getName())
        {
            currentStopIndex = i;
            break;
        }
    }
    if (currentStopIndex != -1 && currentStopIndex + 1 < allStops.size())
    {
        size_t stopsToAdd = std::min(static_cast<size_t>(howMany), allStops.size() - (currentStopIndex + 1));
        for (size_t i = 0; i < stopsToAdd; ++i)
        {
            nextStops.push_back(allStops[currentStopIndex + 1 + i]);
        }
                if (nextStops.size() < static_cast<size_t>(howMany))
        {
            for (int i = currentStopIndex - 1; i >= 0 && nextStops.size() < howMany; --i)
            {
                nextStops.push_back(allStops[i]);
            }
        }
    }
    else
    {
        size_t count = 0;
        for (size_t i = 0; i < allStops.size() - 1 && count < howMany; ++i, ++count)
        {
            nextStops.push_back(allStops[i]);
        }
    }
    return nextStops;
}


void TramI::RegisterPassenger(shared_ptr<PassengerPrx> passenger, const Ice::Current &current)
{
    cout << "Uzytkownik subskrybuje" << endl;
    passengers.push_back(passenger);
}

void TramI::UnregisterPassenger(shared_ptr<PassengerPrx> passenger, const Ice::Current &current)
{
    for (int index = 0; index < passengers.size(); index++)
    {
        if (passengers.at(index)->ice_getIdentity() == passenger->ice_getIdentity())
        {
            cout << "Uzytkownik zakonczyl subskrypcje" << endl;
            passengers.erase(passengers.begin() + index);
            break;
        }
    }
}

string TramI::getStockNumber(const Ice::Current &current)
{
    return stockNumber;
}

TramStatus TramI::getStatus(const Ice::Current &current)
{
    return status;
}

void TramI::setStatus(TramStatus status, const Ice::Current &current)
{
    this->status = status;
}