#pragma once
#include <string>
#include <iostream>
#include <vector>
#include <iomanip>

// ============================================================
// Enumerations
// ============================================================
enum class VehicleType { SEDAN, SUV, TRUCK, MOTORCYCLE, BUS };
enum class MembershipLevel { BASIC, SILVER, GOLD, PLATINUM };
enum class DockType { SLOW, MEDIUM, FAST };
enum class BookingStatus { PENDING, CONFIRMED, IN_PROGRESS, COMPLETED, CANCELLED };

inline std::string vehicleTypeStr(VehicleType t) {
    switch(t) {
        case VehicleType::SEDAN:      return "Sedan";
        case VehicleType::SUV:        return "SUV";
        case VehicleType::TRUCK:      return "Truck";
        case VehicleType::MOTORCYCLE: return "Motorcycle";
        case VehicleType::BUS:        return "Bus";
        default: return "Unknown";
    }
}

inline std::string membershipStr(MembershipLevel m) {
    switch(m) {
        case MembershipLevel::BASIC:    return "Basic";
        case MembershipLevel::SILVER:   return "Silver";
        case MembershipLevel::GOLD:     return "Gold";
        case MembershipLevel::PLATINUM: return "Platinum";
        default: return "Unknown";
    }
}

inline std::string dockTypeStr(DockType d) {
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
    std::string vehicleId;
    std::string model;
    VehicleType type;
    double batteryCapacityKWh;
    double currentSOC;        // State of Charge: 0.0 to 1.0
    double maxChargingRateKW;
    std::string licensePlate;

public:
    EV(const std::string& id, const std::string& model,
       VehicleType type, double batteryKWh, double soc,
       const std::string& plate)
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
    std::string getId() const { return vehicleId; }
    std::string getModel() const { return model; }
    VehicleType getType() const { return type; }
    double getBatteryCapacity() const { return batteryCapacityKWh; }
    double getSOC() const { return currentSOC; }
    double getMaxChargingRate() const { return maxChargingRateKW; }
    std::string getLicensePlate() const { return licensePlate; }
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
        std::cout << std::fixed << std::setprecision(1);
        std::cout << "  Vehicle: " << model << " [" << vehicleId << "]\n"
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
    std::string message;
    std::string timestamp;
    bool isRead;

    Notification(const std::string& msg, const std::string& ts)
        : message(msg), timestamp(ts), isRead(false) {}
};

// ============================================================
// User Class
// ============================================================
class User {
private:
    std::string userId;
    std::string name;
    std::string email;
    std::string phone;
    MembershipLevel membership;
    int priorityLevel;        // 1 (low) to 10 (high)
    double walletBalance;
    double totalCO2Saved;
    std::vector<EV*> vehicles;
    std::vector<Notification> notifications;
    std::vector<std::string> bookingHistory;
    double totalPenalties;

public:
    User(const std::string& id, const std::string& name,
         const std::string& email, const std::string& phone,
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
    std::string getId() const { return userId; }
    std::string getName() const { return name; }
    std::string getEmail() const { return email; }
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

    EV* getVehicle(const std::string& vehicleId) {
        for (auto* v : vehicles) {
            if (v->getId() == vehicleId) return v;
        }
        return nullptr;
    }

    std::vector<EV*>& getVehicles() { return vehicles; }

    // Financial operations
    void topUpWallet(double amount) {
        walletBalance += amount;
        sendNotification("Wallet topped up: PKR " + std::to_string((int)amount));
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
        sendNotification("Cancellation penalty applied: PKR " + std::to_string((int)amount));
    }

    void addCO2Savings(double kg) { totalCO2Saved += kg; }

    // Notifications
    void sendNotification(const std::string& message) {
        notifications.push_back(Notification(message, "Now"));
        std::cout << "  [NOTIFICATION -> " << name << "]: " << message << "\n";
    }

    void addBookingToHistory(const std::string& bookingId) {
        bookingHistory.push_back(bookingId);
    }

    void displayNotifications() const {
        std::cout << "\n--- Notifications for " << name << " ---\n";
        for (auto& n : notifications) {
            std::cout << (n.isRead ? "  [READ] " : "  [NEW]  ") << n.message << "\n";
        }
    }

    void display() const {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "User: " << name << " [" << userId << "]\n"
                  << "  Email: " << email << " | Phone: " << phone << "\n"
                  << "  Membership: " << membershipStr(membership)
                  << " | Priority: " << priorityLevel << "/10\n"
                  << "  Wallet: PKR " << walletBalance
                  << " | CO2 Saved: " << totalCO2Saved << " kg\n"
                  << "  Vehicles registered: " << vehicles.size() << "\n"
                  << "  Bookings made: " << bookingHistory.size() << "\n";
    }
};
