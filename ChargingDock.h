#pragma once
#include "Booking.h"
#include <string>
#include <queue>
#include <vector>
#include <iostream>
#include <iomanip>

// ============================================================
// ChargingDock Class
// ============================================================
class ChargingDock {
private:
    std::string dockId;
    DockType type;
    double powerRatingKW;
    bool isOccupied;
    bool isOnline;
    std::string currentBookingId;
    std::string currentVehicleId;
    double totalEnergyDeliveredKWh;
    double totalSessionTime;
    int totalSessions;
    double currentSessionEnergy;

    // Maintenance tracking
    bool needsMaintenance;
    int sessionsUntilMaintenance;

public:
    ChargingDock(const std::string& id, DockType type)
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
    std::string getId() const { return dockId; }
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
    bool assignSession(const std::string& bookingId, const std::string& vehicleId) {
        if (!isAvailable()) return false;
        isOccupied = true;
        currentBookingId = bookingId;
        currentVehicleId = vehicleId;
        currentSessionEnergy = 0.0;
        totalSessions++;
        sessionsUntilMaintenance--;
        if (sessionsUntilMaintenance <= 0) {
            needsMaintenance = true;
            std::cout << "  [MAINTENANCE ALERT] Dock " << dockId << " requires maintenance!\n";
        }
        return true;
    }

    // Simulate charging for a time step
    double chargeStep(EV* vehicle, double durationHours) {
        if (!isOccupied || !vehicle) return 0.0;

        double effectivePower = std::min(powerRatingKW, vehicle->getMaxChargingRate());
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
        std::cout << "  [MAINTENANCE] Dock " << dockId << " maintenance complete. Back online.\n";
    }

    void setOffline() { isOnline = false; }
    void setOnline()  { isOnline = true; }

    std::string getCurrentBookingId() const { return currentBookingId; }

    // Get effective power for a specific vehicle
    double getEffectivePower(const EV* vehicle) const {
        if (!vehicle) return powerRatingKW;
        return std::min(powerRatingKW, vehicle->getMaxChargingRate());
    }

    void display() const {
        std::string statusStr;
        if (!isOnline)          statusStr = "OFFLINE";
        else if (needsMaintenance) statusStr = "MAINTENANCE";
        else if (isOccupied)    statusStr = "OCCUPIED";
        else                    statusStr = "AVAILABLE";

        std::cout << std::fixed << std::setprecision(1);
        std::cout << "    Dock [" << dockId << "] "
                  << dockTypeStr(type) << " | " << powerRatingKW << " kW"
                  << " | Status: " << statusStr
                  << " | Sessions: " << totalSessions
                  << " | Energy: " << totalEnergyDeliveredKWh << " kWh\n";
    }
};
