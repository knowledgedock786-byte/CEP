#pragma once
#include <string>
#include <iostream>
#include <cmath>

// ============================================================
// EnergySource - Base Class (Polymorphism & Inheritance)
// ============================================================
class EnergySource {
protected:
    std::string name;
    double capacityKW;
    double currentOutputKW;
    bool isRenewable;
    double co2PerKWh; // kg CO2 per kWh

public:
    EnergySource(const std::string& name, double capacityKW, bool isRenewable, double co2PerKWh)
        : name(name), capacityKW(capacityKW), currentOutputKW(0.0),
          isRenewable(isRenewable), co2PerKWh(co2PerKWh) {}

    virtual ~EnergySource() = default;

    // Pure virtual - must be overridden
    virtual double getAvailablePower() const = 0;
    virtual std::string getType() const = 0;
    virtual void updateOutput(double hour) = 0;

    // Common getters
    std::string getName() const { return name; }
    double getCapacity() const { return capacityKW; }
    double getCurrentOutput() const { return currentOutputKW; }
    bool getIsRenewable() const { return isRenewable; }
    double getCO2PerKWh() const { return co2PerKWh; }

    virtual void displayInfo() const {
        std::cout << "[" << getType() << "] " << name
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

    std::string getType() const override { return "GRID"; }

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

    std::string getType() const override { return "SOLAR"; }

    void updateOutput(double hour) override {
        // Solar follows sun curve: peak at noon
        double sunIntensity = 0.0;
        if (hour >= 6 && hour <= 20) {
            // Bell curve peaking at hour 13
            double x = (hour - 13.0) / 4.0;
            sunIntensity = std::exp(-x * x);
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

    std::string getType() const override { return "WIND"; }

    void updateOutput(double hour) override {
        // Wind varies throughout the day
        double variation = 1.0 + 0.3 * std::sin(hour * 0.5);
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
