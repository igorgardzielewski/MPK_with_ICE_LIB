#include "PassengerI.h"

void PassengerI::setTramStopName(string name)
{
    this->stopName = name;
}

void PassengerI::updateTramInfo(shared_ptr<TramPrx> tram, StopList stops, const Ice::Current &current)
{
    cout << "Tramwaj: " << tram->getStockNumber() << endl;
    cout << "Lista kolejnych przystanków:" << endl;
    for (const auto &stop : stops)
    {
        cout << "* Przystanek: " << stop.stop->getName()
             << " planowany czas: " << stop.time.hour << ":" << stop.time.minute << endl;
    }
}

void PassengerI::updateStopInfo(shared_ptr<TramStopPrx> tramStop, TramList tramList, const Ice::Current &current)
{
    for (const auto &tramInfo : tramList)
    {
        cout << ">> UPCOMING tramwaj: " << tramInfo.tram->getStockNumber()
             << " | przewidywany czas przyjazdu: "
             << tramInfo.time.hour << ":" << tramInfo.time.minute << endl;
    }
}

void PassengerI::notifyPassenger(string info, const Ice::Current &current)
{
    cout << info << endl;
}