#pragma once
#include "User.h"
#include "ChargingDock.h"
#include "EnergySource.h"
#include <iostream>
#include <iomanip>
#include <string>

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
        const std::string& bookingId,
        const std::string& stationName,
        const std::string& dockId,
        const std::string& vehicleId,
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
        inv.invoiceId  = "INV-" + std::to_string(++invoiceCounter);
        inv.bookingId  = bookingId;
        inv.userId     = user.getId();
        inv.stationName = stationName;
        inv.dockId     = dockId;
        inv.vehicleId  = vehicleId;
        inv.energyConsumedKWh = energyKWh;
        inv.sessionDurationHours = durationHours;
        inv.energySource = isRenewable ? "Renewable (Solar/Wind)" : "Grid";
        inv.sessionStart = std::to_string((int)startHour) + ":00";
        inv.sessionEnd   = std::to_string((int)(startHour + durationHours)) + ":00";

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
        inv.totalAmount = std::max(0.0, inv.subtotal - renDiscount - memberDiscount);

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
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "\n--- Current Pricing (Hour: " << hour << ") ---\n";
        std::cout << "  Time: " << (isPeakHour(hour) ? "PEAK" : "OFF-PEAK") << "\n";
        std::cout << "  Slow   Dock (Grid):      PKR "
                  << calculateRatePerKWh(DockType::SLOW,   hour, false, MembershipLevel::BASIC) << "/kWh\n";
        std::cout << "  Medium Dock (Grid):      PKR "
                  << calculateRatePerKWh(DockType::MEDIUM, hour, false, MembershipLevel::BASIC) << "/kWh\n";
        std::cout << "  Fast   Dock (Grid):      PKR "
                  << calculateRatePerKWh(DockType::FAST,   hour, false, MembershipLevel::BASIC) << "/kWh\n";
        std::cout << "  Fast   Dock (Renewable): PKR "
                  << calculateRatePerKWh(DockType::FAST,   hour, true,  MembershipLevel::BASIC) << "/kWh\n";
    }
};

int PricingEngine::invoiceCounter = 0;
