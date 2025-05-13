#include <Ice/Ice.h>
#include "MPK.h"
#include <iostream>
#include <memory>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include "PassengerI.h"

using namespace std;
using namespace SIP;
struct ConfigData
{
    string passengerAddress;
    string serverAddress;
    string serverPort;
    string serverName;
};
bool loadConfig(const string &configFilePath, ConfigData &config)
{
    ifstream configFile(configFilePath);
    if (!configFile.is_open())
    {
        cout << "ERR: OTWIERANIE PLIKU CONFIG UNSUCCED: " << configFilePath << endl;
        return false;
    }

    string line;
    while (getline(configFile, line))
    {
        istringstream iss(line);
        string token, value;
        if (getline(iss, token, '=') && getline(iss, value))
        {
            token.erase(remove_if(token.begin(), token.end(), ::isspace), token.end());
            value.erase(remove_if(value.begin(), value.end(), ::isspace), value.end());
            if (token == "self.addr")
            {
                config.passengerAddress = value;
            }
            else if (token == "server.addr")
            {
                config.serverAddress = value;
            }
            else if (token == "server.port")
            {
                config.serverPort = value;
            }
            else if (token == "server.name")
            {
                config.serverName = value;
            }
        }
    }
    configFile.close();
    if (config.serverAddress.empty() || config.serverPort.empty() || config.serverName.empty())
    {
        cout << "ERR: Brakujące wymagane parametry w pliku konfiguracyjnym: " << configFilePath << endl;
        cout << "INFO: Wymagane parametry: server.addr, server.port, server.name" << endl;
        return false;
    }

    return true;
}
StopList getAllUniqueStops(const LineList &transportLines)
{
    StopList uniqueStations;

    for (size_t lineIdx = 0; lineIdx < transportLines.size(); ++lineIdx)
    {
        StopList stationsOnLine = transportLines.at(lineIdx)->getStops();
        for (size_t stationIdx = 0; stationIdx < stationsOnLine.size(); stationIdx++)
        {
            shared_ptr<TramStopPrx> currentStation = stationsOnLine.at(stationIdx).stop;
            bool exist = false;
            for (size_t uniqueIdx = 0; uniqueIdx < uniqueStations.size(); uniqueIdx++)
            {
                if (uniqueStations.at(uniqueIdx).stop->getName() == currentStation->getName())
                {
                    exist = true;
                    break;
                }
            }
            if (!exist)
            {
                StopInfo newStationInfo;
                newStationInfo.stop = currentStation;
                uniqueStations.push_back(newStationInfo);
            }
        }
    }

    return uniqueStations;
}
void updateTramInfoForStops(const StopList &allStops, shared_ptr<PassengerI> passenger)
{
    for (int stopIndex = 0; stopIndex < allStops.size(); stopIndex++)
    {
        shared_ptr<TramStopPrx> tramStop = allStops.at(stopIndex).stop;

        TramList fullTramList;
        int batchSize = 1;
        int totalFetched = 0;

        while (true)
        {
            TramList batch = tramStop->getNextTrams(totalFetched + batchSize);

            if (batch.size() == fullTramList.size())
            {
                break;
            }

            fullTramList = batch;
            totalFetched += batchSize;
        }
        passenger->updateStopInfo(tramStop, fullTramList, Ice::Current());
    }
}
bool checkStopName(const string &name, const StopList &allStops)
{
    for (int i = 0; i < allStops.size(); i++)
    {
        if (allStops.at(i).stop->getName() == name)
        {
            return true;
        }
    }
    return false;
}
bool checkTramNumber(const string &number, const LineList &lines)
{
    for (int i = 0; i < lines.size(); i++)
    {
        TramList trams = lines.at(i)->getTrams();
        for (int j = 0; j < trams.size(); j++)
        {
            if (trams.at(j).tram->getStockNumber() == number)
            {
                return true;
            }
        }
    }
    return false;
}
shared_ptr<TramPrx> getTramByNumber(const string &number, const LineList &lines)
{
    for (int i = 0; i < lines.size(); i++)
    {
        TramList trams = lines.at(i)->getTrams();
        for (int j = 0; j < trams.size(); j++)
        {
            if (trams.at(j).tram->getStockNumber() == number)
            {
                return trams.at(j).tram;
            }
        }
    }
    return nullptr;
}
void displayMenu()
{
    cout << "\n===== MENU GŁÓWNE =====" << endl;
    cout << "1. Zasubskrybuj przystanek" << endl;
    cout << "2. Zasubskrybuj tramwaj" << endl;
    cout << "3. Wyświetl wszystkie linie" << endl;
    cout << "4. Wyświetl wszystkie przystanki" << endl;
    cout << "5. Wyświetl informacje o tramwaju" << endl;
    cout << "6. Wyrejestruj z przystanku" << endl;
    cout << "7. Wyrejestruj z tramwaju" << endl;
    cout << "0. Wyjście" << endl;
    cout << "Wybierz opcję: ";
}
void subscribeStop(const string &stopName, const StopList &allStops, shared_ptr<MPKPrx> mpk, shared_ptr<PassengerPrx> passengerPrx, shared_ptr<PassengerI> passenger, vector<shared_ptr<TramStopPrx>> &subscribedStops)
{
    if (checkStopName(stopName, allStops))
    {
        shared_ptr<TramStopPrx> tramStop = mpk->getTramStop(stopName);
        if (tramStop)
        {
            tramStop->RegisterPassenger(passengerPrx);
            passenger->setTramStopName(stopName);
            subscribedStops.push_back(tramStop);
            cout << "subbed przystanek: " << stopName << endl;
        }
        else
        {
            cout << "err: Nie udało się pobrać przystanku." << endl;
        }
    }
    else
    {
        cout << "err: Przystanek o podanej nazwie nie istnieje." << endl;
    }
}
void subscribeTram(const string &tramNumber, const LineList &lines, shared_ptr<PassengerPrx> passengerPrx, vector<shared_ptr<TramPrx>> &subscribedTrams)
{
    shared_ptr<TramPrx> tram = getTramByNumber(tramNumber, lines);
    if (tram)
    {
        tram->RegisterPassenger(passengerPrx);
        subscribedTrams.push_back(tram);
        cout << "subbed tramwaj numer: " << tramNumber << endl;
    }
    else
    {
        cout << "err: Nie udało się pobrać tramwaju." << endl;
    }
}
void displayAllLines(const LineList &lines)
{
    cout << "\n╔═══════════════════════════════════════════╗" << endl;
    cout << "║            DOSTĘPNE LINIE                 ║" << endl;
    cout << "╚═══════════════════════════════════════════╝" << endl;

    for (int index = 0; index < lines.size(); ++index)
    {
        cout << "-----------------------------------" << endl;

        string lineHeader = "│ Linia nr: " + lines.at(index)->getName();
        cout << lineHeader << endl;

        cout << "-----------------------------------" << endl;

        cout << "│ Przystanki:" << endl;
        StopList tramStops = lines.at(index)->getStops();
        // przystanki
        for (int stopIndex = 0; stopIndex < tramStops.size(); stopIndex++)
        {
            shared_ptr<TramStopPrx> tramStop = tramStops.at(stopIndex).stop;
            string stopName = "│  • " + tramStop->getName();
            cout << stopName << endl;
        }

        // tramwaje numery
        cout << "-----------------------------------" << endl;
        cout << "| Tramwaje:                         " << endl;

        TramList trams = lines.at(index)->getTrams();

        if (trams.size() == 0)
        {
            cout << "│ (brak) " << endl;
        }
        else
        {
            string tramNumbers = "│  ";

            for (int tramIndex = 0; tramIndex < trams.size(); tramIndex++)
            {
                string tramNumber = trams.at(tramIndex).tram->getStockNumber();
                // przecinek bo to nie jest ostatni element
                if (tramIndex < trams.size() - 1)
                {
                    tramNumber += ", ";
                }

                tramNumbers += tramNumber;
            }
            // tramwaje numery
            if (tramNumbers != "│  ")
            {
                cout << tramNumbers << endl;
            }
        }

        cout << "-----------------------------------" << endl;
        cout << endl;
    }
}
void displayTramInfo(const string &tramNumber, const LineList &lines)
{
    if (checkTramNumber(tramNumber, lines))
    {
        shared_ptr<TramPrx> tram = getTramByNumber(tramNumber, lines);
        if (tram)
        {
            cout << "\n╔═══════════════════════════════════════════╗" << endl;
            cout << "║                 TRAMWAJ INFO              ║" << endl;
            cout << "╚═══════════════════════════════════════════╝" << endl;
            cout << "---------------------------------------------" << endl;
            cout << "| \tLinia: " << tram->getLine()->getName() << endl;
            cout << "---------------------------------------------" << endl;
            cout << "| \tAktualna lokalizacja: " << tram->getLocation()->getName() << endl;
            cout << "---------------------------------------------" << endl;
            cout << "Ile nastepnych przystankow chcesz zobaczyc? : ";
            int count;
            cin >> count;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            StopList next = tram->getNextStops(count);
            cout << "Następne przystanki:" << endl;
            cout << "---------------------------------------------" << endl;
            for (int i = 0; i < next.size(); i++)
            {
                cout << "| \t" << next.at(i).stop->getName() << endl;
            }
            cout << "---------------------------------------------" << endl;
        }
        else
        {
            cout << "err: Nie udało się pobrać tramwaju." << endl;
        }
    }
    else
    {
        cout << "err: Tramwaj o podanym numerze nie istnieje." << endl;
    }
}
void unregisterFromStop(vector<shared_ptr<TramStopPrx>> &subscribedStops, shared_ptr<PassengerPrx> passengerPrx)
{
    if (subscribedStops.empty())
    {
        cout << "nie masz zasubskrybowanych przystanków." << endl;
        return;
    }

    cout << "twoje zasubskrybowane przystanki:" << endl;
    for (int i = 0; i < subscribedStops.size(); i++)
    {
        cout << i + 1 << ". " << subscribedStops[i]->getName() << endl;
    }

    cout << "wybierz numer przystanku do wyrejestrowania " << endl;
    int index;
    cin >> index;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    if (index >= 1 && index <= subscribedStops.size())
    {
        subscribedStops[index - 1]->UnregisterPassenger(passengerPrx);
        cout << "unregistered z przystanku: " << subscribedStops[index - 1]->getName() << endl;
        subscribedStops.erase(subscribedStops.begin() + (index - 1));
    }
    else
    {
        cout << "Nieprawidłowy numer przystanku." << endl;
    }
}
void unregisterFromTram(vector<shared_ptr<TramPrx>> &subscribedTrams, shared_ptr<PassengerPrx> passengerPrx)
{
    if (subscribedTrams.empty())
    {
        cout << "nie masz zasubskrybowanych tramwajów." << endl;
        return;
    }

    cout << "twoje zasubskrybowane tramwaje:" << endl;
    for (int i = 0; i < subscribedTrams.size(); i++)
    {
        cout << i + 1 << ". " << subscribedTrams[i]->getStockNumber() << endl;
    }

    cout << "wybierz numer tramwaju do wyrejestrowania" << endl;
    int index;
    cin >> index;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    if (index >= 1 && index <= subscribedTrams.size())
    {
        subscribedTrams[index - 1]->UnregisterPassenger(passengerPrx);
        cout << "wyrejestrowano z tramwaju: " << subscribedTrams[index - 1]->getStockNumber() << endl;
        subscribedTrams.erase(subscribedTrams.begin() + (index - 1));
    }
    else
    {
        cout << "nieprawidlowy numer tramwaju." << endl;
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        cerr << "Użycie: " << argv[0] << " <passenger port ex.10010>" << endl;
        return 1;
    }

    string passengerPort = argv[1];
    ConfigData config;
    if (!loadConfig("config.txt", config))
    {
        return 1;
    }
    cout << "\n╔═══════════════════════════════════════════╗" << endl;
    cout << "║           PODAJ IMIE UZYTKOWNIKA          ║" << endl;
    cout << "╚═══════════════════════════════════════════╝" << endl;
    string nameUser;
    cin >> nameUser;
    std::cin.clear();

    Ice::CommunicatorPtr ic;

    try
    {
        // ice
        ic = Ice::initialize(argc, argv);
        auto connector = ic->stringToProxy(config.serverName + ":default -h " + config.serverAddress + " -p " + config.serverPort + " -t 8000");
        auto mpk = Ice::checkedCast<MPKPrx>(connector);
        if (!mpk)
        {
            throw "Invalid proxy";
        }
        Ice::ObjectAdapterPtr adapter = ic->createObjectAdapterWithEndpoints("PassengerAdapter", "default -h " + config.passengerAddress + " -p " + passengerPort);
        auto passenger = make_shared<PassengerI>();
        auto passengerPrx = Ice::uncheckedCast<PassengerPrx>(adapter->addWithUUID(passenger));
        adapter->add(passenger, Ice::stringToIdentity(nameUser));

        adapter->activate();

        LineList lines = mpk->getLines();

        StopList allStops = getAllUniqueStops(lines);

        updateTramInfoForStops(allStops, passenger);

        vector<shared_ptr<TramStopPrx>> subscribedStops;
        vector<shared_ptr<TramPrx>> subscribedTrams;

        // main loop
        int choice = -1;
        while (choice != 0)
        {
            displayMenu();
            cin >> choice;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');

            switch (choice)
            {
            case 1:
            {
                // sub przystanku
                cout << "Podaj nazwę przystanku do zasubskrybowania: ";
                string stopName;
                getline(cin, stopName);
                subscribeStop(stopName, allStops, mpk, passengerPrx, passenger, subscribedStops);
                break;
            }

            case 2:
            {
                // sub tram
                cout << "Podaj numer tramwaju do zasubskrybowania: ";
                string tramNumber;
                getline(cin, tramNumber);

                if (checkTramNumber(tramNumber, lines))
                {
                    subscribeTram(tramNumber, lines, passengerPrx, subscribedTrams);
                }
                else
                {
                    cout << "Błąd: Tramwaj o podanym numerze nie istnieje." << endl;
                }
                break;
            }

            case 3:
            {
                // all lines
                displayAllLines(lines);
                break;
            }

            case 4:
            {
                // all stops
                cout << "\n╔═══════════════════════════════════════════╗" << endl;
                cout << "║            DOSTĘPNE PRZYSTANKI            ║" << endl;
                cout << "╚═══════════════════════════════════════════╝" << endl;

                for (int stopIndex = 0; stopIndex < allStops.size(); stopIndex++)
                {
                    shared_ptr<TramStopPrx> tramStop = allStops.at(stopIndex).stop;
                    cout << "| \t" << tramStop->getName() << endl;
                }

                cout << "-----------------------------------" << endl;
                break;
            }
            case 5:
            {
                // info o tramwaju
                cout << "Podaj numer tramwaju: ";
                string tramNumber;
                getline(cin, tramNumber);
                displayTramInfo(tramNumber, lines);
                break;
            }

            case 6:
            { // unregister z przystanku
                unregisterFromStop(subscribedStops, passengerPrx);
                break;
            }

            case 7:
            {
                // unregister z tramwaju
                unregisterFromTram(subscribedTrams, passengerPrx);
                break;
            }

            case 0: // exit
                cout << "Kończenie programu..." << endl;
                break;

            default:
                cout << "Nieprawidłowa opcja. Spróbuj ponownie." << endl;
                break;
            }
        }

        // unregister all
        for (auto &stop : subscribedStops)
        {
            try
            {
                stop->UnregisterPassenger(passengerPrx);
            }
            catch (...)
            {
                // ignore
            }
        }

        for (auto &tram : subscribedTrams)
        {
            try
            {
                tram->UnregisterPassenger(passengerPrx);
            }
            catch (...)
            {
                // ignore
            }
        }
    }
    catch (const Ice::Exception &e)
    {
        cout << e << endl;
    }
    catch (const char *msg)
    {
        cout << msg << endl;
    }

    if (ic)
    {
        try
        {
            ic->destroy();
        }
        catch (const Ice::Exception &e)
        {
            cout << e << endl;
        }
    }

    cout << "Koniec programu użytkownika" << endl;
    return 0;
}