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


void print_state(CycleState s, int state_num)
{
    printf("State %d:\n", state_num);
    printf("%-20s %.2f K\n", "Temperature:", s.T);
    printf("%-20s %.1f Pa\n", "Pressure:", s.P);
    printf("%-20s %.1f J/kg\n\n", "Enthalpy:", s.h);
}


int main(void)
{
CycleState state1, state2, state3, state4;

    // Initialize state 1
    state1.T = T1_DEFAULT;
    state1.P = P1_DEFAULT;
    state1.h = CP_AIR * state1.T;

    print_state(state1, 1);

}

