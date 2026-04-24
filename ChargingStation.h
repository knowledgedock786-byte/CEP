#pragma once
#include "ChargingDock.h"
#include "EnergySource.h"
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <memory>

// ============================================================
// ChargingStation Class
// ============================================================
class ChargingStation {
private:
    std::string stationId;
    std::string name;
    std::string address;
    double latitude;
    double longitude;
    bool isOperational;

    std::vector<ChargingDock> docks;
    std::vector<EnergySource*> energySources;

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
    std::vector<double> hourlyLoadHistory;
    std::vector<double> hourlyRevenueHistory;

public:
    ChargingStation(const std::string& id, const std::string& name,
                    const std::string& address, double lat, double lon,
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
        std::string dockId = stationId + "-D" + std::to_string(docks.size() + 1);
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
        return std::min(total, maxCapacityKW);
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
            std::cout << "  [LOAD SHED] Station " << name
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
        currentLoadKW = std::max(0.0, currentLoadKW - kw);
        if (isLoadShedding && currentLoadKW < maxCapacityKW * 0.75) {
            isLoadShedding = false;
            std::cout << "  [LOAD SHED] Station " << name << " - Load shedding deactivated.\n";
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

    void releaseDocK(const std::string& dockId, double dockPower) {
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
        std::cout << "  Station: " << name << "\n"
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
    std::string getId() const { return stationId; }
    std::string getName() const { return name; }
    std::string getAddress() const { return address; }
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
    std::vector<ChargingDock>& getDocks() { return docks; }
    const std::vector<double>& getHourlyLoad() const { return hourlyLoadHistory; }

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
        double exported = std::min(kw * durationHours, maxExportable);
        vehicle->setSOC(vehicle->getSOC() - exported / vehicle->getBatteryCapacity());
        gridEnergyUsedKWh = std::max(0.0, gridEnergyUsedKWh - exported);
        std::cout << "  [V2G] Vehicle " << vehicle->getId() << " exported "
                  << exported << " kWh back to grid.\n";
        return exported;
    }

    void display() const {
        std::cout << std::fixed << std::setprecision(1);
        std::cout << "\nStation: " << name << " [" << stationId << "]\n"
                  << "  Address: " << address << "\n"
                  << "  Docks: " << docks.size()
                  << " | Load: " << currentLoadKW << "/" << maxCapacityKW << " kW\n"
                  << "  Total Energy: " << totalEnergyDeliveredKWh << " kWh"
                  << " | Revenue: PKR " << totalRevenue << "\n"
                  << "  Renewable: " << getRenewableRatio() << "% of energy\n";
        for (const auto& dock : docks) dock.display();
        std::cout << "  Energy Sources:\n";
        for (auto* src : energySources) src->displayInfo();
    }
};
