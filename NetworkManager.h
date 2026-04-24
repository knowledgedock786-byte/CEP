#pragma once
#include "ChargingStation.h"
#include "Booking.h"
#include "User.h"
#include "PricingEngine.h"
#include "AnalyticsEngine.h"
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <iostream>
#include <iomanip>

// ============================================================
// NetworkManager - Orchestrates the entire EV Charging Network
// ============================================================
class NetworkManager {
private:
    std::string networkName;
    std::string cityName;
    double currentHour;

    std::vector<ChargingStation*>  stations;
    std::vector<User*>             users;
    std::vector<Booking*>          bookings;
    std::vector<EnergySource*>     energySources;

    PricingEngine  pricing;
    AnalyticsEngine* analytics;

    // ID counters
    static int bookingCounter;
    static int userCounter;
    static int vehicleCounter;

    // Booking queue (sorted by priority)
    std::vector<Booking*> pendingQueue;

    // Multi-language support
    std::string language; // "EN", "UR", "AR"

public:
    NetworkManager(const std::string& name, const std::string& city)
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
        std::cout << "  [NETWORK] Station added: " << station->getName() << "\n";
    }

    ChargingStation* findStation(const std::string& id) {
        for (auto* s : stations) if (s->getId() == id) return s;
        return nullptr;
    }

    // ============================================================
    // USER MANAGEMENT
    // ============================================================
    User* registerUser(const std::string& name, const std::string& email,
                       const std::string& phone,
                       MembershipLevel membership = MembershipLevel::BASIC) {
        std::string uid = "U" + std::to_string(++userCounter);
        User* u = new User(uid, name, email, phone, membership);
        users.push_back(u);
        std::cout << "\n  [REGISTER] User registered: " << name
                  << " [" << uid << "] - " << membershipStr(membership) << " member\n";
        return u;
    }

    EV* registerVehicle(User* user, const std::string& model,
                         VehicleType type, double batteryKWh,
                         double soc, const std::string& plate) {
        std::string vid = "V" + std::to_string(++vehicleCounter);
        EV* ev = new EV(vid, model, type, batteryKWh, soc, plate);
        user->registerVehicle(ev);
        std::cout << "  [REGISTER] Vehicle: " << model << " [" << vid << "]"
                  << " SOC: " << (soc * 100) << "% registered to " << user->getName() << "\n";
        return ev;
    }

    User* findUser(const std::string& id) {
        for (auto* u : users) if (u->getId() == id) return u;
        return nullptr;
    }

    // ============================================================
    // BOOKING MANAGEMENT
    // ============================================================
    Booking* bookSlot(User* user, EV* vehicle, const std::string& stationId,
                       DockType dockType, double startHour, double durationHours,
                       double targetSOC = 0.9) {
        std::cout << "\n  [BOOKING] " << user->getName() << " requesting slot...\n";

        // Validate
        ChargingStation* station = findStation(stationId);
        if (!station) {
            std::cout << "  [ERROR] Station " << stationId << " not found!\n";
            return nullptr;
        }

        if (!station->getIsOperational()) {
            std::cout << "  [ERROR] Station " << stationId << " is not operational!\n";
            return nullptr;
        }

        // Check wallet balance (rough estimate)
        double estimatedCost = 200.0; // Min estimate
        if (user->getWalletBalance() < estimatedCost) {
            std::cout << "  [WARNING] Low wallet balance. Current: PKR "
                      << user->getWalletBalance() << "\n";
        }

        std::string bid = "B" + std::to_string(++bookingCounter);
        Booking* booking = new Booking(
            bid, user->getId(), vehicle->getId(),
            stationId, dockType, startHour,
            startHour + durationHours, targetSOC,
            user->getPriorityLevel()
        );

        // Check peak hour load shedding - defer if needed
        if (pricing.isPeakHour(startHour) && station->getIsLoadShedding()) {
            std::cout << "  [DEFER] Peak-hour overload. Checking if deferrable...\n";
            booking->deferToOffPeak();
        }

        bookings.push_back(booking);
        pendingQueue.push_back(booking);
        sortPendingQueue();
        user->addBookingToHistory(bid);

        std::cout << "  [BOOKING CONFIRMED] ID: " << bid
                  << " | Station: " << station->getName()
                  << " | " << dockTypeStr(dockType)
                  << " | " << startHour << ":00-" << (startHour + durationHours) << ":00\n";

        user->sendNotification("Booking " + bid + " confirmed at " + station->getName() +
                               " from " + std::to_string((int)startHour) + ":00");

        return booking;
    }

    bool cancelBooking(const std::string& bookingId, User* user) {
        for (auto* b : bookings) {
            if (b->getId() == bookingId && b->getUserId() == user->getId()) {
                if (b->getStatus() == BookingStatus::IN_PROGRESS) {
                    std::cout << "  [CANCEL] Cannot cancel in-progress session!\n";
                    return false;
                }
                if (b->getStatus() == BookingStatus::CONFIRMED ||
                    b->getStatus() == BookingStatus::PENDING) {
                    b->setStatus(BookingStatus::CANCELLED);
                    user->addPenalty(b->getCancellationPenalty());
                    std::cout << "  [CANCEL] Booking " << bookingId
                              << " cancelled. Penalty: PKR "
                              << b->getCancellationPenalty() << "\n";
                    // Remove from pending queue
                    pendingQueue.erase(
                        std::remove(pendingQueue.begin(), pendingQueue.end(), b),
                        pendingQueue.end());
                    return true;
                }
            }
        }
        return false;
    }

    // Sort queue by priority (highest first)
    void sortPendingQueue() {
        std::sort(pendingQueue.begin(), pendingQueue.end(),
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

        std::cout << "\n  [SIMULATION] Processing hour " << hour << ":00\n";

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
                std::cout << "  [SESSION START] Booking " << booking->getId()
                          << " -> Dock " << booking->getDockId() << "\n";
            } else {
                std::cout << "  [QUEUE] Booking " << booking->getId()
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

            std::cout << "  [CHARGING] Booking " << booking->getId()
                      << " | Energy: " << std::fixed << std::setprecision(2)
                      << energy << " kWh | SOC: " << (vehicle->getSOC() * 100) << "%"
                      << " | Source: " << (useRenewable ? "Renewable" : "Grid") << "\n";

            // Auto-complete if target SOC reached
            if (vehicle->getSOC() >= booking->getTargetSOC()) {
                std::cout << "  [COMPLETE] Target SOC reached for booking " << booking->getId() << "\n";
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

            std::cout << "\n  [SESSION COMPLETE] Booking " << booking->getId() << "\n";
            inv.print();

            user->sendNotification("Session complete! Charged " +
                std::to_string((int)booking->getEnergyDelivered()) +
                " kWh. Invoice: " + inv.invoiceId);
        }

        // Remove from pending queue
        pendingQueue.erase(
            std::remove(pendingQueue.begin(), pendingQueue.end(), booking),
            pendingQueue.end());
    }

    // ============================================================
    // REAL-TIME AVAILABILITY
    // ============================================================
    void checkNetworkAvailability() const {
        std::cout << "\n  --- Real-Time Dock Availability ---\n";
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
    void initiateV2G(const std::string& stationId, EV* vehicle, double kw) {
        ChargingStation* station = findStation(stationId);
        if (!station) return;
        std::cout << "\n  [V2G] Vehicle-to-Grid initiated at " << station->getName() << "\n";
        station->executeV2G(vehicle, kw, 1.0);
    }

    // ============================================================
    // REPORTS
    // ============================================================
    void generateFullReport() const {
        analytics->generateFullReport();
    }

    void setLanguage(const std::string& lang) {
        language = lang;
        std::cout << "  [LANG] Interface language set to: " << lang << "\n";
    }

    void advanceTime(double hours = 1.0) {
        currentHour += hours;
        if (currentHour >= 24) currentHour -= 24;
    }

    double getCurrentHour() const { return currentHour; }

    void displayNetworkStatus() const {
        std::cout << "\n  === Network Status: " << networkName
                  << " | City: " << cityName
                  << " | Time: " << currentHour << ":00 ===\n";
        std::cout << "  Stations: " << stations.size()
                  << " | Users: " << users.size()
                  << " | Bookings: " << bookings.size() << "\n";
    }

private:
    void printBanner() {
        std::cout << "\n";
        std::cout << "  ████████████████████████████████████████████████████████████\n";
        std::cout << "  ██                                                        ██\n";
        std::cout << "  ██       SMART EV CHARGING NETWORK MANAGEMENT SYSTEM      ██\n";
        std::cout << "  ██              " << std::left << std::setw(40) << networkName << "      ██\n";
        std::cout << "  ██              City: " << std::left << std::setw(35) << cityName << "      ██\n";
        std::cout << "  ██                                                        ██\n";
        std::cout << "  ████████████████████████████████████████████████████████████\n\n";
    }
};

int NetworkManager::bookingCounter = 0;
int NetworkManager::userCounter    = 0;
int NetworkManager::vehicleCounter = 0;
