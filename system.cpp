#include <Ice/Ice.h>
#include "MPK.h"
#include <iostream>
#include <memory>
#include <fstream>
#include <string>
#include <algorithm>
#include "SystemI.h"
using namespace std;
using namespace SIP;
struct ConfigData
{
    string serverAddress;
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
            if (token == "server.name")
            {
                config.serverName = value;
            }
            else if (token == "server.addr")
            {
                config.serverAddress = value;
            }
        }
    }
    configFile.close();
    if (config.serverAddress.empty())
    {
        cout << "ERR: Brakujące wymagane parametry w pliku konfiguracyjnym: " << configFilePath << endl;
        cout << "INFO: Wymagane parametry: server.addr" << endl;
        return false;
    }

    return true;
}
string displayMainMenu()
{
    cout << "\n╔═════════════════════════════════════════════╗" << endl;
    cout << "║            SYSTEM ZARZĄDZANIA MPK          ║" << endl;
    cout << "╚═════════════════════════════════════════════╝" << endl;
    cout << "  [l] - Wyświetl informacje o liniach" << endl;
    cout << "  [d] - Zarządzaj zajezdnią" << endl;
    cout << "  [q] - Zakończ pracę systemu" << endl;
    cout << "─────────────────────────────────────────────" << endl;
    cout << "> ";
    string option;
    getline(cin, option);
    return option;
}
void displayLinesInfo(MPK_I *mpk)
{
    LineList lines = mpk->getLines(Ice::Current());

    cout << "\n╔═════════════════════════════════════════════╗" << endl;
    cout << "║            DOSTĘPNE LINIE MPK              " << endl;
    cout << "╚═════════════════════════════════════════════╝" << endl;

    for (int i = 0; i < lines.size(); ++i)
    {
        cout << "Linia nr: " << lines.at(i)->getName() << endl;
        cout << "──────────────────────────────────────────" << endl;
        cout << "Przystanki: ";
        StopList stops = lines.at(i)->getStops();
        for (int j = 0; j < stops.size(); ++j)
        {
            cout << stops.at(j).stop->getName();
            if (j < stops.size() - 1)
            {
                cout << " → ";
            }
        }
        cout << endl
             << endl;
    }
}
void manageDepo(shared_ptr<DepoPrx> proxyDepo)
{
    bool exitDepo = false;

    while (!exitDepo)
    {
        cout << "\n╔═════════════════════════════════════════════╗" << endl;
        cout << "║         ZARZĄDZANIE ZAJEZDNIĄ: " << proxyDepo->getName() << endl;
        cout << "╚═════════════════════════════════════════════╝" << endl;
        cout << "Zarejestrowane tramwaje:" << endl;
        cout << "──────────────────────────────────────────" << endl;

        TramList tramList = proxyDepo->getTrams(Ice::Context());
        for (int i = 0; i < tramList.size(); ++i)
        {
            cout << i << ". " << tramList.at(i).tram->getStockNumber() << " - ";
            switch (tramList.at(i).tram->getStatus(Ice::Context()))
            {
            case SIP::TramStatus::ONLINE:
                cout << "W ruchu" << endl;
                break;
            case SIP::TramStatus::OFFLINE:
                cout << "Wyłączony" << endl;
                break;
            case SIP::TramStatus::WAITONLINE:
                cout << "Oczekuje na uruchomienie" << endl;
                break;
            case SIP::TramStatus::WAITOFFLINE:
                cout << "Oczekuje na wyłączenie" << endl;
                break;
            default:
                cout << "Status nieznany" << endl;
            }
        }

        cout << "\nPolecenia:" << endl;
        cout << "  <numer> ONLINE  - wlacza tramwaj" << endl;
        cout << "  <numer> OFFLINE - wylacza tramwaj" << endl;
        cout << "  q - Powrót do głównego menu" << endl;
        cout << "> ";

        string command;
        getline(cin, command);

        if (command == "q")
        {
            exitDepo = true;
            continue;
        }

        istringstream iss(command);
        int number;
        string action;
        iss >> number >> action;

        if (iss.fail())
        {
            cout << "Nieprawidłowe polecenie. Użyj formatu: <numer> <ONLINE/OFFLINE>" << endl;
            continue;
        }
        if (number < 0 || number >= tramList.size())
        {
            cout << "Nieprawidłowy numer tramwaju." << endl;
            continue;
        }
        else if (action == "ONLINE")
        {
            shared_ptr<TramPrx> tram = tramList.at(number).tram;
            if (tram->getStatus(Ice::Context()) == SIP::TramStatus::ONLINE)
            {
                cout << "Tramwaj jest już uruchomiony!" << endl;
            }
            else
            {
                proxyDepo->TramOnline(tram, Ice::Context());
                cout << "Tramwaj " << tram->getStockNumber() << " został uruchomiony." << endl;
            }
        }
        else if (action == "OFFLINE")
        {
            shared_ptr<TramPrx> tram = tramList.at(number).tram;
            if (tram->getStatus(Ice::Context()) == SIP::TramStatus::OFFLINE)
            {
                cout << "Tramwaj jest już wyłączony!" << endl;
            }
            else
            {
                try
                {
                    proxyDepo->TramOffline(tram, Ice::Context());
                    cout << "Tramwaj " << tram->getStockNumber() << " został wyłączony." << endl;
                }
                catch (const Ice::ConnectionRefusedException &e)
                {
                    cout << "Nie można połączyć się z tramwajem. Prawdopodobnie klient tramwaju nie jest uruchomiony." << endl;
                    cout << "Szczegóły błędu: " << e << endl;
                }
                catch (const Ice::Exception &e)
                {
                    cout << "Wystąpił błąd podczas wyłączania tramwaju: " << e << endl;
                }
            }
        }
        else
        {
            cout << "Nieznana komenda." << endl;
        }
    }
}
bool importRouteData(const string &filepath, MPK_I *mpkSystem, LineFactoryI *lineFac, StopFactoryI *stopFac)
{
    ifstream file_p(filepath);
    if (!file_p)
    {
        cerr << "BŁĄD: Nie udało się otworzyć pliku z definicjami tras: " << filepath << endl;
        return false;
    }

    string dataLine;
    while (getline(file_p, dataLine))
    {
        const size_t delimiterPos = dataLine.find(':');
        if (delimiterPos == string::npos)
        {
            cerr << "BŁĄD: Nieprawidłowy format linii w pliku tras: " << dataLine << endl;
            continue;
        }
        const string stopid = dataLine.substr(0, delimiterPos);
        auto lineproxy = lineFac->createLine(stopid, Ice::Current());
        if (!lineproxy)
        {
            cerr << "BŁĄD: Nie udało się utworzyć obiektu trasy: " << stopid << endl;
            continue;
        }
        const string stopdata = dataLine.substr(delimiterPos + 1);
        StopList stationsList;
        istringstream stationsStream(stopdata);
        string stop_nm;
        struct tm currentTimeInfo;
        time_t raw;
        time(&raw);
        currentTimeInfo = *localtime(&raw);

        while (stationsStream >> stop_nm)
        {
            auto stopProxy = mpkSystem->getTramStop(stop_nm, Ice::Current());

            if (!stopProxy)
            {
                stopProxy = stopFac->createStop(stop_nm, Ice::Current());
                if (!stopProxy)
                {
                    cerr << "BŁĄD: Nie udało się utworzyć obiektu przystanku: " << stop_nm << endl;
                    continue;
                }
                mpkSystem->addStop(stopProxy);
            }

            StopInfo stopinfo;
            stopinfo.time.hour = currentTimeInfo.tm_hour;
            stopinfo.time.minute = currentTimeInfo.tm_min;
            stopinfo.stop = stopProxy;

            stationsList.push_back(stopinfo);
        }

        lineproxy
            ->setStops(stationsList);
        mpkSystem->addLine(lineproxy, Ice::Current());
    }

    file_p.close();
    return true;
}
int main(int argc, char *argv[])
{
    ConfigData config;
    if (!loadConfig("config.txt", config))
    {
        cout << "Nie można załadować pliku konfiguracyjnego." << endl;
        return 1;
    }
    Ice::CommunicatorPtr ic;
    try
    {
        ic = Ice::initialize(argc, argv);
        Ice::ObjectAdapterPtr adapter = ic->createObjectAdapterWithEndpoints("MPKAdapter", "default -h " + config.serverAddress + " -p 10000");

        auto mpk = make_shared<MPK_I>();
        adapter->add(mpk, Ice::stringToIdentity(config.serverName));

        auto depo = make_shared<DepoI>("Zajezdnia1");
        auto proxyDepo = Ice::uncheckedCast<DepoPrx>(adapter->addWithUUID(depo));
        mpk->registerDepo(proxyDepo, Ice::Current());

        auto lineFac = make_shared<LineFactoryI>(adapter);
        auto lineFacProxy = Ice::uncheckedCast<LineFactoryPrx>(adapter->addWithUUID(lineFac));
        mpk->registerLineFactory(lineFacProxy, Ice::Current());

        auto stopFac = make_shared<StopFactoryI>(adapter);
        auto stopFacProxy = Ice::uncheckedCast<StopFactoryPrx>(adapter->addWithUUID(stopFac));
        mpk->registerStopFactory(stopFacProxy, Ice::Current());

        // Wczytywanie przystanków
        ifstream stops_file("stops.txt");
        if (!stops_file.is_open())
        {
            cerr << "Nie można otworzyć pliku przystanków." << endl;
            throw "File error";
        }

        string stop_name;
        while (stops_file >> stop_name)
        {
            auto tramStopPrx = stopFac->createStop(stop_name, Ice::Current());
            mpk->addStop(tramStopPrx);
        }

        // wczytywanie lini
        if (!importRouteData("lines.txt", mpk.get(), lineFac.get(), stopFac.get()))
        {
            cout << "Wystąpił błąd podczas wczytywania definicji tras." << endl;
        }
        adapter->activate();
        bool exitSystem = false;
        string option;
        while (!exitSystem)
        {
            option = displayMainMenu();
            if (option == "l" || option == "lines")
            {
                displayLinesInfo(mpk.get());
            }
            else if (option == "d" || option == "depo")
            {
                manageDepo(proxyDepo);
            }
            else if (option == "q" || option == "quit")
            {
                exitSystem = true;
            }
            else
            {
                cout << "Nieznana komenda. Wybierz ponownie." << endl;
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

    cout << "Koniec pracy systemu" << endl;
}