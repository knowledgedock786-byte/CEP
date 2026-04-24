#include "NetworkManager.h"
#include <iostream>
#include <iomanip>
#include <cmath>

// ============================================================
// HELPER: Setup City Infrastructure
// ============================================================
void setupCityInfrastructure(NetworkManager& network) {
    std::cout << "\n  ========== SETTING UP CITY INFRASTRUCTURE ==========\n";

    // ---- Station 1: Downtown Hub ----
    ChargingStation* downtown = new ChargingStation(
        "ST001", "Downtown Hub", "Main Boulevard, Rawalpindi",
        33.597, 73.047, 400.0
    );
    downtown->addDocks(4, 6, 4); // 4 slow, 6 medium, 4 fast
    downtown->addEnergySource(new GridPower(300.0));
    downtown->addEnergySource(new SolarPower(80.0));
    downtown->addEnergySource(new WindPower(50.0));
    network.addStation(downtown);

    // ---- Station 2: Airport Terminal ----
    ChargingStation* airport = new ChargingStation(
        "ST002", "Airport Terminal", "Islamabad International Airport",
        33.549, 72.826, 500.0
    );
    airport->addDocks(2, 4, 8); // More fast chargers for travelers
    airport->addEnergySource(new GridPower(400.0));
    airport->addEnergySource(new SolarPower(120.0));
    network.addStation(airport);

    // ---- Station 3: University Campus ----
    ChargingStation* campus = new ChargingStation(
        "ST003", "University Campus", "NUST Campus, H-12",
        33.643, 72.990, 200.0
    );
    campus->addDocks(6, 4, 2); // Mostly slow/medium for students
    campus->addEnergySource(new GridPower(150.0));
    campus->addEnergySource(new SolarPower(60.0));
    campus->addEnergySource(new WindPower(40.0));
    network.addStation(campus);

    std::cout << "  [SETUP] 3 stations configured with 40 total docks\n";
}

// ============================================================
// HELPER: Register Users and Vehicles
// ============================================================
void registerUsersAndVehicles(NetworkManager& network,
    std::vector<User*>& users, std::vector<EV*>& evs) {

    std::cout << "\n  ========== USER & VEHICLE REGISTRATION ==========\n";

    // Users
    User* ali    = network.registerUser("Ali Hassan",   "ali@email.com",    "+92-300-1234567", MembershipLevel::PLATINUM);
    User* sara   = network.registerUser("Sara Khan",    "sara@email.com",   "+92-321-9876543", MembershipLevel::GOLD);
    User* bilal  = network.registerUser("Bilal Ahmed",  "bilal@email.com",  "+92-333-5555555", MembershipLevel::SILVER);
    User* farah  = network.registerUser("Farah Naz",    "farah@email.com",  "+92-311-4444444", MembershipLevel::BASIC);
    User* usman  = network.registerUser("Usman Malik",  "usman@email.com",  "+92-345-3333333", MembershipLevel::GOLD);

    // Top up wallets
    ali->topUpWallet(5000);
    sara->topUpWallet(3000);
    bilal->topUpWallet(2000);
    farah->topUpWallet(1500);
    usman->topUpWallet(4000);

    // Vehicles
    EV* teslaY   = network.registerVehicle(ali,   "Tesla Model Y",    VehicleType::SUV,        75.0, 0.25, "ISB-T-001");
    EV* bmwi4    = network.registerVehicle(sara,  "BMW i4",           VehicleType::SEDAN,      83.9, 0.45, "RWP-B-002");
    EV* cheryJ6  = network.registerVehicle(bilal, "Chery J6 EV",      VehicleType::SUV,        60.0, 0.15, "LHR-C-003");
    EV* nexonEV  = network.registerVehicle(farah, "Tata Nexon EV",    VehicleType::SUV,        40.5, 0.60, "ISB-N-004");
    EV* byd5     = network.registerVehicle(usman, "BYD Seal",         VehicleType::SEDAN,      82.5, 0.10, "KHI-D-005");
    EV* motoEV   = network.registerVehicle(bilal, "Jolta Electric",   VehicleType::MOTORCYCLE, 3.2,  0.30, "RWP-J-006");

    users = {ali, sara, bilal, farah, usman};
    evs   = {teslaY, bmwi4, cheryJ6, nexonEV, byd5, motoEV};
}

// ============================================================
// MAIN SIMULATION
// ============================================================
int main() {
    // Initialize Network
    NetworkManager network("CityCharge Network", "Rawalpindi-Islamabad");

    // Setup infrastructure
    setupCityInfrastructure(network);

    // Register users and vehicles
    std::vector<User*> users;
    std::vector<EV*>   evs;
    registerUsersAndVehicles(network, users, evs);

    // ============================================================
    // DISPLAY INITIAL STATE
    // ============================================================
    std::cout << "\n  ========== INITIAL NETWORK STATUS ==========\n";
    network.checkNetworkAvailability();
    network.displayCurrentPricing();

    // ============================================================
    // BOOKING PHASE
    // ============================================================
    std::cout << "\n  ========== BOOKING MANAGEMENT ==========\n";

    User* ali   = users[0]; EV* teslaY  = evs[0];
    User* sara  = users[1]; EV* bmwi4   = evs[1];
    User* bilal = users[2]; EV* cheryJ6 = evs[2];
    User* farah = users[3]; EV* nexon   = evs[3];
    User* usman = users[4]; EV* byd     = evs[4];

    // Book slots at different stations
    Booking* b1 = network.bookSlot(ali,   teslaY,  "ST001", DockType::FAST,   8.0, 2.0, 0.90);
    Booking* b2 = network.bookSlot(sara,  bmwi4,   "ST001", DockType::MEDIUM, 9.0, 1.5, 0.80);
    Booking* b3 = network.bookSlot(bilal, cheryJ6, "ST002", DockType::FAST,   8.0, 3.0, 0.85);
    Booking* b4 = network.bookSlot(farah, nexon,   "ST003", DockType::SLOW,   10.0,4.0, 0.70);
    Booking* b5 = network.bookSlot(usman, byd,     "ST001", DockType::FAST,   8.0, 2.5, 0.95);

    // Demonstrate cancellation
    std::cout << "\n  --- Demonstrating Booking Cancellation ---\n";
    network.cancelBooking(b4->getId(), farah);

    // Re-book for farah
    Booking* b6 = network.bookSlot(farah, nexon, "ST003", DockType::MEDIUM, 14.0, 2.0, 0.75);

    // ============================================================
    // SIMULATION: ADVANCE THROUGH HOURS
    // ============================================================
    std::cout << "\n  ========== STARTING SIMULATION ==========\n";

    // Hour 8: Morning rush
    std::cout << "\n  ===== HOUR 8:00 - MORNING PEAK =====\n";
    network.processChargingSessions(8.0);
    network.checkNetworkAvailability();

    // Hour 9
    std::cout << "\n  ===== HOUR 9:00 =====\n";
    network.processChargingSessions(9.0);

    // Hour 10
    std::cout << "\n  ===== HOUR 10:00 =====\n";
    network.processChargingSessions(10.0);

    // Hour 11 - complete sessions that have run their time
    std::cout << "\n  ===== HOUR 11:00 =====\n";
    network.processChargingSessions(11.0);

    // ============================================================
    // V2G DEMONSTRATION
    // ============================================================
    std::cout << "\n  ========== VEHICLE-TO-GRID (V2G) DEMO ==========\n";
    // Tesla has been charged - now send power back during peak
    if (teslaY->getSOC() > 0.5) {
        network.initiateV2G("ST001", teslaY, 50.0);
    }

    // ============================================================
    // OFF-PEAK SIMULATION
    // ============================================================
    std::cout << "\n  ===== HOUR 22:00 - OFF-PEAK =====\n";
    User* bilal2 = users[2]; EV* moto = evs[5];
    Booking* b7 = network.bookSlot(bilal2, moto, "ST003", DockType::SLOW, 22.0, 3.0, 0.90);
    network.processChargingSessions(22.0);
    network.processChargingSessions(23.0);
    network.processChargingSessions(24.0);

    // ============================================================
    // DISPLAY VEHICLE STATUS
    // ============================================================
    std::cout << "\n  ========== VEHICLE SOC STATUS ==========\n";
    const std::string vehicleNames[] = {"Tesla Model Y", "BMW i4", "Chery J6", "Nexon EV", "BYD Seal", "Jolta Moto"};
    for (int i = 0; i < (int)evs.size(); i++) {
        std::cout << "  " << vehicleNames[i] << ": SOC = "
                  << std::fixed << std::setprecision(1)
                  << (evs[i]->getSOC() * 100) << "%\n";
    }

    // ============================================================
    // ANALYTICS & REPORTS
    // ============================================================
    std::cout << "\n  ========== USER PROFILES ==========\n";
    for (auto* u : users) {
        u->display();
        std::cout << "\n";
    }

    // Full analytics report
    network.generateFullReport();

    // Multi-language demo
    std::cout << "\n  ========== MULTI-LANGUAGE SUPPORT ==========\n";
    network.setLanguage("UR");
    std::cout << "  اردو انٹرفیس فعال - Urdu Interface Active\n";
    std::cout << "  شارجنگ اسٹیشن دستیاب ہیں - Charging Stations Available\n";
    network.setLanguage("EN");

    std::cout << "\n  ========== SIMULATION COMPLETE ==========\n";
    std::cout << "  Smart EV Charging Network simulation finished successfully.\n";
    std::cout << "  All OOP principles demonstrated:\n";
    std::cout << "    ✓ Encapsulation  (private members, public interfaces)\n";
    std::cout << "    ✓ Inheritance    (EnergySource -> Grid/Solar/Wind)\n";
    std::cout << "    ✓ Polymorphism   (virtual getAvailablePower, updateOutput)\n";
    std::cout << "    ✓ Abstraction    (NetworkManager hides complexity)\n";
    std::cout << "    ✓ Modular Design (separate classes per responsibility)\n\n";

    return 0;
}
