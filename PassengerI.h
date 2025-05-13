#pragma once
#include <Ice/Ice.h>
#include "MPK.h"
#include <memory>
#include <string>
#include <iostream>

using namespace std;
using namespace SIP;

class PassengerI : public SIP::Passenger
{
private:
    string stopName = "";

public:
    void setTramStopName(string name);
    void updateTramInfo(shared_ptr<TramPrx> tram, StopList stops, const Ice::Current &current) override;
    void updateStopInfo(shared_ptr<TramStopPrx> tramStop, TramList tramList, const Ice::Current &current) override;
    void notifyPassenger(string info, const Ice::Current &current) override;
};