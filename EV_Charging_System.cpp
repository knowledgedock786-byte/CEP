#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <limits>
using namespace std;

// ---------- EV ----------
class EV {
public:
    string id, type;
    int soc;

    EV(string i = "", string t = "", int s = 0) {
        id = i;
        type = t;
        soc = s;
    }
};

// ---------- USER ----------
class User {
public:
    string name;
    bool premium;
    EV vehicle;

    User(string n = "", bool p = false, EV v = EV()) {
        name = n;
        premium = p;
        vehicle = v;
    }
};

// ---------- CHARGING SESSION ----------
class ChargingSession {
public:
    string userName, vehicleId;
    string stationName;
    int dockNumber;
    string energyType;
    double powerRating, duration;
    bool isPeak, isPremium;
    double totalBill;
    string dateTime;

    ChargingSession() {}

    ChargingSession(string uname, string vid, string sname, int dock,
                    string energy, double power, double dur,
                    bool peak, bool prem) {
        userName = uname;
        vehicleId = vid;
        stationName = sname;
        dockNumber = dock;
        energyType = energy;
        powerRating = power;
        duration = dur;
        isPeak = peak;
        isPremium = prem;
        totalBill = calculateBill();
        dateTime = getCurrentDateTime();
    }

    static string getCurrentDateTime() {
        time_t now = time(0);
        tm* ltm = localtime(&now);
        ostringstream oss;
        oss << 1900 + ltm->tm_year << "-"
            << setw(2) << setfill('0') << 1 + ltm->tm_mon << "-"
            << setw(2) << setfill('0') << ltm->tm_mday << " "
            << setw(2) << ltm->tm_hour << ":"
            << setw(2) << ltm->tm_min;
        return oss.str();
    }

    double calculateBill() {
        double energy = powerRating * duration;
        double rate = (energyType == "Renewable") ? 15 : 5;

        if (isPeak) rate *= 1.3;
        if (isPremium) rate *= 0.8;

        return energy * rate;
    }

    void printInvoice() {
        cout << "\n========== INVOICE ==========\n";
        cout << "Date: " << dateTime << endl;
        cout << "User: " << userName << endl;
        cout << "Vehicle: " << vehicleId << endl;
        cout << "Station: " << stationName << endl;
        cout << "Dock No: " << dockNumber << endl;
        cout << "Energy Type: " << energyType << endl;
        cout << "Energy Used: " << powerRating * duration << " kWh\n";
        cout << "Total Bill: " << totalBill << " PKR\n";
        cout << "=============================\n";
    }
};

// ---------- DOCK ----------
class ChargingDock {
public:
    int power;
    string energyType;
    bool available;

    ChargingDock(int p = 0, string e = "") {
        power = p;
        energyType = e;
        available = true;
    }
};

// ---------- STATION ----------
class ChargingStation {
public:
    string name;
    vector<ChargingDock> docks;
    vector<ChargingSession> activeSessions;

    ChargingStation(string n = "") {
        name = n;
    }

    void addDock(int power, string energy) {
        docks.push_back(ChargingDock(power, energy));
    }

    void showDocks() {
        cout << "\nStation: " << name << endl;
        for (int i = 0; i < docks.size(); i++) {
            cout << "Dock " << i + 1
                 << " | " << docks[i].power << "kW"
                 << " | " << docks[i].energyType
                 << " | " << (docks[i].available ? "Available" : "Occupied")
                 << endl;
        }
    }

    int getActiveUsers() {
        return activeSessions.size();
    }
};

// ---------- SYSTEM ----------
class EVChargingSystem {
public:
    vector<User> users;
    vector<ChargingStation> stations;
    vector<ChargingSession> records;

    void clearInput() {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }

    int getValidBinary(string msg) {
        int x;
        while (true) {
            cout << msg;
            cin >> x;
            if (cin.fail() || (x != 0 && x != 1)) {
                clearInput();
                cout << "Invalid input!\n";
            } else return x;
        }
    }

    void registerUser() {
        string name, vid, type;
        int soc;
        bool premium;

        cout << "Enter user name: ";
        cin >> name;
        cout << "Enter vehicle ID: ";
        cin >> vid;
        cout << "Enter vehicle type: ";
        cin >> type;
        cout << "Enter SOC: ";
        cin >> soc;

        premium = getValidBinary("Premium user? (1/0): ");

        users.push_back(User(name, premium, EV(vid, type, soc)));
        cout << "User registered!\n";
    }

    void showUsers() {
        if (users.empty()) {
            cout << "No users registered!\n";
            return;
        }

        for (int i = 0; i < users.size(); i++) {
            cout << i << ". " << users[i].name
                 << " | " << users[i].vehicle.id
                 << " | SOC: " << users[i].vehicle.soc << "%\n";
        }
    }

    void searchUser() {
        string key;
        cout << "Enter name or vehicle ID: ";
        cin >> key;

        for (auto &u : users) {
            if (u.name == key || u.vehicle.id == key) {
                cout << "Found: " << u.name
                     << " | " << u.vehicle.id << endl;
                return;
            }
        }
        cout << "User not found!\n";
    }

    void showStations() {
        for (int i = 0; i < stations.size(); i++) {
            cout << i << ". " << stations[i].name << endl;
        }
    }

    void showAllDocks() {
        for (auto &s : stations)
            s.showDocks();
    }

    void startSession() {
        if (users.empty()) {
            cout << "No users registered!\n";
            return;
        }

        int uChoice, sChoice, energyChoice;
        double duration;
        bool peak;

        showUsers();
        cout << "Select user: ";
        cin >> uChoice;

        showStations();
        cout << "Select station: ";
        cin >> sChoice;

        peak = getValidBinary("Peak hour? (1/0): ");

        cout << "Enter duration(hours): ";
        cin >> duration;

        cout << "Energy Type: 1.Renewable  2.Non-Renewable: ";
        cin >> energyChoice;

        string selectedEnergy = (energyChoice == 1) ? "Renewable" : "Non-Renewable";

        if (uChoice < 0 || uChoice >= users.size() ||
            sChoice < 0 || sChoice >= stations.size()) {
            cout << "Invalid selection!\n";
            return;
        }

        bool found = false;

        for (int i = 0; i < stations[sChoice].docks.size(); i++) {
            ChargingDock &d = stations[sChoice].docks[i];

            if (d.available && d.energyType == selectedEnergy) {
                d.available = false;

                User u = users[uChoice];

                ChargingSession s(
                    u.name,
                    u.vehicle.id,
                    stations[sChoice].name,
                    i + 1,
                    d.energyType,
                    d.power,
                    duration,
                    peak,
                    u.premium
                );

                stations[sChoice].activeSessions.push_back(s);
                records.push_back(s);
                saveCSV(s);

                cout << "Session started!\n";
                s.printInvoice();
                found = true;
                break;
            }
        }

        if (!found)
            cout << "No matching dock available!\n";
    }

    void completeSession() {
        int sChoice, dockNo;

        showStations();
        cout << "Select station: ";
        cin >> sChoice;

        cout << "Enter dock number: ";
        cin >> dockNo;

        if (sChoice >= 0 && sChoice < stations.size()) {
            if (dockNo >= 1 && dockNo <= stations[sChoice].docks.size()) {
                stations[sChoice].docks[dockNo - 1].available = true;
                cout << "Dock freed!\n";
            }
        }
    }

    void totalUsers() {
        int total = 0;
        for (auto &s : stations)
            total += s.getActiveUsers();

        cout << "Total Active Users: " << total << endl;
    }

    void saveCSV(ChargingSession s) {
        ofstream file("records.csv", ios::app);
        file << s.userName << ","
             << s.vehicleId << ","
             << s.stationName << ","
             << s.dockNumber << ","
             << s.totalBill << ","
             << s.dateTime << "\n";
        file.close();
    }

    void saveUsersCSV() {
        ofstream file("users.csv");
        for (auto &u : users) {
            file << u.name << ","
                 << u.vehicle.id << ","
                 << u.vehicle.type << ","
                 << u.vehicle.soc << "\n";
        }
        cout << "Users saved!\n";
    }

    void viewCSV() {
        ifstream file("records.csv");
        if (!file) {
            cout << "No records found!\n";
            return;
        }

        string line;
        while (getline(file, line))
            cout << line << endl;
    }

    void generateTxtInvoices() {
        for (auto &r : records) {
            string filename = r.userName + "_invoice.txt";
            ofstream file(filename);
            file << "INVOICE\n";
            file << "User: " << r.userName << endl;
            file << "Vehicle: " << r.vehicleId << endl;
            file << "Bill: " << r.totalBill << endl;
        }
        cout << "TXT invoices generated!\n";
    }

    void viewInvoices() {
        if (records.empty()) {
            cout << "No invoices!\n";
            return;
        }

        for (auto &r : records)
            r.printInvoice();
    }
};

// ---------- MAIN ----------
int main() {
    EVChargingSystem system;

    ChargingStation s1("NUST Station");
    s1.addDock(7, "Renewable");
    s1.addDock(22, "Renewable");
    s1.addDock(50, "Non-Renewable");

    ChargingStation s2("City Center");
    s2.addDock(7, "Renewable");
    s2.addDock(22, "Renewable");
    s2.addDock(50, "Non-Renewable");

    system.stations.push_back(s1);
    system.stations.push_back(s2);

    int choice;
    do {
        cout << "\n===== SUPER EV CHARGING SYSTEM =====\n";
        cout << "1. Register User\n";
        cout << "2. View Users\n";
        cout << "3. Search User\n";
        cout << "4. View Stations & Docks\n";
        cout << "5. Start Charging Session\n";
        cout << "6. Complete Charging Session\n";
        cout << "7. View Invoices\n";
        cout << "8. View CSV Records\n";
        cout << "9. Save Users CSV\n";
        cout << "10. Generate TXT Invoices\n";
        cout << "11. Total Active Users\n";
        cout << "0. Exit\n";
        cout << "Enter choice: ";
        cin >> choice;

        switch(choice) {
            case 1: system.registerUser(); break;
            case 2: system.showUsers(); break;
            case 3: system.searchUser(); break;
            case 4: system.showAllDocks(); break;
            case 5: system.startSession(); break;
            case 6: system.completeSession(); break;
            case 7: system.viewInvoices(); break;
            case 8: system.viewCSV(); break;
            case 9: system.saveUsersCSV(); break;
            case 10: system.generateTxtInvoices(); break;
            case 11: system.totalUsers(); break;
            case 0: cout << "Exiting...\n"; break;
            default: cout << "Invalid choice!\n";
        }

    } while(choice != 0);

    return 0;
}
