#include <stdio.h>
#include <math.h>

#define GAMMA 1.4
#define R_AIR 287.058
#define CP_AIR 1005.0
#define CV_AIR 718.0
#define GAMMA_MINUS1 (GAMMA - 1.0)
#define GAMMA_RATIO (GAMMA / GAMMA_MINUS1)
#define INV_GAMMA_RATIO (GAMMA_MINUS1 / GAMMA)

#define T1_DEFAULT 288.15
#define P1_DEFAULT 101325.0
#define T3_MAX 1600.0
#define T3_DEFAULT 1400.0
#define ETA_COMPRESSOR_DEFAULT 0.85
#define ETA_TURBINE_DEFAULT 0.88
#define PR_MIN 5.0
#define PR_MAX 30.0
#define PR_DEFAULT 10.0

typedef struct{
    double T; /** Temperature[K] */
    double P; /** Pressure[Pa] */
    double h; /** Enthalpy[J/kg] */
} CycleState;

typedef enum{
    FUEL_JET_A = 0, /* standard aviation kerosene */
    FUEL_METHANE = 1, /* natural gas turbines */
    FUEL_H2 = 2, /* hydrogen -- future turbines */
    FUEL_RP1 = 3, /* rocket propellant (kerosene) */
} FuelType;

double fuel_heating_value(FuelType fuel)
{
    switch (fuel) {
        case FUEL_JET_A:
            return 43e6; /* J/kg */
        case FUEL_METHANE:
            return 55.5e6; /* J/kg */
        case FUEL_H2:
            return 120e6; /* J/kg */
        case FUEL_RP1:
            return 43e6; /* J/kg */
        default:
            return 0.0;
    }
}

double fuel_mass_flow(CycleState inlet, CycleState outlet, FuelType fuel)
{
    double delta_h = outlet.h - inlet.h; /* J/kg */
    double heating_value = fuel_heating_value(fuel); /* J/kg */
    return delta_h / heating_value; /* kg of fuel per kg of air */
}

void print_state(CycleState s, int state_num)
{
    printf("State %d:\n", state_num);
    printf("%-20s %.2f K\n", "Temperature:", s.T);
    printf("%-20s %.1f Pa\n", "Pressure:", s.P);
    printf("%-20s %.1f J/kg\n\n", "Enthalpy:", s.h);
}

CycleState make_state(double T, double P)
{
    CycleState s;
    s.T = T;
    s.P = P;
    s.h = CP_AIR * T;
    return s;
}

CycleState compress(CycleState inlet, double pressure_ratio, double efficiency)
{
    CycleState outlet;
    /*Step 1: compute ideal exit temperature using isentropic relation*/
    double T2s = inlet.T * pow(pressure_ratio, INV_GAMMA_RATIO);
    /*Step 2: compute actual exit temperature using efficiency*/
    outlet.T = inlet.T + (T2s - inlet.T) / efficiency;
    /*Step 3: exit pressure = inlet pressure * pressure ratio*/
    outlet.P = inlet.P * pressure_ratio;
    /*Step 4: compute enthalpy from temperature*/
    outlet.h = CP_AIR * outlet.T;

    return outlet;
}

CycleState combustor(CycleState inlet, double T3)
{
    CycleState outlet;
    /*Step 1: pressure drop across combustor is negligible, so P3 = P2*/
    outlet.P = inlet.P;
    /*Step 2: temperature at exit is specified by T3*/
    outlet.T = T3;
    /*Step 3: compute enthalpy from temperature*/
    outlet.h = CP_AIR * outlet.T;

    return outlet;
}

CycleState expand(CycleState inlet, double pressure_ratio, double efficiency)
{
    CycleState outlet;
    /*Step 1: compute ideal exit temperature using isentropic relation*/
    double T4s = inlet.T * pow(1.0 / pressure_ratio, INV_GAMMA_RATIO);
    /*Step 2: compute actual exit temperature using efficiency*/
    outlet.T = inlet.T - (inlet.T - T4s) * efficiency;
    /*Step 3: exit pressure = inlet pressure / pressure ratio*/
    outlet.P = inlet.P / pressure_ratio;
    /*Step 4: compute enthalpy from temperature*/
    outlet.h = CP_AIR * outlet.T;

    return outlet;
}

double performance_metric(CycleState inlet, CycleState outlet)
{
    double W_c = CP_AIR * (inlet.T - outlet.T); /* J/kg */
    double W_t = CP_AIR * (outlet.T - inlet.T); /* J/kg */
    double W_net = W_t - W_c; /* J/kg */
    double Q_in = CP_AIR * (outlet.T - inlet.T); /* J/kg */
    double thermal_efficiency = W_net / Q_in; /* dimensionless */
    double BWR = W_c / W_t; /* dimensionless */
    double Q_out = CP_AIR * (outlet.T - inlet.T); /* J/kg */
    double specific_work = W_net * fuel_mass_flow(inlet, outlet, FUEL_H2); /* J/kg */
    
    printf("Performance Metrics:\n");
    printf("%-25s %.4f J/kg\n", "Compressor Work:", W_c);
    printf("%-25s %.4f J/kg\n", "Turbine Work:", W_t);
    printf("%-25s %.4f J/kg\n", "Net Work:", W_net);
    printf("%-25s %.4f J/kg\n", "Heat Added:", Q_in);
    printf("%-25s %.4f\n", "Thermal Efficiency:", thermal_efficiency);
    printf("%-25s %.4f\n", "Back Work Ratio:", BWR);
    printf("%-25s %.4f J/kg\n", "Specific Work:", specific_work);   
    
}

int main(void)
{
CycleState state1, state2, state3, state4;

    state1 = make_state(T1_DEFAULT, P1_DEFAULT);
    print_state(state1, 1);

    state2 = compress(state1, PR_DEFAULT, ETA_COMPRESSOR_DEFAULT);
    print_state(state2, 2);

    state3 = combustor(state2, T3_DEFAULT);
    print_state(state3, 3);

    double fuel_flow = fuel_mass_flow(state2, state3, FUEL_H2);
    //printf("Fuel mass flow: %.4f kg/kg\n", fuel_flow);

    state4 = expand(state3, PR_DEFAULT, ETA_TURBINE_DEFAULT);
    print_state(state4, 4);
}


