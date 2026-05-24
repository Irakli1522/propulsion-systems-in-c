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

void performance_metric(CycleState s1, CycleState s2, CycleState s3, CycleState s4)
{
    double W_c = CP_AIR * (s2.T - s1.T); /* J/kg */
    double W_t = CP_AIR * (s3.T - s4.T); /* J/kg */
    double W_net = W_t - W_c; /* J/kg */
    double Q_in = CP_AIR * (s3.T - s2.T); /* J/kg */
    double thermal_efficiency = W_net / Q_in; /* dimensionless */
    double BWR = W_c / W_t; /* dimensionless */
    double Q_out = CP_AIR * (s4.T - s1.T); /* J/kg */
    //double specific_work = W_net / fuel_mass_flow(s2, s3, FUEL_JET_A); /* J/kg */
    
    printf("Performance Metrics:\n");
    printf("%-25s %.4f J/kg\n", "Compressor Work:", W_c);
    printf("%-25s %.4f J/kg\n", "Turbine Work:", W_t);
    printf("%-25s %.4f J/kg\n", "Net Work:", W_net);
    printf("%-25s %.4f J/kg\n", "Heat Added:", Q_in);
    printf("%-25s %.4f\n", "Thermal Efficiency:", thermal_efficiency);
    printf("%-25s %.4f\n", "Back Work Ratio:", BWR);
    printf("%-25s %.4f J/kg\n", "Heat Rejected:", Q_out);
    //printf("%-25s %.4f J/kg\n", "Specific Work:", specific_work);   
    
}

void sweep_pressure_ratio(double T1, double P1, double T3, double eta_c, double eta_t)
{
    FILE *fp = fopen("brayton_cycle_data.csv", "w");
    if (fp == NULL) {
        printf("Error opening file for writing.\n");
        return;
    }
    fprintf(fp, "Pressure Ratio,Compressor Work,Turbine Work,Net Work,Heat Added,Thermal Efficiency,Back Work Ratio,Heat Rejected,Specific Work\n");
    double best_wnet = 0.0;
    int best_pr = 0;
    int pr;
    for (pr = (int)PR_MIN; pr <= (int)PR_MAX; pr++) {
        CycleState s1 = make_state(T1, P1);
        CycleState s2 = compress(s1, pr, eta_c);
        CycleState s3 = combustor(s2, T3);
        CycleState s4 = expand(s3, pr, eta_t);
        double W_c = CP_AIR * (s2.T - s1.T); /* J/kg */
        double W_t = CP_AIR * (s3.T - s4.T); /* J/kg */
        double W_net = W_t - W_c; /* J/kg */
        double Q_in = CP_AIR * (s3.T - s2.T); /* J/kg */
        double thermal_efficiency = W_net / Q_in; /* dimensionless */
        double BWR = W_c / W_t; /* dimensionless */
        double Q_out = CP_AIR * (s4.T - s1.T); /* J/kg */
        //double specific_work = W_net / fuel_mass_flow(s2, s3, FUEL_JET_A); /* J/kg */

        fprintf(fp, "%d,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f\n", pr, W_c, W_t, W_net, Q_in, thermal_efficiency, BWR, Q_out);

        if (W_net > best_wnet) {
            best_wnet = W_net;
            best_pr = pr;
        }
    }
    fclose(fp);
    printf("Best Pressure Ratio for Maximum Net Work: %d (%.2f kJ/kg)\n", best_pr, best_wnet/1000.0);


}

void user_input(void)
{
    int running = 1;
    char line[128];

    while (running)
    {
        double T1, P1, T3, PR, eta_c, eta_t;

        printf("\n=== BRAYTON CYCLE ANALYSER ===\n");
        printf("Enter -1 at any prompt to quit, or press Enter for default value.\n\n");

        /* T1 */
        printf("Enter T1 (K) [default %.2f]: ", T1_DEFAULT);
        fflush(stdout);
        fgets(line, sizeof(line), stdin);
        if (sscanf(line, "%lf", &T1) != 1) T1 = T1_DEFAULT;
        if (T1 < 0) { running = 0; break; }
        if (T1 < 150.0 || T1 > 400.0)
        {
            printf("T1 out of range (150–400 K). Please re-enter.\n");
            continue;
        }

        /* P1 */
        printf("Enter P1 (Pa) [default %.2f]: ", P1_DEFAULT);
        fflush(stdout);
        fgets(line, sizeof(line), stdin);
        if (sscanf(line, "%lf", &P1) != 1) P1 = P1_DEFAULT;
        if (P1 < 0) { running = 0; break; }
        if (P1 <= 0 || P1 > 200000.0)
        {
            printf("P1 out of range (0–200000 Pa). Please re-enter.\n");
            continue;
        }

        /* T3 */
        printf("Enter T3 (K) [default %.2f]: ", T3_DEFAULT);
        fflush(stdout);
        fgets(line, sizeof(line), stdin);
        if (sscanf(line, "%lf", &T3) != 1) T3 = T3_DEFAULT;
        if (T3 < 0) { running = 0; break; }
        if (T3 > 2000.0)
        {
            printf("T3 out of range (max 2000 K). Please re-enter.\n");
            continue;
        }

        /* PR */
        printf("Enter PR [default %.2f]: ", PR_DEFAULT);
        fflush(stdout);
        fgets(line, sizeof(line), stdin);
        if (sscanf(line, "%lf", &PR) != 1) PR = PR_DEFAULT;
        if (PR < 0) { running = 0; break; }
        if (PR < 1.0 || PR > 50.0)
        {
            printf("PR out of range (1–50). Please re-enter.\n");
            continue;
        }

        /* eta_c */
        printf("Enter eta_c (0.5–1.0) [default %.2f]: ", ETA_COMPRESSOR_DEFAULT);
        fflush(stdout);
        fgets(line, sizeof(line), stdin);
        if (sscanf(line, "%lf", &eta_c) != 1) eta_c = ETA_COMPRESSOR_DEFAULT;
        if (eta_c < 0) { running = 0; break; }
        if (eta_c < 0.5 || eta_c > 1.0)
        {
            printf("eta_c out of range (0.5–1.0). Please re-enter.\n");
            continue;
        }

        /* eta_t */
        printf("Enter eta_t (0.5–1.0) [default %.2f]: ", ETA_TURBINE_DEFAULT);
        fflush(stdout);
        fgets(line, sizeof(line), stdin);
        if (sscanf(line, "%lf", &eta_t) != 1) eta_t = ETA_TURBINE_DEFAULT;
        if (eta_t < 0) { running = 0; break; }
        if (eta_t < 0.5 || eta_t > 1.0)
        {
            printf("eta_t out of range (0.5–1.0). Please re-enter.\n");
            continue;
        }

        /* Run cycle */
        CycleState s1 = make_state(T1, P1);
        CycleState s2 = compress(s1, PR, eta_c);

        if (T3 <= s2.T)
        {
            printf("T3 (%.2f K) must be greater than compressor exit T2 (%.2f K). Please re-enter.\n",
                   T3, s2.T);
            continue;
        }

        CycleState s3 = combustor(s2, T3);
        CycleState s4 = expand(s3, PR, eta_t);

        /* Print results */
        printf("\n--- CYCLE STATES ---\n");
        print_state(s1, 1);
        print_state(s2, 2);
        print_state(s3, 3);
        print_state(s4, 4);

        printf("\n--- PERFORMANCE ---\n");
        performance_metric(s1, s2, s3, s4);
    }

    printf("\nExiting Brayton Cycle Analyser.\n");
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

    double fuel_flow = fuel_mass_flow(state2, state3, FUEL_JET_A);
    //printf("Fuel mass flow: %.4f kg/kg\n", fuel_flow);

    state4 = expand(state3, PR_DEFAULT, ETA_TURBINE_DEFAULT);
    print_state(state4, 4);

    //performance_metric(state1, state2, state3, state4);
    //sweep_pressure_ratio(T1_DEFAULT, P1_DEFAULT, T3_DEFAULT, ETA_COMPRESSOR_DEFAULT, ETA_TURBINE_DEFAULT);
    user_input();
}


