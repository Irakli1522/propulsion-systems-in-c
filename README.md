# propulsion-systems-in-c

C programming for propulsion engineering — built project by project.

Covers space propulsion, aeronautical propulsion, gas turbines,
embedded systems, and real-time data acquisition.

## Projects

| # | Project | Domain | Status |
|---|---------|--------|--------|
| 01 | Mach & Atmosphere Calculator | Aeronautics / Space | ✅ Done |
| 02 | Brayton Cycle Analyser | Gas Turbines | ✅ Done |
| 03 | Telemetry Log Parser | Oil & Gas | 🔄 In Progress |
| 04 | Compressor Map Library | Aircraft Engines | ⬜ Not Started |
| 05 | Rocket Trajectory Simulator | Space Propulsion | ⬜ Not Started |
| 06 | PID Engine Controller | Embedded / FADEC | ⬜ Not Started |
| 07 | Real-Time Sensor DAQ | Test Bench | ⬜ Not Started |
| 08 | TCP Telemetry Server | SCADA / Oil & Gas | ⬜ Not Started |
| 09 | Gas Turbine Simulation | Turbomachinery | ⬜ Not Started |
| 10 | Test Bench Framework | All Domains | ⬜ Not Started |

## Build environment
- Language: C (C11)
- Compiler: gcc -Wall -Wextra -g -lm
- OS: macOS / Linux


## 01 · Mach & Atmosphere Calculator

Computes ISA standard atmosphere properties and isentropic flow 
conditions at any altitude and Mach number.

**Computes:** temperature, pressure, density, speed of sound, 
stagnation temperature T₀, stagnation pressure P₀, and flight regime.

**Build:**
```bash
gcc -Wall -Wextra -g -lm main.c -o mach
```


## 02 · Brayton Cycle Analyser

Models the thermodynamic cycle behind every jet engine and gas turbine.
Computes all four cycle states, thermal efficiency, net work output,
and back work ratio. Sweeps pressure ratios from 5 to 30 and writes
results to CSV.

**Build:**
```bash
gcc -Wall -Wextra -g -lm brayton.c -o brayton
```

## 03 · Telemetry Log Parser & Anomaly Detector

Reads a CSV sensor log file from a turbine or engine test bench,
parses every row into a dynamically allocated struct array, computes
per-channel statistics, and flags readings that exceed safety thresholds.
Writes a formatted anomaly report file.

**Channels monitored:** temperature, pressure, vibration, RPM

**Computes:** min, max, mean, RMS per channel. Anomaly count per channel.

**Build:**
```bash
gcc -Wall -Wextra -g -lm telemetry.c -o parser
./parser telemetry.csv
```