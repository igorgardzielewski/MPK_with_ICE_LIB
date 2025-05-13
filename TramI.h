#pragma once
#include <Ice/Ice.h>
#include "MPK.h"
#include <memory>
#include <vector>
#include <string>

using namespace std;
using namespace SIP;

class TramI : public SIP::Tram
{
private:
    shared_ptr<LinePrx> line;
    shared_ptr<TramPrx> selfPrx;
    TramStatus status;
    string stockNumber;
    shared_ptr<TramStopPrx> currentStop;
    StopList stopList;
    vector<shared_ptr<PassengerPrx>> passengers;

public:
    TramI(string stockNumber);
    void addTramStop(const struct StopInfo stopInfo);
    void setProxy(shared_ptr<TramPrx> prx);
    void goNext();
    void setLine(shared_ptr<LinePrx> line, const Ice::Current &current) override;
    shared_ptr<LinePrx> getLine(const Ice::Current &current) override;
    shared_ptr<TramStopPrx> getLocation(const Ice::Current &current) override;
    StopList getNextStops(int howMany, const Ice::Current &current) override;
    void RegisterPassenger(shared_ptr<PassengerPrx> passenger, const Ice::Current &current) override;
    void UnregisterPassenger(shared_ptr<PassengerPrx> passenger, const Ice::Current &current) override;
    string getStockNumber(const Ice::Current &current) override;
    TramStatus getStatus(const Ice::Current &current) override;
    void setStatus(TramStatus status, const Ice::Current &current) override;
};