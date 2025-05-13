#pragma once
#include <Ice/Ice.h>
#include "MPK.h"
#include <memory>
#include <vector>
#include <string>

using namespace std;
using namespace SIP;

class MPK_I : public SIP::MPK
{
private:
    LineList lines;
    StopList stops;
    DepoList depos;
    vector<shared_ptr<LineFactoryPrx>> lineFactories;
    vector<shared_ptr<StopFactoryPrx>> stopFactories;

public:
    LineList getLines(const Ice::Current &current) override;
    void addStop(shared_ptr<TramStopPrx> tramStop);
    void addLine(shared_ptr<LinePrx> line, const Ice::Current &current) override;
    void registerDepo(shared_ptr<DepoPrx> depo, const Ice::Current &current) override;
    void unregisterDepo(shared_ptr<DepoPrx> depo, const Ice::Current &current) override;
    shared_ptr<TramStopPrx> getTramStop(string name, const Ice::Current &current) override;
    shared_ptr<DepoPrx> getDepo(string name, const Ice::Current &current) override;
    DepoList getDepos(const Ice::Current &current) override;
    void registerLineFactory(shared_ptr<LineFactoryPrx> lf, const Ice::Current &current) override;
    void unregisterLineFactory(shared_ptr<LineFactoryPrx> lf, const Ice::Current &current) override;
    void registerStopFactory(shared_ptr<StopFactoryPrx> lf, const Ice::Current &current) override;
    void unregisterStopFactory(shared_ptr<StopFactoryPrx> lf, const Ice::Current &current) override;
};

class TramStopI : public SIP::TramStop
{
private:
    string name;
    LineList lines;
    vector<shared_ptr<PassengerPrx>> passengers;
    TramList comingTrams;
    TramList currentTrams;

public:
    TramStopI(string name);
    void addLine(shared_ptr<LinePrx> line);
    string getName(const Ice::Current &current) override;
    TramList getNextTrams(int howMany, const Ice::Current &current) override;
    void RegisterPassenger(shared_ptr<PassengerPrx> passenger, const Ice::Current &current) override;
    void UnregisterPassenger(shared_ptr<PassengerPrx> passenger, const Ice::Current &current) override;
    void UpdateTramInfo(shared_ptr<TramPrx> tram, SIP::Time time, const Ice::Current &current) override;
    void addCurrentTram(shared_ptr<TramPrx> tram, const Ice::Current &current) override;
    void removeCurrentTram(shared_ptr<TramPrx> tram, const Ice::Current &current) override;
    void removeComingTram(shared_ptr<TramPrx> tram, const Ice::Current &current) override;
};

class LineI : public SIP::Line
{
private:
    TramList trams;
    StopList stops;
    string name;

public:
    LineI(string name);
    TramList getTrams(const Ice::Current &current) override;
    StopList getStops(const Ice::Current &current) override;
    string getName(const Ice::Current &current) override;
    void registerTram(shared_ptr<TramPrx> tram, const Ice::Current &current) override;
    void unregisterTram(shared_ptr<TramPrx> tram, const Ice::Current &current) override;
    void setStops(StopList sl, const Ice::Current &current) override;
};

class DepoI : public SIP::Depo
{
private:
    string name;
    TramList trams;

public:
    DepoI(string name);
    void TramOnline(shared_ptr<TramPrx> tram, const Ice::Current &current) override;
    void TramOffline(shared_ptr<TramPrx> tram, const Ice::Current &current) override;
    string getName(const Ice::Current &current) override;
    void registerTram(shared_ptr<TramPrx> tram, const Ice::Current &current) override;
    void unregisterTram(shared_ptr<TramPrx> tram, const Ice::Current &current) override;
    TramList getTrams(const Ice::Current &current) override;
};

class LineFactoryI : public SIP::LineFactory
{
private:
    int linesCreated = 0;
    Ice::ObjectAdapterPtr adapter;

public:
    LineFactoryI(Ice::ObjectAdapterPtr adapter);
    shared_ptr<LinePrx> createLine(string name, const Ice::Current &current) override;
    double getLoad(const Ice::Current &current) override;
};

class StopFactoryI : public SIP::StopFactory
{
private:
    int stopsCreated = 0;
    Ice::ObjectAdapterPtr adapter;

public:
    StopFactoryI(Ice::ObjectAdapterPtr adapter);
    shared_ptr<TramStopPrx> createStop(string name, const Ice::Current &current) override;
    double getLoad(const Ice::Current &current) override;
};