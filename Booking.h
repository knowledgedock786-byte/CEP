#pragma once
#include "User.h"
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>

// ============================================================
// Invoice - Digital billing document
// ============================================================
struct Invoice {
    std::string invoiceId;
    std::string bookingId;
    std::string userId;
    std::string stationName;
    std::string dockId;
    std::string vehicleId;
    double energyConsumedKWh;
    double sessionDurationHours;
    double baseRatePerKWh;
    double peakSurcharge;
    double speedPremium;
    double renewableDiscount;
    double membershipDiscount;
    double subtotal;
    double totalAmount;
    std::string energySource;
    double co2Saved;
    double co2Emitted;
    std::string sessionStart;
    std::string sessionEnd;

    Invoice() : energyConsumedKWh(0), sessionDurationHours(0),
                baseRatePerKWh(0), peakSurcharge(0), speedPremium(0),
                renewableDiscount(0), membershipDiscount(0),
                subtotal(0), totalAmount(0), co2Saved(0), co2Emitted(0) {}

    void print() const {
        std::cout << "\n";
        std::cout << "  ╔══════════════════════════════════════════════╗\n";
        std::cout << "  ║         EV CHARGING NETWORK - INVOICE        ║\n";
        std::cout << "  ╠══════════════════════════════════════════════╣\n";
        std::cout << "  ║ Invoice ID  : " << std::left << std::setw(31) << invoiceId << "║\n";
        std::cout << "  ║ Booking ID  : " << std::left << std::setw(31) << bookingId << "║\n";
        std::cout << "  ║ Station     : " << std::left << std::setw(31) << stationName << "║\n";
        std::cout << "  ║ Dock        : " << std::left << std::setw(31) << dockId << "║\n";
        std::cout << "  ║ Vehicle     : " << std::left << std::setw(31) << vehicleId << "║\n";
        std::cout << "  ║ Energy Src  : " << std::left << std::setw(31) << energySource << "║\n";
        std::cout << "  ╠══════════════════════════════════════════════╣\n";
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "  ║ Energy Used : " << std::left << std::setw(25) << (std::to_string(energyConsumedKWh).substr(0,6) + " kWh") << "       ║\n";
        std::cout << "  ║ Duration    : " << std::left << std::setw(25) << (std::to_string(sessionDurationHours).substr(0,5) + " hrs") << "       ║\n";
        std::cout << "  ╠══════════════════════════════════════════════╣\n";
        std::cout << "  ║ BILLING BREAKDOWN                            ║\n";
        std::cout << "  ║ Base Rate   : PKR " << std::setw(27) << baseRatePerKWh << "║\n";
        std::cout << "  ║ Peak Charge : PKR " << std::setw(27) << peakSurcharge << "║\n";
        std::cout << "  ║ Speed Fee   : PKR " << std::setw(27) << speedPremium << "║\n";
        std::cout << "  ║ Renew Disc  : PKR -" << std::setw(26) << renewableDiscount << "║\n";
        std::cout << "  ║ Member Disc : PKR -" << std::setw(26) << membershipDiscount << "║\n";
        std::cout << "  ╠══════════════════════════════════════════════╣\n";
        std::cout << "  ║ TOTAL AMOUNT: PKR " << std::setw(27) << totalAmount << "║\n";
        std::cout << "  ╠══════════════════════════════════════════════╣\n";
        std::cout << "  ║ CO2 Saved   : " << std::left << std::setw(25) << (std::to_string(co2Saved).substr(0,5) + " kg") << "       ║\n";
        std::cout << "  ║ CO2 Emitted : " << std::left << std::setw(25) << (std::to_string(co2Emitted).substr(0,5) + " kg") << "       ║\n";
        std::cout << "  ╚══════════════════════════════════════════════╝\n\n";
    }
};

// ============================================================
// Booking Class
// ============================================================
class Booking {
private:
    std::string bookingId;
    std::string userId;
    std::string vehicleId;
    std::string stationId;
    std::string dockId;
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
    std::string scheduledTime;

public:
    Booking(const std::string& bId, const std::string& uId,
            const std::string& vId, const std::string& sId,
            DockType dType, double startH, double endH,
            double targetSOC, int priority)
        : bookingId(bId), userId(uId), vehicleId(vId), stationId(sId),
          dockId(""), requestedDockType(dType), status(BookingStatus::PENDING),
          startHour(startH), endHour(endH), targetSOC(targetSOC),
          energyDelivered(0.0), isDeferrable(true),
          priorityScore(priority), hasInvoice(false),
          cancellationPenalty(50.0) {}

    // Getters
    std::string getId() const { return bookingId; }
    std::string getUserId() const { return userId; }
    std::string getVehicleId() const { return vehicleId; }
    std::string getStationId() const { return stationId; }
    std::string getDockId() const { return dockId; }
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
    void setDockId(const std::string& id) { dockId = id; }
    void setStatus(BookingStatus s) { status = s; }
    void setEnergyDelivered(double e) { energyDelivered = e; }
    void setIsDeferrable(bool d) { isDeferrable = d; }
    void setStartHour(double h) { startHour = h; }
    void setEndHour(double h) { endHour = h; }
    void setInvoice(const Invoice& inv) { generatedInvoice = inv; hasInvoice = true; }

    const Invoice& getInvoice() const { return generatedInvoice; }
    bool hasGeneratedInvoice() const { return hasInvoice; }

    std::string getStatusStr() const {
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
        std::cout << "  [DEFER] Booking " << bookingId
                  << " deferred to off-peak: " << startHour << ":00 - " << endHour << ":00\n";
    }

    void display() const {
        std::cout << std::fixed << std::setprecision(1);
        std::cout << "  Booking [" << bookingId << "] Status: " << getStatusStr() << "\n"
                  << "    User: " << userId << " | Vehicle: " << vehicleId << "\n"
                  << "    Station: " << stationId << " | Dock: "
                  << (dockId.empty() ? "Not assigned" : dockId) << "\n"
                  << "    Time: " << startHour << ":00 - " << endHour << ":00"
                  << " | Dock Type: " << dockTypeStr(requestedDockType) << "\n"
                  << "    Target SOC: " << (targetSOC * 100) << "%"
                  << " | Priority: " << priorityScore << "\n";
    }
};
