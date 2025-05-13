#include <Ice/Ice.h>
#include "MPK.h"
#include <iostream>
#include <memory>
#include <fstream>
#include <string>
#include "TramI.h"
using namespace std;
using namespace SIP;
struct ConfigData
{
    string tramAddress;
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
                config.tramAddress = value;
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
int getIdLine(LineList lines, string name)
{
    for (int i = 0; i < lines.size(); ++i)
    {
        if (lines.at(i)->getName() == name)
        {
            return i;
        }
    }
    return -1;
}
bool checkName(string line_name, LineList lines)
{
    for (int i = 0; i < lines.size(); ++i)
    {
        if (line_name == lines.at(i)->getName())
        {
            return true;
        }
    }
    return false;
}
void displayAvailableLines(const LineList &lines)
{
    cout << "\n╔═══════════════════════════════════════════╗" << endl;
    cout << "║            DOSTĘPNE LINIE                 ║" << endl;
    cout << "╚═══════════════════════════════════════════╝" << endl;
    for (int index = 0; index < lines.size(); ++index)
    {
        cout << "Linia nr: " << lines.at(index)->getName() << endl;
        cout << "------------------------------------" << endl;
        cout << "Przystanki:" << endl;

        StopList tramStops = lines.at(index)->getStops();
        for (int stopIndex = 0; stopIndex < tramStops.size(); stopIndex++)
        {
            cout << "  - " << tramStops.at(stopIndex).stop->getName() << endl;
        }

        cout << endl;
    }
    cout << "====================================" << endl
         << endl;
}
void setStopTimesWithRandomIntervals(shared_ptr<TramI> tram, StopList tramStops, shared_ptr<LinePrx> linePrx, shared_ptr<TramPrx> tramPrx)
{
    time_t currentTime;
    time(&currentTime);
    tm *now = localtime(&currentTime);

    int hour = now->tm_hour;
    int minute = now->tm_min;

    srand(time(NULL));

    for (int index = 0; index < tramStops.size(); index++)
    {
        Time time;
        time.hour = hour;
        time.minute = minute;
        StopInfo stopInfo;
        stopInfo.time = time;
        shared_ptr<TramStopPrx> tramStopPrx = tramStops.at(index).stop;
        stopInfo.stop = tramStopPrx;
        tram->addTramStop(stopInfo);
        tramStops.at(index).stop->UpdateTramInfo(tramPrx, time);
        int randomInterval = rand() % 10 + 1;
        minute += randomInterval;
        if (minute >= 60)
        {
            hour += minute / 60;
            minute = minute % 60;
        }
        if (hour >= 24)
        {
            hour = hour % 24;
        }
        cout << "Przystanek " << tramStops.at(index).stop->getName()
             << " - czas przyjazdu: " << time.hour << ":"
             << (time.minute < 10 ? "0" : "") << time.minute << endl;
    }
}
void showMenu()
{
    cout << "========== MENU STEROWANIA TRAMWAJEM ==========" << endl;
    cout << "  n lub next  - przejazd do następnego przystanku" << endl;
    cout << "  i lub info  - informacje o aktualnej pozycji" << endl;
    cout << "  l lub line  - pokaż wszystkie przystanki na linii" << endl;
    cout << "  q lub quit  - zakończ program" << endl;
    cout << "  m lub menu  - pokaż to menu" << endl;
    cout << "=============================================" << endl;
}
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        cerr << "Usage: " << argv[0] << " <tramPort ex. 10010>" << endl;
        return 1;
    }
    string tramPort = argv[1];
    ConfigData config;
    if (!loadConfig("config.txt", config))
    {
        cout << "Nie udalo sie wczytac pliku konfiguracyjnego" << endl;
        return 1;
    }

    Ice::CommunicatorPtr ic;
    try
    {
        ic = Ice::initialize(argc, argv);
        auto base = ic->stringToProxy(config.serverName + ":default -h " + config.serverAddress + " -p " + config.serverPort + " -t 8000");
        auto mpk = Ice::checkedCast<MPKPrx>(base);
        if (!mpk)
        {
            throw "Invalid proxy";
        }
        LineList lines = mpk->getLines();
        // wyswietlam info o dostepnych liniach
        displayAvailableLines(lines);

        Ice::ObjectAdapterPtr adapter = ic->createObjectAdapterWithEndpoints("TramAdapter", "default -h " + config.tramAddress + " -p " + tramPort);

        // tutaj tylko numer do identyfikacji tramwaju
        string stockNum;
        cout << "Wybierz wlasny identyfikator tramwaju: ";
        cin >> stockNum;
        std::cin.clear();
        cout << endl;

        auto tram = make_shared<TramI>(stockNum); // tram
        auto tramPrx = Ice::uncheckedCast<TramPrx>(adapter->addWithUUID(tram));

        tram->setProxy(tramPrx);
        adapter->add(tram, Ice::stringToIdentity("tram" + stockNum));
        string line_name;
        cout << "Wybierz istniejaca linie po ktorej bedziesz kroczyl swoim tempem :)";
        cin >> line_name;

        while (!checkName(line_name, lines))
        {
            cout << "niewlasciwa linia, wybierz ponownie: " << endl;
            cin >> line_name;
        }

        std::cin.clear();

        // ustawienie czasu dotarcia na przystanki

        int ID = getIdLine(lines, line_name);

        shared_ptr<LinePrx> linePrx = lines.at(ID);
        tram->setLine(linePrx, Ice::Current());

        StopList tramStops = linePrx->getStops();
        setStopTimesWithRandomIntervals(tram, tramStops, linePrx, tramPrx);
        // dolaczanie do linii
        adapter->activate();
        linePrx->registerTram(tramPrx);
        mpk->getDepo("Zajezdnia1")->registerTram(tramPrx);
        cout << "Waiting for tram to be online..." << endl;
        while (tram->getStatus(Ice::Current()) != SIP::TramStatus::ONLINE)
        {
        }
        showMenu();
        string command;
        while (true)
        {
            cout << "\n> ";
            cin >> command;
            transform(command.begin(), command.end(), command.begin(), ::tolower);

            if (command == "q" || command == "quit")
            {
                cout << "Kończenie pracy tramwaju..." << endl;
                // tram->unregisterAllUser(tramPrx);
                break;
            }

            if (command == "n" || command == "next")
            {
                StopList stops = linePrx->getStops();
                auto currentStopName = tramPrx->getLocation()->getName();
                auto lastStopName = stops.at(stops.size() - 1).stop->getName();

                // Sprawdzanie czy to ostatni przystanek
                if (currentStopName == lastStopName)
                {
                    cout << "Jesteś już na końcowym przystanku: " << currentStopName << endl;
                    cout << "Zawracam na początek trasy..." << endl;
                }

                tram->goNext();
                cout << "Dotarłeś do przystanku: " << tramPrx->getLocation()->getName() << endl;
                // tram->informAllUser(tramPrx);
            }
            else if (command == "i" || command == "info")
            {
                cout << "Aktualny przystanek: " << tramPrx->getLocation()->getName() << endl;
                cout << "Linia: " << line_name << endl;
                cout << "Numer tramwaju: " << stockNum << endl;
                cout << "Status: " << (tram->getStatus(Ice::Current()) == SIP::TramStatus::ONLINE ? "ONLINE" : "OFFLINE") << endl;
            }
            else if (command == "l" || command == "line")
            {
                StopList stops = linePrx->getStops();
                auto currentStopName = tramPrx->getLocation()->getName();
                cout << "Linia " << line_name << " - przystanki:" << endl;
                cout << "------------------------------------" << endl;
                for (int i = 0; i < stops.size(); i++)
                {
                    string marker = (stops.at(i).stop->getName() == currentStopName) ? " [TUTAJ] " : "";
                    cout << (i + 1) << ". " << stops.at(i).stop->getName() << marker << endl;
                }
            }
            else if (command == "m" || command == "menu")
            {
                showMenu();
            }
            else
            {
                cout << "Nieznane polecenie. Wpisz 'n', 'i', 'l' lub 'q'" << endl;
            }
        }
        mpk->getDepo("Zajezdnia1")->unregisterTram(tramPrx);
        cout << "Jestes w zajezdni, czekam na offline tramwaju..." << endl;
        while (tram->getStatus(Ice::Current()) != SIP::TramStatus::OFFLINE)
        {
        }
        linePrx->unregisterTram(tramPrx);
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

    cout << "Koniec programu tramwaj" << endl;
}
