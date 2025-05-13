#include "SystemI.h"
#include <iostream>
#include <algorithm>

// --- MPK_I ---
LineList MPK_I::getLines(const Ice::Current &current) { return lines; }
void MPK_I::addStop(shared_ptr<TramStopPrx> tramStop)
{
    StopInfo stopInfo;
    stopInfo.stop = tramStop;
    stops.push_back(stopInfo);
}
void MPK_I::addLine(shared_ptr<LinePrx> line, const Ice::Current &current) { lines.push_back(line); }
void MPK_I::registerDepo(shared_ptr<DepoPrx> depo, const Ice::Current &current)
{
    cout << "NEW DEPOOO NAME::::: " << depo->getName() << endl;
    DepoInfo depoInfo;
    depoInfo.stop = depo;
    depoInfo.name = depo->getName();
    depos.push_back(depoInfo);
}
void MPK_I::unregisterDepo(shared_ptr<DepoPrx> depo, const Ice::Current &current)
{
    auto it = std::find_if(depos.begin(), depos.end(), [&depo](const DepoInfo &d)
                           { return d.stop->getName() == depo->getName(); });

    if (it != depos.end())
    {
        cout << "DELETING DEPO WITH NAME::: " << depo->getName() << endl;
        depos.erase(it);
    }
}

shared_ptr<TramStopPrx> MPK_I::getTramStop(string name, const Ice::Current &current)
{
    auto it = std::find_if(stops.begin(), stops.end(), [&name](const StopInfo &s)
                           { return s.stop->getName() == name; });

    return (it != stops.end()) ? it->stop : nullptr;
}

shared_ptr<DepoPrx> MPK_I::getDepo(string name, const Ice::Current &current)
{
    auto it = std::find_if(depos.begin(), depos.end(), [&name](const DepoInfo &d)
                           { return d.stop->getName() == name; });

    return (it != depos.end()) ? it->stop : nullptr;
}

DepoList MPK_I::getDepos(const Ice::Current &current) { return depos; }
void MPK_I::registerLineFactory(shared_ptr<LineFactoryPrx> lf, const Ice::Current &current)
{
    if (std::find(lineFactories.begin(), lineFactories.end(), lf) == lineFactories.end())
    {
        lineFactories.push_back(lf);
        std::cout << "L_FABRIC REGISTERED" << std::endl;
    }
}
void MPK_I::unregisterLineFactory(shared_ptr<LineFactoryPrx> lf, const Ice::Current &current)
{
    auto it = std::find(lineFactories.begin(), lineFactories.end(), lf);
    if (it != lineFactories.end())
    {
        lineFactories.erase(it);
        std::cout << "L_FABRIC UNREGISTERED" << std::endl;
    }
}
void MPK_I::registerStopFactory(shared_ptr<StopFactoryPrx> lf, const Ice::Current &current)
{
    if (std::find(stopFactories.begin(), stopFactories.end(), lf) == stopFactories.end())
    {
        stopFactories.push_back(lf);
        std::cout << "S_FABRIC REGISTERED" << std::endl;
    }
}
void MPK_I::unregisterStopFactory(shared_ptr<StopFactoryPrx> lf, const Ice::Current &current)
{
    auto it = std::find(stopFactories.begin(), stopFactories.end(), lf);
    if (it != stopFactories.end())
    {
        stopFactories.erase(it);
        std::cout << "S_FABRIC UNREGISTERED" << std::endl;
    }
}

// --- TramStopI ---
TramStopI::TramStopI(string name) { this->name = name; }
void TramStopI::addLine(shared_ptr<LinePrx> line) { lines.push_back(line); }
string TramStopI::getName(const Ice::Current &current) { return name; }
TramList TramStopI::getNextTrams(int howMany, const Ice::Current &current)
{
    TramList nextTrams;
    size_t count = std::min(static_cast<size_t>(howMany), comingTrams.size());
    nextTrams.reserve(count);

    std::copy_n(comingTrams.begin(), count, std::back_inserter(nextTrams));
    return nextTrams;
}

void TramStopI::RegisterPassenger(shared_ptr<PassengerPrx> passenger, const Ice::Current &current)
{
    passengers.push_back(passenger);
    cout << "Pasazer zasubskrybowal przystanek: " << name << endl;
    cout << "Liczba zasubskrybowanych pasażerów: " << passengers.size() << endl;
}
void TramStopI::UnregisterPassenger(shared_ptr<PassengerPrx> passenger, const Ice::Current &current)
{
    auto it = std::find_if(passengers.begin(), passengers.end(), [&passenger](const auto &p)
                           { return p->ice_getIdentity() == passenger->ice_getIdentity(); });

    if (it != passengers.end())
    {
        passengers.erase(it);
        cout << "Pasazer odsubskrybowal przystanek: " << name << endl;
    }
}
void TramStopI::removeComingTram(shared_ptr<TramPrx> tram, const Ice::Current &current)
{
    for (auto it = comingTrams.begin(); it != comingTrams.end(); ++it)
    {
        if (it->tram->ice_getIdentity() == tram->ice_getIdentity())
        {
            comingTrams.erase(it);
            cout << "Tramwaj o numerze: " << tram->getStockNumber() << " deleted z przystanku " << name << endl;
            return;
        }
    }
}
void TramStopI::UpdateTramInfo(shared_ptr<TramPrx> tram, SIP::Time time, const Ice::Current &current)
{
    TramInfo tramInfo;
    tramInfo.tram = tram;
    tramInfo.time = time;

    auto insertPosition = comingTrams.begin();
    while (insertPosition != comingTrams.end())
    {
        if (time.hour < insertPosition->time.hour ||
            (time.hour == insertPosition->time.hour && time.minute < insertPosition->time.minute))
        {
            break;
        }
        ++insertPosition;
    }

    comingTrams.insert(insertPosition, tramInfo);
}
void TramStopI::addCurrentTram(shared_ptr<TramPrx> tram, const Ice::Current &current)
{
    TramInfo tramInfo;
    tramInfo.tram = tram;
    currentTrams.push_back(tramInfo);
    string header = "Tramwaje na przystanku " + name;
    cout << header << endl;
    cout << "Liczba zasubskrybowanych pasażerów: " << passengers.size() << endl;
    for (const auto &passenger : passengers)
    {
        passenger->notifyPassenger(header, Ice::Context());
    }
    for (auto it = currentTrams.begin(); it != currentTrams.end(); ++it)
    {
        string info = "Tramwaj: " + it->tram->getStockNumber();
        cout << info << endl;
        for (const auto &passenger : passengers)
        {
            passenger->notifyPassenger(info, Ice::Context());
        }
    }
}
void TramStopI::removeCurrentTram(shared_ptr<TramPrx> tram, const Ice::Current &current)
{
    auto it = std::find_if(currentTrams.begin(), currentTrams.end(), [&tram](const TramInfo &t)
                           { return t.tram->ice_getIdentity() == tram->ice_getIdentity(); });

    if (it != currentTrams.end())
    {
        currentTrams.erase(it);
    }
}

// --- LineI ---
LineI::LineI(string name) { this->name = name; }
TramList LineI::getTrams(const Ice::Current &current) { return trams; }
StopList LineI::getStops(const Ice::Current &current) { return stops; }
string LineI::getName(const Ice::Current &current) { return name; }
void LineI::registerTram(shared_ptr<TramPrx> tram, const Ice::Current &current)
{
    TramInfo tramInfo;
    tramInfo.tram = tram;
    trams.push_back(tramInfo);
    cout << "Nowy tramwaj o numerze: " << tram->getStockNumber() << " zostal dodany" << endl;
}
void LineI::unregisterTram(shared_ptr<TramPrx> tram, const Ice::Current &current)
{
    auto it = std::find_if(trams.begin(), trams.end(), [&tram](const TramInfo &t)
                           { return t.tram->getStockNumber() == tram->getStockNumber(); });
    if (it != trams.end())
    {
        cout << "Tramwaj o numerze: " << tram->getStockNumber() << " zostal usuniety z linii" << endl;
        trams.erase(it);
    }
    for (const auto &stop : stops)
    {
        stop.stop->removeCurrentTram(tram, Ice::Context());
    }
    for (const auto &stop : stops)
    {
        try
        {
            stop.stop->removeComingTram(tram, Ice::Context());
        }
        catch (const Ice::ObjectNotExistException &e)
        {
            cout << "ERR: podczas usuwania tramwaju z przystanku: " << stop.stop->getName() << endl;
            return;
        }
    }
}
void LineI::setStops(StopList sl, const Ice::Current &current) { stops = sl; }

// --- DepoI ---
DepoI::DepoI(string name) { this->name = name; }
void DepoI::TramOnline(shared_ptr<TramPrx> tram, const Ice::Current &current)
{
    if (tram)
    {
        tram->setStatus(SIP::TramStatus::ONLINE, Ice::Context());
        cout << "Tramwaj " << tram->getStockNumber() << " wyjechal z zajezdni" << endl;
    }
    else
    {
        cout << "Dany Tramwaj nie istnieje" << endl;
    }
}
void DepoI::TramOffline(shared_ptr<TramPrx> tram, const Ice::Current &current)
{
    if (!tram)
    {
        cout << "Dany Tramwaj nie istnieje" << endl;
        return;
    }
    tram->setStatus(SIP::TramStatus::OFFLINE, Ice::Context());
    cout << "Tramwaj " << tram->getStockNumber() << " zjechal do zajezdni" << endl;
    auto it = std::find_if(trams.begin(), trams.end(), [&tram](const TramInfo &t)
                           { return t.tram->ice_getIdentity() == tram->ice_getIdentity(); });
    if (it != trams.end())
    {
        trams.erase(it);
    }
}
string DepoI::getName(const Ice::Current &current) { return name; }
void DepoI::registerTram(shared_ptr<TramPrx> tram, const Ice::Current &current)
{
    TramInfo tramInfo;
    tramInfo.tram = tram;
    tramInfo.tram->setStatus(SIP::TramStatus::WAITONLINE, Ice::Context());
    trams.push_back(tramInfo);
    cout << "Zajezdnia zarejestrowala tramwaj o numerze: " << tram->getStockNumber() << endl;
}
void DepoI::unregisterTram(shared_ptr<TramPrx> tram, const Ice::Current &current)
{
    TramInfo tramInfo;
    tramInfo.tram = tram;
    tramInfo.tram->setStatus(SIP::TramStatus::WAITOFFLINE, Ice::Context());
}
TramList DepoI::getTrams(const Ice::Current &current) { return trams; }

// --- LineFactoryI ---
LineFactoryI::LineFactoryI(Ice::ObjectAdapterPtr adapter) : adapter(adapter) {}
shared_ptr<LinePrx> LineFactoryI::createLine(string name, const Ice::Current &current)
{
    auto newLine = make_shared<LineI>(name);
    linesCreated++;
    auto linePrx = Ice::uncheckedCast<SIP::LinePrx>(adapter->addWithUUID(newLine));
    return linePrx;
}
double LineFactoryI::getLoad(const Ice::Current &current) { return static_cast<double>(linesCreated); }

// --- StopFactoryI ---
StopFactoryI::StopFactoryI(Ice::ObjectAdapterPtr adapter) : adapter(adapter) {}
shared_ptr<TramStopPrx> StopFactoryI::createStop(string name, const Ice::Current &current)
{
    auto newStop = make_shared<TramStopI>(name);
    stopsCreated++;
    auto stopPrx = Ice::uncheckedCast<SIP::TramStopPrx>(adapter->addWithUUID(newStop));
    return stopPrx;
}
double StopFactoryI::getLoad(const Ice::Current &current) { return static_cast<double>(stopsCreated); }