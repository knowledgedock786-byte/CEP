// ============================================================
// SMART EV CHARGING NETWORK MANAGEMENT SYSTEM
// Single File - Paste directly into OnlineGDB
// Compile: g++ -std=c++17 -o ev_system ev_charging_single.cpp
// ============================================================

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <memory>
#include <limits>

using namespace std;


// ============================================================
// EnergySource.h
// ============================================================


// ============================================================
// EnergySource - Base Class (Polymorphism & Inheritance)
// ============================================================
class EnergySource {
protected:
    string name;
    double capacityKW;
    double currentOutputKW;
    bool isRenewable;
    double co2PerKWh; // kg CO2 per kWh

public:
    EnergySource(const string& name, double capacityKW, bool isRenewable, double co2PerKWh)
        : name(name), capacityKW(capacityKW), currentOutputKW(0.0),
          isRenewable(isRenewable), co2PerKWh(co2PerKWh) {}

    virtual ~EnergySource() = default;

    // Pure virtual - must be overridden
    virtual double getAvailablePower() const = 0;
    virtual string getType() const = 0;
    virtual void updateOutput(double hour) = 0;

    // Common getters
    string getName() const { return name; }
    double getCapacity() const { return capacityKW; }
    double getCurrentOutput() const { return currentOutputKW; }
    bool getIsRenewable() const { return isRenewable; }
    double getCO2PerKWh() const { return co2PerKWh; }

    virtual void displayInfo() const {
        cout << "[" << getType() << "] " << name
                  << " | Capacity: " << capacityKW << " kW"
                  << " | Output: " << currentOutputKW << " kW"
                  << " | Renewable: " << (isRenewable ? "Yes" : "No") << "\n";
    }
};

// ============================================================
// GridPower - Derived from EnergySource
// ============================================================
class GridPower : public EnergySource {
private:
    double reliability; // 0.0 - 1.0

public:
    GridPower(double capacityKW = 500.0)
        : EnergySource("City Grid", capacityKW, false, 0.45), reliability(0.99) {
        currentOutputKW = capacityKW; // Grid always available
    }

    double getAvailablePower() const override {
        return capacityKW * reliability;
    }

    string getType() const override { return "GRID"; }

    void updateOutput(double hour) override {
        // Grid has stable output, slight variation during peak hours
        double peakFactor = (hour >= 8 && hour <= 20) ? 0.95 : 1.0;
        currentOutputKW = capacityKW * reliability * peakFactor;
    }
};

// ============================================================
// SolarPower - Derived from EnergySource
// ============================================================
class SolarPower : public EnergySource {
private:
    double panelEfficiency;
    double cloudCoverFactor; // 0.0 = full cloud, 1.0 = clear sky

public:
    SolarPower(double capacityKW = 100.0, double efficiency = 0.20)
        : EnergySource("Solar Farm", capacityKW, true, 0.02),
          panelEfficiency(efficiency), cloudCoverFactor(0.8) {}

    double getAvailablePower() const override {
        return currentOutputKW;
    }

    string getType() const override { return "SOLAR"; }

    void updateOutput(double hour) override {
        // Solar follows sun curve: peak at noon
        double sunIntensity = 0.0;
        if (hour >= 6 && hour <= 20) {
            // Bell curve peaking at hour 13
            double x = (hour - 13.0) / 4.0;
            sunIntensity = exp(-x * x);
        }
        currentOutputKW = capacityKW * panelEfficiency * sunIntensity * cloudCoverFactor * 5.0;
        if (currentOutputKW > capacityKW) currentOutputKW = capacityKW;
    }

    void setCloudCover(double factor) { cloudCoverFactor = factor; }
};

// ============================================================
// WindPower - Derived from EnergySource
// ============================================================
class WindPower : public EnergySource {
private:
    double windSpeedKmh;
    double cutInSpeed;   // Minimum wind speed
    double ratedSpeed;   // Optimal wind speed
    double cutOutSpeed;  // Maximum safe wind speed

public:
    WindPower(double capacityKW = 150.0)
        : EnergySource("Wind Farm", capacityKW, true, 0.01),
          windSpeedKmh(25.0), cutInSpeed(12.0),
          ratedSpeed(45.0), cutOutSpeed(90.0) {}

    double getAvailablePower() const override {
        return currentOutputKW;
    }

    string getType() const override { return "WIND"; }

    void updateOutput(double hour) override {
        // Wind varies throughout the day
        double variation = 1.0 + 0.3 * sin(hour * 0.5);
        windSpeedKmh = 25.0 * variation;

        if (windSpeedKmh < cutInSpeed || windSpeedKmh > cutOutSpeed) {
            currentOutputKW = 0.0;
        } else if (windSpeedKmh >= ratedSpeed) {
            currentOutputKW = capacityKW;
        } else {
            double ratio = (windSpeedKmh - cutInSpeed) / (ratedSpeed - cutInSpeed);
            currentOutputKW = capacityKW * ratio * ratio * ratio; // Cubic relationship
        }
    }

    void setWindSpeed(double speed) { windSpeedKmh = speed; }
    double getWindSpeed() const { return windSpeedKmh; }
};


// ============================================================
// User.h
// ============================================================


// ============================================================
// Enumerations
// ============================================================
enum class VehicleType { SEDAN, SUV, TRUCK, MOTORCYCLE, BUS };
enum class MembershipLevel { BASIC, SILVER, GOLD, PLATINUM };
enum class DockType { SLOW, MEDIUM, FAST };
enum class BookingStatus { PENDING, CONFIRMED, IN_PROGRESS, COMPLETED, CANCELLED };

inline string vehicleTypeStr(VehicleType t) {
    switch(t) {
        case VehicleType::SEDAN:      return "Sedan";
        case VehicleType::SUV:        return "SUV";
        case VehicleType::TRUCK:      return "Truck";
        case VehicleType::MOTORCYCLE: return "Motorcycle";
        case VehicleType::BUS:        return "Bus";
        default: return "Unknown";
    }
}

inline string membershipStr(MembershipLevel m) {
    switch(m) {
        case MembershipLevel::BASIC:    return "Basic";
        case MembershipLevel::SILVER:   return "Silver";
        case MembershipLevel::GOLD:     return "Gold";
        case MembershipLevel::PLATINUM: return "Platinum";
        default: return "Unknown";
    }
}

inline string dockTypeStr(DockType d) {
    switch(d) {
        case DockType::SLOW:   return "Slow (3.7kW)";
        case DockType::MEDIUM: return "Medium (22kW)";
        case DockType::FAST:   return "Fast (150kW)";
        default: return "Unknown";
    }
}

// ============================================================
// EV - Electric Vehicle Class
// ============================================================
class EV {
private:
    string vehicleId;
    string model;
    VehicleType type;
    double batteryCapacityKWh;
    double currentSOC;        // State of Charge: 0.0 to 1.0
    double maxChargingRateKW;
    string licensePlate;

public:
    EV(const string& id, const string& model,
       VehicleType type, double batteryKWh, double soc,
       const string& plate)
        : vehicleId(id), model(model), type(type),
          batteryCapacityKWh(batteryKWh), currentSOC(soc),
          licensePlate(plate) {
        // Max charging rate depends on vehicle type
        switch(type) {
            case VehicleType::MOTORCYCLE: maxChargingRateKW = 11.0;  break;
            case VehicleType::SEDAN:      maxChargingRateKW = 50.0;  break;
            case VehicleType::SUV:        maxChargingRateKW = 100.0; break;
            case VehicleType::TRUCK:      maxChargingRateKW = 150.0; break;
            case VehicleType::BUS:        maxChargingRateKW = 200.0; break;
        }
    }

    // Getters
    string getId() const { return vehicleId; }
    string getModel() const { return model; }
    VehicleType getType() const { return type; }
    double getBatteryCapacity() const { return batteryCapacityKWh; }
    double getSOC() const { return currentSOC; }
    double getMaxChargingRate() const { return maxChargingRateKW; }
    string getLicensePlate() const { return licensePlate; }
    double getEnergyNeeded() const { return batteryCapacityKWh * (1.0 - currentSOC); }

    // Setters
    void setSOC(double soc) {
        currentSOC = (soc > 1.0) ? 1.0 : (soc < 0.0) ? 0.0 : soc;
    }

    // Charge the vehicle - returns energy consumed
    double charge(double powerKW, double durationHours) {
        double energyKWh = powerKW * durationHours;
        double energyNeeded = getEnergyNeeded();
        if (energyKWh > energyNeeded) energyKWh = energyNeeded;
        currentSOC += energyKWh / batteryCapacityKWh;
        if (currentSOC > 1.0) currentSOC = 1.0;
        return energyKWh;
    }

    // Best dock type for this vehicle
    DockType recommendedDockType() const {
        if (currentSOC >= 0.7) return DockType::SLOW;
        if (currentSOC >= 0.4) return DockType::MEDIUM;
        return DockType::FAST;
    }

    void display() const {
        cout << fixed << setprecision(1);
        cout << "  Vehicle: " << model << " [" << vehicleId << "]\n"
                  << "  Type: " << vehicleTypeStr(type) << " | Plate: " << licensePlate << "\n"
                  << "  Battery: " << batteryCapacityKWh << " kWh"
                  << " | SOC: " << (currentSOC * 100) << "%"
                  << " | Needs: " << getEnergyNeeded() << " kWh\n";
    }
};

// ============================================================
// Notification struct
// ============================================================
struct Notification {
    string message;
    string timestamp;
    bool isRead;

    Notification(const string& msg, const string& ts)
        : message(msg), timestamp(ts), isRead(false) {}
};

// ============================================================
// User Class
// ============================================================
class User {
private:
    string userId;
    string name;
    string email;
    string phone;
    MembershipLevel membership;
    int priorityLevel;        // 1 (low) to 10 (high)
    double walletBalance;
    double totalCO2Saved;
    vector<EV*> vehicles;
    vector<Notification> notifications;
    vector<string> bookingHistory;
    double totalPenalties;

public:
    User(const string& id, const string& name,
         const string& email, const string& phone,
         MembershipLevel membership = MembershipLevel::BASIC)
        : userId(id), name(name), email(email), phone(phone),
          membership(membership), walletBalance(0.0),
          totalCO2Saved(0.0), totalPenalties(0.0) {
        // Priority based on membership
        switch(membership) {
            case MembershipLevel::PLATINUM: priorityLevel = 10; break;
            case MembershipLevel::GOLD:     priorityLevel = 7;  break;
            case MembershipLevel::SILVER:   priorityLevel = 5;  break;
            default:                        priorityLevel = 2;  break;
        }
    }

    // Getters
    string getId() const { return userId; }
    string getName() const { return name; }
    string getEmail() const { return email; }
    MembershipLevel getMembership() const { return membership; }
    int getPriorityLevel() const { return priorityLevel; }
    double getWalletBalance() const { return walletBalance; }
    double getTotalCO2Saved() const { return totalCO2Saved; }
    double getPriceDiscount() const {
        switch(membership) {
            case MembershipLevel::PLATINUM: return 0.25;
            case MembershipLevel::GOLD:     return 0.15;
            case MembershipLevel::SILVER:   return 0.08;
            default:                        return 0.0;
        }
    }

    // Vehicle management
    void registerVehicle(EV* ev) {
        vehicles.push_back(ev);
        sendNotification("Vehicle " + ev->getModel() + " [" + ev->getId() + "] registered successfully.");
    }

    EV* getVehicle(const string& vehicleId) {
        for (auto* v : vehicles) {
            if (v->getId() == vehicleId) return v;
        }
        return nullptr;
    }

    vector<EV*>& getVehicles() { return vehicles; }

    // Financial operations
    void topUpWallet(double amount) {
        walletBalance += amount;
        sendNotification("Wallet topped up: PKR " + to_string((int)amount));
    }

    bool deductBalance(double amount) {
        if (walletBalance >= amount) {
            walletBalance -= amount;
            return true;
        }
        return false;
    }

    void addPenalty(double amount) {
        totalPenalties += amount;
        walletBalance -= amount;
        sendNotification("Cancellation penalty applied: PKR " + to_string((int)amount));
    }

    void addCO2Savings(double kg) { totalCO2Saved += kg; }

    // Notifications
    void sendNotification(const string& message) {
        notifications.push_back(Notification(message, "Now"));
        cout << "  [NOTIFICATION -> " << name << "]: " << message << "\n";
    }

    void addBookingToHistory(const string& bookingId) {
        bookingHistory.push_back(bookingId);
    }

    void displayNotifications() const {
        cout << "\n--- Notifications for " << name << " ---\n";
        for (auto& n : notifications) {
            cout << (n.isRead ? "  [READ] " : "  [NEW]  ") << n.message << "\n";
        }
    }

    void display() const {
        cout << fixed << setprecision(2);
        cout << "User: " << name << " [" << userId << "]\n"
                  << "  Email: " << email << " | Phone: " << phone << "\n"
                  << "  Membership: " << membershipStr(membership)
                  << " | Priority: " << priorityLevel << "/10\n"
                  << "  Wallet: PKR " << walletBalance
                  << " | CO2 Saved: " << totalCO2Saved << " kg\n"
                  << "  Vehicles registered: " << vehicles.size() << "\n"
                  << "  Bookings made: " << bookingHistory.size() << "\n";
    }
};


// ============================================================
// Booking.h
// ============================================================


// ============================================================
// Invoice - Digital billing document
// ============================================================
struct Invoice {
    string invoiceId;
    string bookingId;
    string userId;
    string stationName;
    string dockId;
    string vehicleId;
    double energyConsumedKWh;
    double sessionDurationHours;
    double baseRatePerKWh;
    double peakSurcharge;
    double speedPremium;
    double renewableDiscount;
    double membershipDiscount;
    double subtotal;
    double totalAmount;
    string energySource;
    double co2Saved;
    double co2Emitted;
    string sessionStart;
    string sessionEnd;

    Invoice() : energyConsumedKWh(0), sessionDurationHours(0),
                baseRatePerKWh(0), peakSurcharge(0), speedPremium(0),
                renewableDiscount(0), membershipDiscount(0),
                subtotal(0), totalAmount(0), co2Saved(0), co2Emitted(0) {}

    void print() const {
        cout << "\n";
        cout << "  ╔══════════════════════════════════════════════╗\n";
        cout << "  ║         EV CHARGING NETWORK - INVOICE        ║\n";
        cout << "  ╠══════════════════════════════════════════════╣\n";
        cout << "  ║ Invoice ID  : " << left << setw(31) << invoiceId << "║\n";
        cout << "  ║ Booking ID  : " << left << setw(31) << bookingId << "║\n";
        cout << "  ║ Station     : " << left << setw(31) << stationName << "║\n";
        cout << "  ║ Dock        : " << left << setw(31) << dockId << "║\n";
        cout << "  ║ Vehicle     : " << left << setw(31) << vehicleId << "║\n";
        cout << "  ║ Energy Src  : " << left << setw(31) << energySource << "║\n";
        cout << "  ╠══════════════════════════════════════════════╣\n";
        cout << fixed << setprecision(2);
        cout << "  ║ Energy Used : " << left << setw(25) << (to_string(energyConsumedKWh).substr(0,6) + " kWh") << "       ║\n";
        cout << "  ║ Duration    : " << left << setw(25) << (to_string(sessionDurationHours).substr(0,5) + " hrs") << "       ║\n";
        cout << "  ╠══════════════════════════════════════════════╣\n";
        cout << "  ║ BILLING BREAKDOWN                            ║\n";
        cout << "  ║ Base Rate   : PKR " << setw(27) << baseRatePerKWh << "║\n";
        cout << "  ║ Peak Charge : PKR " << setw(27) << peakSurcharge << "║\n";
        cout << "  ║ Speed Fee   : PKR " << setw(27) << speedPremium << "║\n";
        cout << "  ║ Renew Disc  : PKR -" << setw(26) << renewableDiscount << "║\n";
        cout << "  ║ Member Disc : PKR -" << setw(26) << membershipDiscount << "║\n";
        cout << "  ╠══════════════════════════════════════════════╣\n";
        cout << "  ║ TOTAL AMOUNT: PKR " << setw(27) << totalAmount << "║\n";
        cout << "  ╠══════════════════════════════════════════════╣\n";
        cout << "  ║ CO2 Saved   : " << left << setw(25) << (to_string(co2Saved).substr(0,5) + " kg") << "       ║\n";
        cout << "  ║ CO2 Emitted : " << left << setw(25) << (to_string(co2Emitted).substr(0,5) + " kg") << "       ║\n";
        cout << "  ╚══════════════════════════════════════════════╝\n\n";
    }
};

// ============================================================
// Booking Class
// ============================================================
class Booking {
private:
    string bookingId;
    string userId;
    string vehicleId;
    string stationId;
    string dockId;
    DockType requestedDockType;
    BookingStatus status;
    double startHour;
    double endHour;
    double targetSOC;      // Desired final SOC
    double energyDelivered;
    bool isDeferrable;     // Can be pushed to off-peak?
    int priorityScore;
    Invoice generatedInvoice;
    bool hasInvoice;
    double cancellationPenalty;
    string scheduledTime;

public:
    Booking(const string& bId, const string& uId,
            const string& vId, const string& sId,
            DockType dType, double startH, double endH,
            double targetSOC, int priority)
        : bookingId(bId), userId(uId), vehicleId(vId), stationId(sId),
          dockId(""), requestedDockType(dType), status(BookingStatus::PENDING),
          startHour(startH), endHour(endH), targetSOC(targetSOC),
          energyDelivered(0.0), isDeferrable(true),
          priorityScore(priority), hasInvoice(false),
          cancellationPenalty(50.0) {}

    // Getters
    string getId() const { return bookingId; }
    string getUserId() const { return userId; }
    string getVehicleId() const { return vehicleId; }
    string getStationId() const { return stationId; }
    string getDockId() const { return dockId; }
    DockType getDockType() const { return requestedDockType; }
    BookingStatus getStatus() const { return status; }
    double getStartHour() const { return startHour; }
    double getEndHour() const { return endHour; }
    double getDuration() const { return endHour - startHour; }
    double getTargetSOC() const { return targetSOC; }
    double getEnergyDelivered() const { return energyDelivered; }
    int getPriority() const { return priorityScore; }
    bool getIsDeferrable() const { return isDeferrable; }
    double getCancellationPenalty() const { return cancellationPenalty; }

    // Setters
    void setDockId(const string& id) { dockId = id; }
    void setStatus(BookingStatus s) { status = s; }
    void setEnergyDelivered(double e) { energyDelivered = e; }
    void setIsDeferrable(bool d) { isDeferrable = d; }
    void setStartHour(double h) { startHour = h; }
    void setEndHour(double h) { endHour = h; }
    void setInvoice(const Invoice& inv) { generatedInvoice = inv; hasInvoice = true; }

    const Invoice& getInvoice() const { return generatedInvoice; }
    bool hasGeneratedInvoice() const { return hasInvoice; }

    string getStatusStr() const {
        switch(status) {
            case BookingStatus::PENDING:     return "PENDING";
            case BookingStatus::CONFIRMED:   return "CONFIRMED";
            case BookingStatus::IN_PROGRESS: return "IN_PROGRESS";
            case BookingStatus::COMPLETED:   return "COMPLETED";
            case BookingStatus::CANCELLED:   return "CANCELLED";
            default: return "UNKNOWN";
        }
    }

    // Defer booking to off-peak hours (e.g., move to after 22:00)
    void deferToOffPeak() {
        double duration = endHour - startHour;
        startHour = 22.0;
        endHour = startHour + duration;
        cout << "  [DEFER] Booking " << bookingId
                  << " deferred to off-peak: " << startHour << ":00 - " << endHour << ":00\n";
    }

    void display() const {
        cout << fixed << setprecision(1);
        cout << "  Booking [" << bookingId << "] Status: " << getStatusStr() << "\n"
                  << "    User: " << userId << " | Vehicle: " << vehicleId << "\n"
                  << "    Station: " << stationId << " | Dock: "
                  << (dockId.empty() ? "Not assigned" : dockId) << "\n"
                  << "    Time: " << startHour << ":00 - " << endHour << ":00"
                  << " | Dock Type: " << dockTypeStr(requestedDockType) << "\n"
                  << "    Target SOC: " << (targetSOC * 100) << "%"
                  << " | Priority: " << priorityScore << "\n";
    }
};


// ============================================================
// ChargingDock.h
// ============================================================


// ============================================================
// ChargingDock Class
// ============================================================
class ChargingDock {
private:
    string dockId;
    DockType type;
    double powerRatingKW;
    bool isOccupied;
    bool isOnline;
    string currentBookingId;
    string currentVehicleId;
    double totalEnergyDeliveredKWh;
    double totalSessionTime;
    int totalSessions;
    double currentSessionEnergy;

    // Maintenance tracking
    bool needsMaintenance;
    int sessionsUntilMaintenance;

public:
    ChargingDock(const string& id, DockType type)
        : dockId(id), type(type), isOccupied(false), isOnline(true),
          totalEnergyDeliveredKWh(0.0), totalSessionTime(0.0),
          totalSessions(0), currentSessionEnergy(0.0),
          needsMaintenance(false), sessionsUntilMaintenance(50) {
        switch(type) {
            case DockType::SLOW:   powerRatingKW = 3.7;  break;
            case DockType::MEDIUM: powerRatingKW = 22.0; break;
            case DockType::FAST:   powerRatingKW = 150.0; break;
        }
    }

    // Getters
    string getId() const { return dockId; }
    DockType getType() const { return type; }
    double getPowerRating() const { return powerRatingKW; }
    bool getIsOccupied() const { return isOccupied; }
    bool getIsOnline() const { return isOnline; }
    bool isAvailable() const { return isOnline && !isOccupied && !needsMaintenance; }
    double getTotalEnergy() const { return totalEnergyDeliveredKWh; }
    int getTotalSessions() const { return totalSessions; }
    double getAvgSessionTime() const {
        return (totalSessions > 0) ? totalSessionTime / totalSessions : 0.0;
    }

    // Assign booking to this dock
    bool assignSession(const string& bookingId, const string& vehicleId) {
        if (!isAvailable()) return false;
        isOccupied = true;
        currentBookingId = bookingId;
        currentVehicleId = vehicleId;
        currentSessionEnergy = 0.0;
        totalSessions++;
        sessionsUntilMaintenance--;
        if (sessionsUntilMaintenance <= 0) {
            needsMaintenance = true;
            cout << "  [MAINTENANCE ALERT] Dock " << dockId << " requires maintenance!\n";
        }
        return true;
    }

    // Simulate charging for a time step
    double chargeStep(EV* vehicle, double durationHours) {
        if (!isOccupied || !vehicle) return 0.0;

        double effectivePower = min(powerRatingKW, vehicle->getMaxChargingRate());
        double energyDelivered = vehicle->charge(effectivePower, durationHours);
        currentSessionEnergy += energyDelivered;
        totalEnergyDeliveredKWh += energyDelivered;
        totalSessionTime += durationHours;
        return energyDelivered;
    }

    // Release dock after session
    void endSession() {
        isOccupied = false;
        currentBookingId = "";
        currentVehicleId = "";
        currentSessionEnergy = 0.0;
    }

    void performMaintenance() {
        needsMaintenance = false;
        sessionsUntilMaintenance = 50;
        cout << "  [MAINTENANCE] Dock " << dockId << " maintenance complete. Back online.\n";
    }

    void setOffline() { isOnline = false; }
    void setOnline()  { isOnline = true; }

    string getCurrentBookingId() const { return currentBookingId; }

    // Get effective power for a specific vehicle
    double getEffectivePower(const EV* vehicle) const {
        if (!vehicle) return powerRatingKW;
        return min(powerRatingKW, vehicle->getMaxChargingRate());
    }

    void display() const {
        string statusStr;
        if (!isOnline)          statusStr = "OFFLINE";
        else if (needsMaintenance) statusStr = "MAINTENANCE";
        else if (isOccupied)    statusStr = "OCCUPIED";
        else                    statusStr = "AVAILABLE";

        cout << fixed << setprecision(1);
        cout << "    Dock [" << dockId << "] "
                  << dockTypeStr(type) << " | " << powerRatingKW << " kW"
                  << " | Status: " << statusStr
                  << " | Sessions: " << totalSessions
                  << " | Energy: " << totalEnergyDeliveredKWh << " kWh\n";
    }
};


// ============================================================
// ChargingStation.h
// ============================================================


// ============================================================
// ChargingStation Class
// ============================================================
class ChargingStation {
private:
    string stationId;
    string name;
    string address;
    double latitude;
    double longitude;
    bool isOperational;

    vector<ChargingDock> docks;
    vector<EnergySource*> energySources;

    // Energy management
    double maxCapacityKW;
    double currentLoadKW;
    double peakLoadKW;
    double totalEnergyDeliveredKWh;
    double renewableEnergyUsedKWh;
    double gridEnergyUsedKWh;

    // Load shedding threshold
    double loadSheddingThreshold; // fraction of max capacity
    bool isLoadShedding;

    // Revenue tracking
    double totalRevenue;
    int totalBookingsCompleted;

    // Analytics
    vector<double> hourlyLoadHistory;
    vector<double> hourlyRevenueHistory;

public:
    ChargingStation(const string& id, const string& name,
                    const string& address, double lat, double lon,
                    double maxCapKW = 300.0)
        : stationId(id), name(name), address(address),
          latitude(lat), longitude(lon), isOperational(true),
          maxCapacityKW(maxCapKW), currentLoadKW(0.0), peakLoadKW(0.0),
          totalEnergyDeliveredKWh(0.0), renewableEnergyUsedKWh(0.0),
          gridEnergyUsedKWh(0.0), loadSheddingThreshold(0.90),
          isLoadShedding(false), totalRevenue(0.0), totalBookingsCompleted(0) {
        hourlyLoadHistory.resize(24, 0.0);
        hourlyRevenueHistory.resize(24, 0.0);
    }

    // ---- Dock Management ----
    void addDock(DockType type) {
        string dockId = stationId + "-D" + to_string(docks.size() + 1);
        docks.push_back(ChargingDock(dockId, type));
    }

    void addDocks(int slow, int medium, int fast) {
        for (int i = 0; i < slow;   i++) addDock(DockType::SLOW);
        for (int i = 0; i < medium; i++) addDock(DockType::MEDIUM);
        for (int i = 0; i < fast;   i++) addDock(DockType::FAST);
    }

    // ---- Energy Source Management ----
    void addEnergySource(EnergySource* src) {
        energySources.push_back(src);
    }

    // ---- Load Balancing ----
    double getTotalAvailablePower() {
        double total = 0.0;
        for (auto* src : energySources) total += src->getAvailablePower();
        return min(total, maxCapacityKW);
    }

    double getRenewablePower() {
        double total = 0.0;
        for (auto* src : energySources) {
            if (src->getIsRenewable()) total += src->getAvailablePower();
        }
        return total;
    }

    // Check if load shedding required
    bool checkLoadShedding() {
        if (currentLoadKW >= maxCapacityKW * loadSheddingThreshold) {
            isLoadShedding = true;
            cout << "  [LOAD SHED] Station " << name
                      << " - Load shedding activated! Current: "
                      << currentLoadKW << "/" << maxCapacityKW << " kW\n";
            return true;
        }
        isLoadShedding = false;
        return false;
    }

    bool canAcceptLoad(double additionalKW) const {
        return (currentLoadKW + additionalKW) <= maxCapacityKW * loadSheddingThreshold;
    }

    void addLoad(double kw) {
        currentLoadKW += kw;
        if (currentLoadKW > peakLoadKW) peakLoadKW = currentLoadKW;
        checkLoadShedding();
    }

    void removeLoad(double kw) {
        currentLoadKW = max(0.0, currentLoadKW - kw);
        if (isLoadShedding && currentLoadKW < maxCapacityKW * 0.75) {
            isLoadShedding = false;
            cout << "  [LOAD SHED] Station " << name << " - Load shedding deactivated.\n";
        }
    }

    // Update energy sources based on time of day
    void updateEnergySources(double hour) {
        for (auto* src : energySources) src->updateOutput(hour);
        hourlyLoadHistory[(int)hour % 24] = currentLoadKW;
    }

    // ---- Dock Assignment (Priority-Based) ----
    ChargingDock* findBestDock(DockType preferredType, double vehicleMaxKW) {
        // First pass: exact type match
        for (auto& dock : docks) {
            if (dock.isAvailable() && dock.getType() == preferredType
                && canAcceptLoad(dock.getPowerRating())) {
                return &dock;
            }
        }
        // Second pass: any available dock that fits power budget
        for (auto& dock : docks) {
            if (dock.isAvailable() && canAcceptLoad(dock.getPowerRating())) {
                return &dock;
            }
        }
        return nullptr;
    }

    bool assignDockToBooking(Booking& booking, EV* vehicle) {
        ChargingDock* dock = findBestDock(booking.getDockType(), vehicle->getMaxChargingRate());
        if (!dock) return false;

        dock->assignSession(booking.getId(), vehicle->getId());
        booking.setDockId(dock->getId());
        booking.setStatus(BookingStatus::IN_PROGRESS);
        addLoad(dock->getPowerRating());
        return true;
    }

    // ---- Session Simulation ----
    double simulateChargingStep(Booking& booking, EV* vehicle, double durationHours,
                                 bool useRenewable) {
        // Find dock
        for (auto& dock : docks) {
            if (dock.getId() == booking.getDockId()) {
                double energy = dock.chargeStep(vehicle, durationHours);
                totalEnergyDeliveredKWh += energy;
                if (useRenewable) renewableEnergyUsedKWh += energy;
                else              gridEnergyUsedKWh += energy;
                return energy;
            }
        }
        return 0.0;
    }

    void releaseDocK(const string& dockId, double dockPower) {
        for (auto& dock : docks) {
            if (dock.getId() == dockId) {
                dock.endSession();
                removeLoad(dockPower);
                totalBookingsCompleted++;
                return;
            }
        }
    }

    // ---- Availability Report ----
    void checkAvailability() const {
        int avail = 0, occupied = 0, offline = 0;
        for (const auto& dock : docks) {
            if (!dock.getIsOnline()) offline++;
            else if (dock.getIsOccupied()) occupied++;
            else avail++;
        }
        cout << "  Station: " << name << "\n"
                  << "    Available: " << avail
                  << " | Occupied: " << occupied
                  << " | Offline: " << offline
                  << " | Total: " << docks.size() << "\n"
                  << "    Current Load: " << currentLoadKW << "/" << maxCapacityKW << " kW ("
                  << (int)(currentLoadKW / maxCapacityKW * 100) << "%)\n";
    }

    // ---- Revenue ----
    void addRevenue(double amount) {
        totalRevenue += amount;
    }

    // ---- Getters ----
    string getId() const { return stationId; }
    string getName() const { return name; }
    string getAddress() const { return address; }
    bool getIsOperational() const { return isOperational; }
    double getCurrentLoad() const { return currentLoadKW; }
    double getMaxCapacity() const { return maxCapacityKW; }
    double getPeakLoad() const { return peakLoadKW; }
    double getTotalEnergyDelivered() const { return totalEnergyDeliveredKWh; }
    double getRenewableEnergyUsed() const { return renewableEnergyUsedKWh; }
    double getGridEnergyUsed() const { return gridEnergyUsedKWh; }
    double getTotalRevenue() const { return totalRevenue; }
    int getTotalBookings() const { return totalBookingsCompleted; }
    bool getIsLoadShedding() const { return isLoadShedding; }
    vector<ChargingDock>& getDocks() { return docks; }
    const vector<double>& getHourlyLoad() const { return hourlyLoadHistory; }

    double getUtilizationRate() const {
        int total = docks.size();
        if (total == 0) return 0.0;
        int occupied = 0;
        for (const auto& d : docks) if (d.getIsOccupied()) occupied++;
        return (double)occupied / total * 100.0;
    }

    double getRenewableRatio() const {
        double total = renewableEnergyUsedKWh + gridEnergyUsedKWh;
        if (total == 0) return 0.0;
        return renewableEnergyUsedKWh / total * 100.0;
    }

    // V2G: Vehicle to Grid - vehicle sends power back
    double executeV2G(EV* vehicle, double kw, double durationHours) {
        if (!vehicle || vehicle->getSOC() < 0.3) return 0.0; // Don't drain below 30%
        double maxExportable = vehicle->getBatteryCapacity() * (vehicle->getSOC() - 0.3);
        double exported = min(kw * durationHours, maxExportable);
        vehicle->setSOC(vehicle->getSOC() - exported / vehicle->getBatteryCapacity());
        gridEnergyUsedKWh = max(0.0, gridEnergyUsedKWh - exported);
        cout << "  [V2G] Vehicle " << vehicle->getId() << " exported "
                  << exported << " kWh back to grid.\n";
        return exported;
    }

    void display() const {
        cout << fixed << setprecision(1);
        cout << "\nStation: " << name << " [" << stationId << "]\n"
                  << "  Address: " << address << "\n"
                  << "  Docks: " << docks.size()
                  << " | Load: " << currentLoadKW << "/" << maxCapacityKW << " kW\n"
                  << "  Total Energy: " << totalEnergyDeliveredKWh << " kWh"
                  << " | Revenue: PKR " << totalRevenue << "\n"
                  << "  Renewable: " << getRenewableRatio() << "% of energy\n";
        for (const auto& dock : docks) dock.display();
        cout << "  Energy Sources:\n";
        for (auto* src : energySources) src->displayInfo();
    }
};


// ============================================================
// PricingEngine.h
// ============================================================


// ============================================================
// PricingEngine - Dynamic Pricing and Billing
// ============================================================
class PricingEngine {
private:
    // Base rates (PKR per kWh)
    double baseRatePKR;        // Standard rate
    double peakMultiplier;     // Peak hour surcharge factor
    double offPeakDiscount;    // Off-peak discount factor

    // Speed premiums
    double slowPremium;
    double mediumPremium;
    double fastPremium;

    // Renewable discount
    double renewableDiscount;  // fraction discount for green energy

    // Peak hours definition
    double peakStartHour;
    double peakEndHour;

    static int invoiceCounter;

public:
    PricingEngine()
        : baseRatePKR(85.0),       // PKR 85/kWh base
          peakMultiplier(1.50),    // 50% surcharge peak
          offPeakDiscount(0.80),   // 20% off during off-peak
          slowPremium(0.0),
          mediumPremium(10.0),
          fastPremium(25.0),
          renewableDiscount(0.12),  // 12% off for renewables
          peakStartHour(8.0),
          peakEndHour(21.0) {}

    bool isPeakHour(double hour) const {
        return (hour >= peakStartHour && hour < peakEndHour);
    }

    double calculateRatePerKWh(DockType dockType, double hour,
                                bool isRenewable, MembershipLevel membership) const {
        double rate = baseRatePKR;

        // Time-of-day adjustment
        if (isPeakHour(hour)) {
            rate *= peakMultiplier;
        } else {
            rate *= offPeakDiscount;
        }

        // Speed premium
        switch(dockType) {
            case DockType::MEDIUM: rate += mediumPremium; break;
            case DockType::FAST:   rate += fastPremium;   break;
            default: break;
        }

        // Renewable discount
        if (isRenewable) rate *= (1.0 - renewableDiscount);

        return rate;
    }

    // Full invoice generation
    Invoice generateInvoice(
        const string& bookingId,
        const string& stationName,
        const string& dockId,
        const string& vehicleId,
        const User& user,
        DockType dockType,
        double energyKWh,
        double durationHours,
        double startHour,
        bool isRenewable,
        double gridCO2Factor = 0.45,    // kg CO2 per kWh grid
        double renewCO2Factor = 0.02    // kg CO2 per kWh renewable
    ) {
        Invoice inv;
        inv.invoiceId  = "INV-" + to_string(++invoiceCounter);
        inv.bookingId  = bookingId;
        inv.userId     = user.getId();
        inv.stationName = stationName;
        inv.dockId     = dockId;
        inv.vehicleId  = vehicleId;
        inv.energyConsumedKWh = energyKWh;
        inv.sessionDurationHours = durationHours;
        inv.energySource = isRenewable ? "Renewable (Solar/Wind)" : "Grid";
        inv.sessionStart = to_string((int)startHour) + ":00";
        inv.sessionEnd   = to_string((int)(startHour + durationHours)) + ":00";

        // Base rate calculation
        double baseRate = baseRatePKR * energyKWh;
        inv.baseRatePerKWh = baseRate;

        // Peak surcharge
        double peakSurcharge = 0.0;
        if (isPeakHour(startHour)) {
            peakSurcharge = baseRate * (peakMultiplier - 1.0);
        } else {
            peakSurcharge = -baseRate * (1.0 - offPeakDiscount); // negative = discount
        }
        inv.peakSurcharge = peakSurcharge;

        // Speed premium
        double speedPrem = 0.0;
        switch(dockType) {
            case DockType::MEDIUM: speedPrem = mediumPremium * durationHours; break;
            case DockType::FAST:   speedPrem = fastPremium   * durationHours; break;
            default: break;
        }
        inv.speedPremium = speedPrem;

        // Renewable discount
        double renDiscount = isRenewable ? (baseRate * renewableDiscount) : 0.0;
        inv.renewableDiscount = renDiscount;

        // Membership discount
        double memberDiscount = (baseRate + peakSurcharge + speedPrem - renDiscount)
                                * user.getPriceDiscount();
        inv.membershipDiscount = memberDiscount;

        // Subtotal
        inv.subtotal = baseRate + peakSurcharge + speedPrem;
        inv.totalAmount = max(0.0, inv.subtotal - renDiscount - memberDiscount);

        // CO2 calculations
        if (isRenewable) {
            inv.co2Emitted = energyKWh * renewCO2Factor;
            inv.co2Saved   = energyKWh * (gridCO2Factor - renewCO2Factor);
        } else {
            inv.co2Emitted = energyKWh * gridCO2Factor;
            inv.co2Saved   = 0.0;
        }

        return inv;
    }

    void displayRates(double hour) const {
        cout << fixed << setprecision(2);
        cout << "\n--- Current Pricing (Hour: " << hour << ") ---\n";
        cout << "  Time: " << (isPeakHour(hour) ? "PEAK" : "OFF-PEAK") << "\n";
        cout << "  Slow   Dock (Grid):      PKR "
                  << calculateRatePerKWh(DockType::SLOW,   hour, false, MembershipLevel::BASIC) << "/kWh\n";
        cout << "  Medium Dock (Grid):      PKR "
                  << calculateRatePerKWh(DockType::MEDIUM, hour, false, MembershipLevel::BASIC) << "/kWh\n";
        cout << "  Fast   Dock (Grid):      PKR "
                  << calculateRatePerKWh(DockType::FAST,   hour, false, MembershipLevel::BASIC) << "/kWh\n";
        cout << "  Fast   Dock (Renewable): PKR "
                  << calculateRatePerKWh(DockType::FAST,   hour, true,  MembershipLevel::BASIC) << "/kWh\n";
    }
};

int PricingEngine::invoiceCounter = 0;


// ============================================================
// AnalyticsEngine.h
// ============================================================


// ============================================================
// AnalyticsEngine - Reporting and Insights
// ============================================================
class AnalyticsEngine {
private:
    vector<ChargingStation*>& stations;
    vector<Booking*>& bookings;
    vector<User*>& users;

    // Revenue by hour
    map<int, double> revenueByHour;
    map<string, double> revenueByStation;
    map<string, int> bookingsByUser;

public:
    AnalyticsEngine(vector<ChargingStation*>& s,
                    vector<Booking*>& b,
                    vector<User*>& u)
        : stations(s), bookings(b), users(u) {}

    // ---- Station Utilization Report ----
    void generateStationReport() const {
        cout << "\n";
        cout << "  ╔══════════════════════════════════════════════════════════╗\n";
        cout << "  ║              STATION UTILIZATION REPORT                  ║\n";
        cout << "  ╠══════════════════════════════════════════════════════════╣\n";

        for (auto* station : stations) {
            cout << fixed << setprecision(1);
            cout << "  ║ " << left << setw(20) << station->getName()
                      << " Util: " << setw(5) << station->getUtilizationRate() << "%"
                      << "  Peak: " << setw(6) << station->getPeakLoad() << "kW   ║\n";
            cout << "  ║   Energy: " << setw(8) << station->getTotalEnergyDelivered()
                      << "kWh  Revenue: PKR " << setw(8) << station->getTotalRevenue()
                      << "         ║\n";
            cout << "  ║   Renewable: " << setw(5) << station->getRenewableRatio()
                      << "%  Bookings: " << setw(5) << station->getTotalBookings()
                      << "                    ║\n";
            cout << "  ╠══════════════════════════════════════════════════════════╣\n";
        }
        cout << "  ╚══════════════════════════════════════════════════════════╝\n";
    }

    // ---- Session Analytics ----
    void generateSessionReport() const {
        cout << "\n";
        cout << "  ╔══════════════════════════════════════════════════════════╗\n";
        cout << "  ║                SESSION ANALYTICS REPORT                  ║\n";
        cout << "  ╠══════════════════════════════════════════════════════════╣\n";

        int total = bookings.size();
        int completed = 0, cancelled = 0, inProgress = 0;
        double totalEnergy = 0, totalDuration = 0;

        for (auto* b : bookings) {
            switch(b->getStatus()) {
                case BookingStatus::COMPLETED:   completed++;   break;
                case BookingStatus::CANCELLED:   cancelled++;   break;
                case BookingStatus::IN_PROGRESS: inProgress++;  break;
                default: break;
            }
            totalEnergy   += b->getEnergyDelivered();
            totalDuration += b->getDuration();
        }

        double avgDuration = total > 0 ? totalDuration / total : 0;
        double avgEnergy   = total > 0 ? totalEnergy   / total : 0;
        double completionRate = total > 0 ? (double)completed / total * 100.0 : 0;

        cout << "  ║ Total Bookings    : " << setw(36) << total << "║\n";
        cout << "  ║ Completed         : " << setw(36) << completed << "║\n";
        cout << "  ║ In Progress       : " << setw(36) << inProgress << "║\n";
        cout << "  ║ Cancelled         : " << setw(36) << cancelled << "║\n";
        cout << fixed << setprecision(2);
        cout << "  ║ Avg Duration (hr) : " << setw(36) << avgDuration << "║\n";
        cout << "  ║ Avg Energy (kWh)  : " << setw(36) << avgEnergy << "║\n";
        cout << "  ║ Completion Rate   : " << setw(34) << completionRate << "% ║\n";
        cout << "  ╚══════════════════════════════════════════════════════════╝\n";
    }

    // ---- Renewable vs Grid Report ----
    void generateEnergyReport() const {
        double totalRenewable = 0, totalGrid = 0;
        for (auto* s : stations) {
            totalRenewable += s->getRenewableEnergyUsed();
            totalGrid      += s->getGridEnergyUsed();
        }
        double total = totalRenewable + totalGrid;
        double renewRatio = total > 0 ? totalRenewable / total * 100.0 : 0;

        // CO2 estimates
        double co2Emitted = totalGrid * 0.45;
        double co2Saved   = totalRenewable * (0.45 - 0.02);

        cout << "\n";
        cout << "  ╔══════════════════════════════════════════════════════════╗\n";
        cout << "  ║            ENERGY & ENVIRONMENTAL REPORT                 ║\n";
        cout << "  ╠══════════════════════════════════════════════════════════╣\n";
        cout << fixed << setprecision(2);
        cout << "  ║ Total Energy Delivered : " << setw(31) << total << "kWh ║\n";
        cout << "  ║ Renewable Energy Used  : " << setw(31) << totalRenewable << "kWh ║\n";
        cout << "  ║ Grid Energy Used       : " << setw(31) << totalGrid << "kWh ║\n";
        cout << "  ║ Renewable Ratio        : " << setw(30) << renewRatio << "% ║\n";
        cout << "  ╠══════════════════════════════════════════════════════════╣\n";
        cout << "  ║ CO2 Emitted (Grid)     : " << setw(30) << co2Emitted << "kg ║\n";
        cout << "  ║ CO2 Saved (Renewable)  : " << setw(30) << co2Saved << "kg ║\n";
        cout << "  ║ Net Environmental Gain : " << setw(30) << (co2Saved - co2Emitted) << "kg ║\n";

        // Visualize renewable ratio
        cout << "  ╠══════════════════════════════════════════════════════════╣\n";
        cout << "  ║ Renewable Usage: [";
        int filled = (int)(renewRatio / 2.5); // Scale to 40 chars
        for (int i = 0; i < 40; i++) cout << (i < filled ? "█" : "░");
        cout << "] " << (int)renewRatio << "% ║\n";
        cout << "  ╚══════════════════════════════════════════════════════════╝\n";
    }

    // ---- Revenue Report ----
    void generateRevenueReport() const {
        double totalRev = 0;
        for (auto* s : stations) totalRev += s->getTotalRevenue();

        // Revenue per station
        cout << "\n";
        cout << "  ╔══════════════════════════════════════════════════════════╗\n";
        cout << "  ║                   REVENUE REPORT                         ║\n";
        cout << "  ╠══════════════════════════════════════════════════════════╣\n";
        cout << fixed << setprecision(2);
        for (auto* s : stations) {
            double pct = totalRev > 0 ? s->getTotalRevenue() / totalRev * 100.0 : 0;
            cout << "  ║ " << left << setw(22) << s->getName()
                      << ": PKR " << right << setw(10) << s->getTotalRevenue()
                      << " (" << setw(5) << pct << "%)    ║\n";
        }
        cout << "  ╠══════════════════════════════════════════════════════════╣\n";
        cout << "  ║ TOTAL NETWORK REVENUE: PKR " << setw(29) << totalRev << "║\n";
        cout << "  ╚══════════════════════════════════════════════════════════╝\n";
    }

    // ---- User Demand Trends ----
    void generateUserReport() const {
        cout << "\n";
        cout << "  ╔══════════════════════════════════════════════════════════╗\n";
        cout << "  ║                USER DEMAND TRENDS                        ║\n";
        cout << "  ╠══════════════════════════════════════════════════════════╣\n";

        map<MembershipLevel, int> memberDist;
        for (auto* u : users) memberDist[u->getMembership()]++;

        cout << "  ║ Total Users       : " << setw(36) << users.size() << "║\n";
        cout << "  ║ Basic Members     : " << setw(36) << memberDist[MembershipLevel::BASIC] << "║\n";
        cout << "  ║ Silver Members    : " << setw(36) << memberDist[MembershipLevel::SILVER] << "║\n";
        cout << "  ║ Gold Members      : " << setw(36) << memberDist[MembershipLevel::GOLD] << "║\n";
        cout << "  ║ Platinum Members  : " << setw(36) << memberDist[MembershipLevel::PLATINUM] << "║\n";

        double totalCO2Saved = 0;
        for (auto* u : users) totalCO2Saved += u->getTotalCO2Saved();
        cout << fixed << setprecision(2);
        cout << "  ║ Total CO2 Saved   : " << setw(33) << totalCO2Saved << " kg ║\n";
        cout << "  ╚══════════════════════════════════════════════════════════╝\n";
    }

    // ---- Hourly Load Visualization ----
    void displayHourlyLoadChart(ChargingStation* station) const {
        const auto& hourly = station->getHourlyLoad();
        double maxLoad = *max_element(hourly.begin(), hourly.end());
        if (maxLoad == 0) maxLoad = 1;

        cout << "\n  Hourly Load Chart - " << station->getName() << "\n";
        cout << "  " << string(52, '-') << "\n";

        for (int h = 0; h < 24; h++) {
            int bars = (int)(hourly[h] / maxLoad * 30);
            cout << "  " << setw(2) << h << ":00 |";
            for (int b = 0; b < 30; b++) {
                if (b < bars) {
                    // Peak hour styling
                    if (h >= 8 && h < 21) cout << "▓";
                    else cout << "░";
                } else {
                    cout << " ";
                }
            }
            cout << "| " << fixed << setprecision(0) << hourly[h] << "kW\n";
        }
        cout << "  " << string(52, '-') << "\n";
        cout << "  ▓ = Peak Hour  ░ = Off-Peak\n";
    }

    // ---- Full System Report ----
    void generateFullReport() const {
        cout << "\n\n";
        cout << "  ════════════════════════════════════════════════════════════\n";
        cout << "  ██████   SMART EV CHARGING NETWORK - FULL SYSTEM REPORT   ██\n";
        cout << "  ════════════════════════════════════════════════════════════\n";
        generateStationReport();
        generateSessionReport();
        generateEnergyReport();
        generateRevenueReport();
        generateUserReport();
        if (!stations.empty()) {
            displayHourlyLoadChart(stations[0]);
        }
    }
};


// ============================================================
// NetworkManager.h
// ============================================================


// ============================================================
// NetworkManager - Orchestrates the entire EV Charging Network
// ============================================================
class NetworkManager {
private:
    string networkName;
    string cityName;
    double currentHour;

    vector<ChargingStation*>  stations;
    vector<User*>             users;
    vector<Booking*>          bookings;
    vector<EnergySource*>     energySources;

    PricingEngine  pricing;
    AnalyticsEngine* analytics;

    // ID counters
    static int bookingCounter;
    static int userCounter;
    static int vehicleCounter;

    // Booking queue (sorted by priority)
    vector<Booking*> pendingQueue;

    // Multi-language support
    string language; // "EN", "UR", "AR"

public:
    NetworkManager(const string& name, const string& city)
        : networkName(name), cityName(city), currentHour(8.0), language("EN") {
        analytics = new AnalyticsEngine(stations, bookings, users);
        printBanner();
    }

    ~NetworkManager() {
        delete analytics;
        for (auto* s : stations)      delete s;
        for (auto* u : users)         delete u;
        for (auto* b : bookings)      delete b;
        for (auto* e : energySources) delete e;
    }

    // ============================================================
    // INFRASTRUCTURE SETUP
    // ============================================================
    void addStation(ChargingStation* station) {
        stations.push_back(station);
        cout << "  [NETWORK] Station added: " << station->getName() << "\n";
    }

    ChargingStation* findStation(const string& id) {
        for (auto* s : stations) if (s->getId() == id) return s;
        return nullptr;
    }

    // ============================================================
    // USER MANAGEMENT
    // ============================================================
    User* registerUser(const string& name, const string& email,
                       const string& phone,
                       MembershipLevel membership = MembershipLevel::BASIC) {
        string uid = "U" + to_string(++userCounter);
        User* u = new User(uid, name, email, phone, membership);
        users.push_back(u);
        cout << "\n  [REGISTER] User registered: " << name
                  << " [" << uid << "] - " << membershipStr(membership) << " member\n";
        return u;
    }

    EV* registerVehicle(User* user, const string& model,
                         VehicleType type, double batteryKWh,
                         double soc, const string& plate) {
        string vid = "V" + to_string(++vehicleCounter);
        EV* ev = new EV(vid, model, type, batteryKWh, soc, plate);
        user->registerVehicle(ev);
        cout << "  [REGISTER] Vehicle: " << model << " [" << vid << "]"
                  << " SOC: " << (soc * 100) << "% registered to " << user->getName() << "\n";
        return ev;
    }

    User* findUser(const string& id) {
        for (auto* u : users) if (u->getId() == id) return u;
        return nullptr;
    }

    // ============================================================
    // BOOKING MANAGEMENT
    // ============================================================
    Booking* bookSlot(User* user, EV* vehicle, const string& stationId,
                       DockType dockType, double startHour, double durationHours,
                       double targetSOC = 0.9) {
        cout << "\n  [BOOKING] " << user->getName() << " requesting slot...\n";

        // Validate
        ChargingStation* station = findStation(stationId);
        if (!station) {
            cout << "  [ERROR] Station " << stationId << " not found!\n";
            return nullptr;
        }

        if (!station->getIsOperational()) {
            cout << "  [ERROR] Station " << stationId << " is not operational!\n";
            return nullptr;
        }

        // Check wallet balance (rough estimate)
        double estimatedCost = 200.0; // Min estimate
        if (user->getWalletBalance() < estimatedCost) {
            cout << "  [WARNING] Low wallet balance. Current: PKR "
                      << user->getWalletBalance() << "\n";
        }

        string bid = "B" + to_string(++bookingCounter);
        Booking* booking = new Booking(
            bid, user->getId(), vehicle->getId(),
            stationId, dockType, startHour,
            startHour + durationHours, targetSOC,
            user->getPriorityLevel()
        );

        // Check peak hour load shedding - defer if needed
        if (pricing.isPeakHour(startHour) && station->getIsLoadShedding()) {
            cout << "  [DEFER] Peak-hour overload. Checking if deferrable...\n";
            booking->deferToOffPeak();
        }

        bookings.push_back(booking);
        pendingQueue.push_back(booking);
        sortPendingQueue();
        user->addBookingToHistory(bid);

        cout << "  [BOOKING CONFIRMED] ID: " << bid
                  << " | Station: " << station->getName()
                  << " | " << dockTypeStr(dockType)
                  << " | " << startHour << ":00-" << (startHour + durationHours) << ":00\n";

        user->sendNotification("Booking " + bid + " confirmed at " + station->getName() +
                               " from " + to_string((int)startHour) + ":00");

        return booking;
    }

    bool cancelBooking(const string& bookingId, User* user) {
        for (auto* b : bookings) {
            if (b->getId() == bookingId && b->getUserId() == user->getId()) {
                if (b->getStatus() == BookingStatus::IN_PROGRESS) {
                    cout << "  [CANCEL] Cannot cancel in-progress session!\n";
                    return false;
                }
                if (b->getStatus() == BookingStatus::CONFIRMED ||
                    b->getStatus() == BookingStatus::PENDING) {
                    b->setStatus(BookingStatus::CANCELLED);
                    user->addPenalty(b->getCancellationPenalty());
                    cout << "  [CANCEL] Booking " << bookingId
                              << " cancelled. Penalty: PKR "
                              << b->getCancellationPenalty() << "\n";
                    // Remove from pending queue
                    pendingQueue.erase(
                        remove(pendingQueue.begin(), pendingQueue.end(), b),
                        pendingQueue.end());
                    return true;
                }
            }
        }
        return false;
    }

    // Sort queue by priority (highest first)
    void sortPendingQueue() {
        sort(pendingQueue.begin(), pendingQueue.end(),
            [](Booking* a, Booking* b) {
                return a->getPriority() > b->getPriority();
            });
    }

    // ============================================================
    // CHARGING SESSION SIMULATION
    // ============================================================
    void processChargingSessions(double hour) {
        currentHour = hour;

        // Update all energy sources
        for (auto* station : stations) {
            station->updateEnergySources(hour);
        }

        cout << "\n  [SIMULATION] Processing hour " << hour << ":00\n";

        // Process pending bookings in priority order
        for (auto* booking : pendingQueue) {
            if (booking->getStatus() != BookingStatus::PENDING) continue;
            if (booking->getStartHour() > hour) continue;

            ChargingStation* station = findStation(booking->getStationId());
            if (!station) continue;

            // Find the vehicle
            EV* vehicle = nullptr;
            for (auto* u : users) {
                vehicle = u->getVehicle(booking->getVehicleId());
                if (vehicle) break;
            }
            if (!vehicle) continue;

            // Assign dock
            if (station->assignDockToBooking(*booking, vehicle)) {
                cout << "  [SESSION START] Booking " << booking->getId()
                          << " -> Dock " << booking->getDockId() << "\n";
            } else {
                cout << "  [QUEUE] Booking " << booking->getId()
                          << " waiting - no dock available\n";
            }
        }

        // Run active sessions
        for (auto* booking : bookings) {
            if (booking->getStatus() != BookingStatus::IN_PROGRESS) continue;
            if (hour > booking->getEndHour()) {
                completeSession(booking, hour);
                continue;
            }

            ChargingStation* station = findStation(booking->getStationId());
            if (!station) continue;

            EV* vehicle = nullptr;
            User* user = nullptr;
            for (auto* u : users) {
                vehicle = u->getVehicle(booking->getVehicleId());
                if (vehicle) { user = u; break; }
            }
            if (!vehicle) continue;

            bool useRenewable = station->getRenewablePower() > 0;
            double energy = station->simulateChargingStep(*booking, vehicle, 1.0, useRenewable);
            booking->setEnergyDelivered(booking->getEnergyDelivered() + energy);

            cout << "  [CHARGING] Booking " << booking->getId()
                      << " | Energy: " << fixed << setprecision(2)
                      << energy << " kWh | SOC: " << (vehicle->getSOC() * 100) << "%"
                      << " | Source: " << (useRenewable ? "Renewable" : "Grid") << "\n";

            // Auto-complete if target SOC reached
            if (vehicle->getSOC() >= booking->getTargetSOC()) {
                cout << "  [COMPLETE] Target SOC reached for booking " << booking->getId() << "\n";
                completeSession(booking, hour);
            }
        }
    }

    void completeSession(Booking* booking, double hour) {
        ChargingStation* station = findStation(booking->getStationId());
        if (!station) return;

        // Find user and vehicle
        User* user = nullptr;
        EV* vehicle = nullptr;
        for (auto* u : users) {
            vehicle = u->getVehicle(booking->getVehicleId());
            if (vehicle) { user = u; break; }
        }

        booking->setStatus(BookingStatus::COMPLETED);

        // Release dock
        for (auto& dock : station->getDocks()) {
            if (dock.getId() == booking->getDockId()) {
                station->releaseDocK(booking->getDockId(), dock.getPowerRating());
                break;
            }
        }

        // Generate invoice
        if (user && vehicle) {
            bool useRenewable = station->getRenewableEnergyUsed() > 0;
            double duration = hour - booking->getStartHour();
            if (duration <= 0) duration = booking->getDuration();

            Invoice inv = pricing.generateInvoice(
                booking->getId(),
                station->getName(),
                booking->getDockId(),
                vehicle->getId(),
                *user,
                booking->getDockType(),
                booking->getEnergyDelivered(),
                duration,
                booking->getStartHour(),
                useRenewable
            );
            booking->setInvoice(inv);
            station->addRevenue(inv.totalAmount);
            user->deductBalance(inv.totalAmount);
            user->addCO2Savings(inv.co2Saved);

            cout << "\n  [SESSION COMPLETE] Booking " << booking->getId() << "\n";
            inv.print();

            user->sendNotification("Session complete! Charged " +
                to_string((int)booking->getEnergyDelivered()) +
                " kWh. Invoice: " + inv.invoiceId);
        }

        // Remove from pending queue
        pendingQueue.erase(
            remove(pendingQueue.begin(), pendingQueue.end(), booking),
            pendingQueue.end());
    }

    // ============================================================
    // REAL-TIME AVAILABILITY
    // ============================================================
    void checkNetworkAvailability() const {
        cout << "\n  --- Real-Time Dock Availability ---\n";
        for (auto* station : stations) {
            station->checkAvailability();
        }
    }

    // ============================================================
    // PRICING DISPLAY
    // ============================================================
    void displayCurrentPricing() const {
        pricing.displayRates(currentHour);
    }

    // ============================================================
    // V2G OPERATION
    // ============================================================
    void initiateV2G(const string& stationId, EV* vehicle, double kw) {
        ChargingStation* station = findStation(stationId);
        if (!station) return;
        cout << "\n  [V2G] Vehicle-to-Grid initiated at " << station->getName() << "\n";
        station->executeV2G(vehicle, kw, 1.0);
    }

    // ============================================================
    // REPORTS
    // ============================================================
    void generateFullReport() const {
        analytics->generateFullReport();
    }

    void setLanguage(const string& lang) {
        language = lang;
        cout << "  [LANG] Interface language set to: " << lang << "\n";
    }

    void advanceTime(double hours = 1.0) {
        currentHour += hours;
        if (currentHour >= 24) currentHour -= 24;
    }

    double getCurrentHour() const { return currentHour; }

    void displayNetworkStatus() const {
        cout << "\n  === Network Status: " << networkName
                  << " | City: " << cityName
                  << " | Time: " << currentHour << ":00 ===\n";
        cout << "  Stations: " << stations.size()
                  << " | Users: " << users.size()
                  << " | Bookings: " << bookings.size() << "\n";
    }

private:
    void printBanner() {
        cout << "\n";
        cout << "  ████████████████████████████████████████████████████████████\n";
        cout << "  ██                                                        ██\n";
        cout << "  ██       SMART EV CHARGING NETWORK MANAGEMENT SYSTEM      ██\n";
        cout << "  ██              " << left << setw(40) << networkName << "      ██\n";
        cout << "  ██              City: " << left << setw(35) << cityName << "      ██\n";
        cout << "  ██                                                        ██\n";
        cout << "  ████████████████████████████████████████████████████████████\n\n";
    }
};

int NetworkManager::bookingCounter = 0;
int NetworkManager::userCounter    = 0;
int NetworkManager::vehicleCounter = 0;


// ============================================================
// main.cpp
// ============================================================


void clearInput() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

void setupCityInfrastructure(NetworkManager& network) {
    cout << "\n  Setting up city charging infrastructure...\n";

    ChargingStation* downtown = new ChargingStation(
        "ST001", "Downtown Hub", "Main Boulevard, Rawalpindi", 33.597, 73.047, 400.0);
    downtown->addDocks(4, 6, 4);
    downtown->addEnergySource(new GridPower(300.0));
    downtown->addEnergySource(new SolarPower(80.0));
    downtown->addEnergySource(new WindPower(50.0));
    network.addStation(downtown);

    ChargingStation* airport = new ChargingStation(
        "ST002", "Airport Terminal", "Islamabad International Airport", 33.549, 72.826, 500.0);
    airport->addDocks(2, 4, 8);
    airport->addEnergySource(new GridPower(400.0));
    airport->addEnergySource(new SolarPower(120.0));
    network.addStation(airport);

    ChargingStation* campus = new ChargingStation(
        "ST003", "University Campus", "NUST Campus, H-12", 33.643, 72.990, 200.0);
    campus->addDocks(6, 4, 2);
    campus->addEnergySource(new GridPower(150.0));
    campus->addEnergySource(new SolarPower(60.0));
    campus->addEnergySource(new WindPower(40.0));
    network.addStation(campus);

    cout << "  [OK] 3 stations ready: Downtown Hub | Airport Terminal | University Campus\n";
}

void printDivider() { cout << "  " << string(54, '=') << "\n"; }

void printHeader(const string& title) {
    cout << "\n"; printDivider();
    cout << "  " << title << "\n"; printDivider();
}

int getIntInput(const string& prompt, int minVal, int maxVal) {
    int val;
    while (true) {
        cout << prompt;
        if (cin >> val && val >= minVal && val <= maxVal) { clearInput(); return val; }
        cout << "  [!] Enter a number between " << minVal << " and " << maxVal << ".\n";
        clearInput();
    }
}

double getDoubleInput(const string& prompt, double minVal, double maxVal) {
    double val;
    while (true) {
        cout << prompt;
        if (cin >> val && val >= minVal && val <= maxVal) { clearInput(); return val; }
        cout << "  [!] Enter a value between " << minVal << " and " << maxVal << ".\n";
        clearInput();
    }
}

string getStringInput(const string& prompt) {
    string val;
    cout << prompt;
    clearInput();
    getline(cin, val);
    return val;
}

// ---- Register User ----
void menuRegisterUser(NetworkManager& network) {
    printHeader("REGISTER NEW USER");
    string name  = getStringInput("  Full name       : ");
    string email = getStringInput("  Email           : ");
    string phone = getStringInput("  Phone number    : ");
    cout << "\n  Membership Levels:\n";
    cout << "    1. Basic    (no discount)\n";
    cout << "    2. Silver   (8% discount)\n";
    cout << "    3. Gold     (15% discount)\n";
    cout << "    4. Platinum (25% discount)\n";
    int mChoice = getIntInput("  Select [1-4]: ", 1, 4);
    MembershipLevel membership;
    switch (mChoice) {
        case 2: membership = MembershipLevel::SILVER;   break;
        case 3: membership = MembershipLevel::GOLD;     break;
        case 4: membership = MembershipLevel::PLATINUM; break;
        default:membership = MembershipLevel::BASIC;    break;
    }
    User* user = network.registerUser(name, email, phone, membership);
    double topup = getDoubleInput("  Wallet top-up PKR (0 to skip): ", 0, 100000);
    if (topup > 0) user->topUpWallet(topup);
    cout << "  [SUCCESS] User registered! Your ID: " << user->getId() << "\n";
}

// ---- Register Vehicle ----
void menuRegisterVehicle(NetworkManager& network) {
    printHeader("REGISTER VEHICLE");
    string userId = getStringInput("  Enter User ID (e.g. U1): ");
    User* user = network.findUser(userId);
    if (!user) { cout << "  [ERROR] User not found!\n"; return; }
    cout << "  User: " << user->getName() << "\n\n";
    string model = getStringInput("  Vehicle model    : ");
    string plate = getStringInput("  License plate    : ");
    cout << "\n  Vehicle Types:\n";
    cout << "    1. Motorcycle\n    2. Sedan\n    3. SUV\n    4. Truck\n    5. Bus\n";
    int vChoice = getIntInput("  Select [1-5]: ", 1, 5);
    VehicleType vtype;
    switch (vChoice) {
        case 1: vtype = VehicleType::MOTORCYCLE; break;
        case 2: vtype = VehicleType::SEDAN;      break;
        case 3: vtype = VehicleType::SUV;        break;
        case 4: vtype = VehicleType::TRUCK;      break;
        default:vtype = VehicleType::BUS;        break;
    }
    double battery = getDoubleInput("  Battery capacity kWh (1-300): ", 1, 300);
    double soc     = getDoubleInput("  Current charge % (0-100): ", 0, 100) / 100.0;
    EV* ev = network.registerVehicle(user, model, vtype, battery, soc, plate);
    cout << "  [SUCCESS] Vehicle registered! ID: " << ev->getId() << "\n";
}

// ---- View Profile ----
void menuViewProfile(NetworkManager& network) {
    printHeader("USER PROFILE");
    string userId = getStringInput("  Enter User ID: ");
    User* user = network.findUser(userId);
    if (!user) { cout << "  [ERROR] User not found!\n"; return; }
    user->display();
}

// ---- Top Up Wallet ----
void menuTopUpWallet(NetworkManager& network) {
    printHeader("TOP UP WALLET");
    string userId = getStringInput("  Enter User ID: ");
    User* user = network.findUser(userId);
    if (!user) { cout << "  [ERROR] User not found!\n"; return; }
    cout << "  Current balance: PKR " << fixed << setprecision(2) << user->getWalletBalance() << "\n";
    double amount = getDoubleInput("  Top-up amount PKR: ", 1, 100000);
    user->topUpWallet(amount);
    cout << "  [SUCCESS] New balance: PKR " << user->getWalletBalance() << "\n";
}

// ---- View Notifications ----
void menuViewNotifications(NetworkManager& network) {
    printHeader("NOTIFICATIONS");
    string userId = getStringInput("  Enter User ID: ");
    User* user = network.findUser(userId);
    if (!user) { cout << "  [ERROR] User not found!\n"; return; }
    user->displayNotifications();
}

// ---- Book Slot ----
void menuBookSlot(NetworkManager& network) {
    printHeader("BOOK CHARGING SLOT");
    string userId = getStringInput("  Enter User ID: ");
    User* user = network.findUser(userId);
    if (!user) { cout << "  [ERROR] User not found!\n"; return; }

    auto& vehicles = user->getVehicles();
    if (vehicles.empty()) { cout << "  [ERROR] No vehicles registered! Register a vehicle first.\n"; return; }

    cout << "\n  Your Vehicles:\n";
    for (int i = 0; i < (int)vehicles.size(); i++)
        cout << "    " << (i+1) << ". " << vehicles[i]->getModel()
             << " [" << vehicles[i]->getId() << "]"
             << " | Battery: " << vehicles[i]->getBatteryCapacity() << "kWh"
             << " | SOC: " << fixed << setprecision(1) << (vehicles[i]->getSOC()*100) << "%\n";

    int vIdx = getIntInput("  Select vehicle: ", 1, (int)vehicles.size()) - 1;
    EV* vehicle = vehicles[vIdx];

    cout << "\n  Available Stations:\n";
    cout << "    1. ST001 - Downtown Hub        (400 kW capacity)\n";
    cout << "    2. ST002 - Airport Terminal    (500 kW capacity)\n";
    cout << "    3. ST003 - University Campus   (200 kW capacity)\n";
    int sChoice = getIntInput("  Select station [1-3]: ", 1, 3);
    string stationId = "ST00" + to_string(sChoice);

    cout << "\n  Dock Types:\n";
    cout << "    1. Slow    3.7 kW   - Cheapest, best for overnight\n";
    cout << "    2. Medium  22  kW   - Balanced speed and cost\n";
    cout << "    3. Fast    150 kW   - Fastest, premium price\n";
    int dChoice = getIntInput("  Select dock type [1-3]: ", 1, 3);
    DockType dockType;
    switch (dChoice) {
        case 2: dockType = DockType::MEDIUM; break;
        case 3: dockType = DockType::FAST;   break;
        default:dockType = DockType::SLOW;   break;
    }

    double startHour = getDoubleInput("  Start hour (0-23): ", 0, 23);
    double duration  = getDoubleInput("  Duration hours (0.5-12): ", 0.5, 12);
    double minSOC    = vehicle->getSOC() * 100;
    double targetSOC = getDoubleInput("  Target charge % (" + to_string((int)minSOC) + "-100): ",
                                       minSOC, 100) / 100.0;

    network.bookSlot(user, vehicle, stationId, dockType, startHour, duration, targetSOC);
}

// ---- Cancel Booking ----
void menuCancelBooking(NetworkManager& network) {
    printHeader("CANCEL BOOKING");
    string userId    = getStringInput("  Enter User ID    : ");
    User* user = network.findUser(userId);
    if (!user) { cout << "  [ERROR] User not found!\n"; return; }
    string bookingId = getStringInput("  Enter Booking ID : ");
    if (network.cancelBooking(bookingId, user))
        cout << "  [SUCCESS] Booking cancelled.\n";
    else
        cout << "  [FAILED] Booking not found or cannot be cancelled.\n";
}

// ---- Check Availability ----
void menuCheckAvailability(NetworkManager& network) {
    printHeader("DOCK AVAILABILITY & PRICING");
    network.checkNetworkAvailability();
    network.displayCurrentPricing();
}

// ---- Run Simulation ----
void menuRunSimulation(NetworkManager& network) {
    printHeader("RUN CHARGING SIMULATION");
    cout << "  This advances the simulation clock and processes\n";
    cout << "  all active bookings for the selected hour.\n\n";
    double hour = getDoubleInput("  Enter hour to simulate (0-24): ", 0, 24);
    network.processChargingSessions(hour);
    network.checkNetworkAvailability();
}

// ---- V2G ----
void menuV2G(NetworkManager& network) {
    printHeader("VEHICLE-TO-GRID (V2G)");
    string userId = getStringInput("  Enter User ID: ");
    User* user = network.findUser(userId);
    if (!user) { cout << "  [ERROR] User not found!\n"; return; }
    auto& vehicles = user->getVehicles();
    if (vehicles.empty()) { cout << "  [ERROR] No vehicles found!\n"; return; }
    cout << "\n  Your Vehicles:\n";
    for (int i = 0; i < (int)vehicles.size(); i++)
        cout << "    " << (i+1) << ". " << vehicles[i]->getModel()
             << " | SOC: " << (vehicles[i]->getSOC()*100) << "%\n";
    int vIdx = getIntInput("  Select vehicle: ", 1, (int)vehicles.size()) - 1;
    cout << "  Stations:  1.ST001-Downtown  2.ST002-Airport  3.ST003-Campus\n";
    int sChoice = getIntInput("  Select station [1-3]: ", 1, 3);
    double power = getDoubleInput("  Export power kW (1-100): ", 1, 100);
    network.initiateV2G("ST00" + to_string(sChoice), vehicles[vIdx], power);
}

// ---- Reports ----
void menuReports(NetworkManager& network) {
    printHeader("ANALYTICS & REPORTS");
    cout << "    1. Full System Report\n";
    cout << "    2. Network Status Overview\n";
    cout << "    3. Current Pricing Rates\n";
    int choice = getIntInput("  Select [1-3]: ", 1, 3);
    switch (choice) {
        case 1: network.generateFullReport();     break;
        case 2: network.displayNetworkStatus();   break;
        case 3: network.displayCurrentPricing();  break;
    }
}

// ---- Main Menu ----
void printMainMenu() {
    cout << "\n";
    cout << "  ╔════════════════════════════════════════════════════╗\n";
    cout << "  ║      SMART EV CHARGING NETWORK - MAIN MENU        ║\n";
    cout << "  ╠════════════════════════════════════════════════════╣\n";
    cout << "  ║  USER MANAGEMENT                                   ║\n";
    cout << "  ║    1.  Register New User                           ║\n";
    cout << "  ║    2.  Register Vehicle for User                   ║\n";
    cout << "  ║    3.  View User Profile                           ║\n";
    cout << "  ║    4.  Top Up Wallet                               ║\n";
    cout << "  ║    5.  View Notifications                          ║\n";
    cout << "  ╠════════════════════════════════════════════════════╣\n";
    cout << "  ║  BOOKING MANAGEMENT                                ║\n";
    cout << "  ║    6.  Book a Charging Slot                        ║\n";
    cout << "  ║    7.  Cancel a Booking                            ║\n";
    cout << "  ║    8.  Check Dock Availability & Pricing           ║\n";
    cout << "  ╠════════════════════════════════════════════════════╣\n";
    cout << "  ║  SIMULATION & ADVANCED                             ║\n";
    cout << "  ║    9.  Run Charging Simulation (Advance Time)      ║\n";
    cout << "  ║    10. Vehicle-to-Grid (V2G)                       ║\n";
    cout << "  ║    11. Analytics & Reports                         ║\n";
    cout << "  ╠════════════════════════════════════════════════════╣\n";
    cout << "  ║    0.  Exit                                        ║\n";
    cout << "  ╚════════════════════════════════════════════════════╝\n";
    cout << "  Enter option: ";
}

int main() {
    cout << "\n";
    cout << "  ██████████████████████████████████████████████████████\n";
    cout << "  ██                                                  ██\n";
    cout << "  ██   SMART EV CHARGING NETWORK MANAGEMENT SYSTEM   ██\n";
    cout << "  ██         CityCharge - Rawalpindi/Islamabad        ██\n";
    cout << "  ██                                                  ██\n";
    cout << "  ██████████████████████████████████████████████████████\n";

    NetworkManager network("CityCharge Network", "Rawalpindi-Islamabad");
    setupCityInfrastructure(network);

    int choice = -1;
    while (choice != 0) {
        printMainMenu();
        if (!(cin >> choice)) { clearInput(); continue; }
        clearInput();
        switch (choice) {
            case 1:  menuRegisterUser(network);       break;
            case 2:  menuRegisterVehicle(network);    break;
            case 3:  menuViewProfile(network);        break;
            case 4:  menuTopUpWallet(network);        break;
            case 5:  menuViewNotifications(network);  break;
            case 6:  menuBookSlot(network);           break;
            case 7:  menuCancelBooking(network);      break;
            case 8:  menuCheckAvailability(network);  break;
            case 9:  menuRunSimulation(network);      break;
            case 10: menuV2G(network);                break;
            case 11: menuReports(network);            break;
            case 0:  cout << "\n  Goodbye! Thank you for using CityCharge Network.\n\n"; break;
            default: cout << "  [!] Invalid option. Try again.\n";
        }
    }
    return 0;
}


