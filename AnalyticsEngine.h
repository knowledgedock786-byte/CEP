#pragma once
#include "ChargingStation.h"
#include "Booking.h"
#include "User.h"
#include <vector>
#include <map>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <string>

// ============================================================
// AnalyticsEngine - Reporting and Insights
// ============================================================
class AnalyticsEngine {
private:
    std::vector<ChargingStation*>& stations;
    std::vector<Booking*>& bookings;
    std::vector<User*>& users;

    // Revenue by hour
    std::map<int, double> revenueByHour;
    std::map<std::string, double> revenueByStation;
    std::map<std::string, int> bookingsByUser;

public:
    AnalyticsEngine(std::vector<ChargingStation*>& s,
                    std::vector<Booking*>& b,
                    std::vector<User*>& u)
        : stations(s), bookings(b), users(u) {}

    // ---- Station Utilization Report ----
    void generateStationReport() const {
        std::cout << "\n";
        std::cout << "  ╔══════════════════════════════════════════════════════════╗\n";
        std::cout << "  ║              STATION UTILIZATION REPORT                  ║\n";
        std::cout << "  ╠══════════════════════════════════════════════════════════╣\n";

        for (auto* station : stations) {
            std::cout << std::fixed << std::setprecision(1);
            std::cout << "  ║ " << std::left << std::setw(20) << station->getName()
                      << " Util: " << std::setw(5) << station->getUtilizationRate() << "%"
                      << "  Peak: " << std::setw(6) << station->getPeakLoad() << "kW   ║\n";
            std::cout << "  ║   Energy: " << std::setw(8) << station->getTotalEnergyDelivered()
                      << "kWh  Revenue: PKR " << std::setw(8) << station->getTotalRevenue()
                      << "         ║\n";
            std::cout << "  ║   Renewable: " << std::setw(5) << station->getRenewableRatio()
                      << "%  Bookings: " << std::setw(5) << station->getTotalBookings()
                      << "                    ║\n";
            std::cout << "  ╠══════════════════════════════════════════════════════════╣\n";
        }
        std::cout << "  ╚══════════════════════════════════════════════════════════╝\n";
    }

    // ---- Session Analytics ----
    void generateSessionReport() const {
        std::cout << "\n";
        std::cout << "  ╔══════════════════════════════════════════════════════════╗\n";
        std::cout << "  ║                SESSION ANALYTICS REPORT                  ║\n";
        std::cout << "  ╠══════════════════════════════════════════════════════════╣\n";

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

        std::cout << "  ║ Total Bookings    : " << std::setw(36) << total << "║\n";
        std::cout << "  ║ Completed         : " << std::setw(36) << completed << "║\n";
        std::cout << "  ║ In Progress       : " << std::setw(36) << inProgress << "║\n";
        std::cout << "  ║ Cancelled         : " << std::setw(36) << cancelled << "║\n";
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "  ║ Avg Duration (hr) : " << std::setw(36) << avgDuration << "║\n";
        std::cout << "  ║ Avg Energy (kWh)  : " << std::setw(36) << avgEnergy << "║\n";
        std::cout << "  ║ Completion Rate   : " << std::setw(34) << completionRate << "% ║\n";
        std::cout << "  ╚══════════════════════════════════════════════════════════╝\n";
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

        std::cout << "\n";
        std::cout << "  ╔══════════════════════════════════════════════════════════╗\n";
        std::cout << "  ║            ENERGY & ENVIRONMENTAL REPORT                 ║\n";
        std::cout << "  ╠══════════════════════════════════════════════════════════╣\n";
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "  ║ Total Energy Delivered : " << std::setw(31) << total << "kWh ║\n";
        std::cout << "  ║ Renewable Energy Used  : " << std::setw(31) << totalRenewable << "kWh ║\n";
        std::cout << "  ║ Grid Energy Used       : " << std::setw(31) << totalGrid << "kWh ║\n";
        std::cout << "  ║ Renewable Ratio        : " << std::setw(30) << renewRatio << "% ║\n";
        std::cout << "  ╠══════════════════════════════════════════════════════════╣\n";
        std::cout << "  ║ CO2 Emitted (Grid)     : " << std::setw(30) << co2Emitted << "kg ║\n";
        std::cout << "  ║ CO2 Saved (Renewable)  : " << std::setw(30) << co2Saved << "kg ║\n";
        std::cout << "  ║ Net Environmental Gain : " << std::setw(30) << (co2Saved - co2Emitted) << "kg ║\n";

        // Visualize renewable ratio
        std::cout << "  ╠══════════════════════════════════════════════════════════╣\n";
        std::cout << "  ║ Renewable Usage: [";
        int filled = (int)(renewRatio / 2.5); // Scale to 40 chars
        for (int i = 0; i < 40; i++) std::cout << (i < filled ? "█" : "░");
        std::cout << "] " << (int)renewRatio << "% ║\n";
        std::cout << "  ╚══════════════════════════════════════════════════════════╝\n";
    }

    // ---- Revenue Report ----
    void generateRevenueReport() const {
        double totalRev = 0;
        for (auto* s : stations) totalRev += s->getTotalRevenue();

        // Revenue per station
        std::cout << "\n";
        std::cout << "  ╔══════════════════════════════════════════════════════════╗\n";
        std::cout << "  ║                   REVENUE REPORT                         ║\n";
        std::cout << "  ╠══════════════════════════════════════════════════════════╣\n";
        std::cout << std::fixed << std::setprecision(2);
        for (auto* s : stations) {
            double pct = totalRev > 0 ? s->getTotalRevenue() / totalRev * 100.0 : 0;
            std::cout << "  ║ " << std::left << std::setw(22) << s->getName()
                      << ": PKR " << std::right << std::setw(10) << s->getTotalRevenue()
                      << " (" << std::setw(5) << pct << "%)    ║\n";
        }
        std::cout << "  ╠══════════════════════════════════════════════════════════╣\n";
        std::cout << "  ║ TOTAL NETWORK REVENUE: PKR " << std::setw(29) << totalRev << "║\n";
        std::cout << "  ╚══════════════════════════════════════════════════════════╝\n";
    }

    // ---- User Demand Trends ----
    void generateUserReport() const {
        std::cout << "\n";
        std::cout << "  ╔══════════════════════════════════════════════════════════╗\n";
        std::cout << "  ║                USER DEMAND TRENDS                        ║\n";
        std::cout << "  ╠══════════════════════════════════════════════════════════╣\n";

        std::map<MembershipLevel, int> memberDist;
        for (auto* u : users) memberDist[u->getMembership()]++;

        std::cout << "  ║ Total Users       : " << std::setw(36) << users.size() << "║\n";
        std::cout << "  ║ Basic Members     : " << std::setw(36) << memberDist[MembershipLevel::BASIC] << "║\n";
        std::cout << "  ║ Silver Members    : " << std::setw(36) << memberDist[MembershipLevel::SILVER] << "║\n";
        std::cout << "  ║ Gold Members      : " << std::setw(36) << memberDist[MembershipLevel::GOLD] << "║\n";
        std::cout << "  ║ Platinum Members  : " << std::setw(36) << memberDist[MembershipLevel::PLATINUM] << "║\n";

        double totalCO2Saved = 0;
        for (auto* u : users) totalCO2Saved += u->getTotalCO2Saved();
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "  ║ Total CO2 Saved   : " << std::setw(33) << totalCO2Saved << " kg ║\n";
        std::cout << "  ╚══════════════════════════════════════════════════════════╝\n";
    }

    // ---- Hourly Load Visualization ----
    void displayHourlyLoadChart(ChargingStation* station) const {
        const auto& hourly = station->getHourlyLoad();
        double maxLoad = *std::max_element(hourly.begin(), hourly.end());
        if (maxLoad == 0) maxLoad = 1;

        std::cout << "\n  Hourly Load Chart - " << station->getName() << "\n";
        std::cout << "  " << std::string(52, '-') << "\n";

        for (int h = 0; h < 24; h++) {
            int bars = (int)(hourly[h] / maxLoad * 30);
            std::cout << "  " << std::setw(2) << h << ":00 |";
            for (int b = 0; b < 30; b++) {
                if (b < bars) {
                    // Peak hour styling
                    if (h >= 8 && h < 21) std::cout << "▓";
                    else std::cout << "░";
                } else {
                    std::cout << " ";
                }
            }
            std::cout << "| " << std::fixed << std::setprecision(0) << hourly[h] << "kW\n";
        }
        std::cout << "  " << std::string(52, '-') << "\n";
        std::cout << "  ▓ = Peak Hour  ░ = Off-Peak\n";
    }

    // ---- Full System Report ----
    void generateFullReport() const {
        std::cout << "\n\n";
        std::cout << "  ════════════════════════════════════════════════════════════\n";
        std::cout << "  ██████   SMART EV CHARGING NETWORK - FULL SYSTEM REPORT   ██\n";
        std::cout << "  ════════════════════════════════════════════════════════════\n";
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
