#include <math.h>
#include <stdio.h>

#define GAMMA 1.4
#define R_AIR 287.058
#define G0 9.80665
#define T_SL 288.15
#define P_SL 101325.0
#define RHO_SL 1.225
#define LAPSE_RATE 0.0065
#define STRATOSPHERE_LAPSE_RATE 0.001
#define H_TROPOPAUSE 11000.0
#define H_STRATOPAUSE 20000.0
#define T_TROPOPAUSE 216.65
#define P_TROPOPAUSE 22632.1
#define ISA_EXPONENT 5.25588
#define P0_EXPONENT 3.5

#define TEST_VELOCITY_MS 300.0
#define TEST_TEMPERATURE_K T_SL
#define TEST_ALTITUDE_M 15000.0
#define TEST_MACH 2.0
#define TABLE_MAX_ALTITUDE_M 30000.0
#define TABLE_STEP_ALTITUDE_M 1000.0

double isa_temperature(double altitude_m);
double isa_pressure(double altitude_m);
double isa_density(double pressure_pa, double temperature_k);
double speed_of_sound(double temperature_k);
double mach_number(double velocity_ms, double speed_of_sound_ms);
double stagnation_temperature(double static_temperature_k, double mach);
double stagnation_pressure(double static_pressure_pa, double mach);
void print_mach_result(double mach, double speed_of_sound_ms, double velocity_ms);
void print_atmosphere_result(double altitude_m);
void print_stagnation_result(double altitude_m, double mach);
void print_regime(double mach);
void print_atmosphere_table(double reference_velocity_ms);

int main(void)
{
    double speed_ms = speed_of_sound(TEST_TEMPERATURE_K);
    double mach = mach_number(TEST_VELOCITY_MS, speed_ms);

    print_mach_result(mach, speed_ms, TEST_VELOCITY_MS);
    print_atmosphere_result(TEST_ALTITUDE_M);
    print_stagnation_result(TEST_ALTITUDE_M, TEST_MACH);
    print_regime(mach);
    print_atmosphere_table(TEST_VELOCITY_MS);

    return 0;
}

double isa_temperature(double altitude_m)
{
    if (altitude_m < 0.0) {
        return T_SL;
    }

    if (altitude_m < H_TROPOPAUSE) {
        return T_SL - LAPSE_RATE * altitude_m;
    }

    if (altitude_m <= H_STRATOPAUSE) {
        return T_TROPOPAUSE;
    }

    return T_TROPOPAUSE + STRATOSPHERE_LAPSE_RATE * (altitude_m - H_STRATOPAUSE);
}

double isa_pressure(double altitude_m)
{
    if (altitude_m < 0.0) {
        return P_SL;
    }

    if (altitude_m < H_TROPOPAUSE) {
        double temperature_k = isa_temperature(altitude_m);

        return P_SL * pow(temperature_k / T_SL, ISA_EXPONENT);
    }

    if (altitude_m <= H_STRATOPAUSE) {
        double height_above_tropopause_m = altitude_m - H_TROPOPAUSE;

        return P_TROPOPAUSE
               * exp(-G0 * height_above_tropopause_m
                     / (R_AIR * T_TROPOPAUSE));
    }

    {
        double pressure_at_stratopause_pa = isa_pressure(H_STRATOPAUSE);
        double temperature_k = isa_temperature(altitude_m);

        return pressure_at_stratopause_pa
               * pow(temperature_k / T_TROPOPAUSE,
                     -G0 / (STRATOSPHERE_LAPSE_RATE * R_AIR));
    }
}

double isa_density(double pressure_pa, double temperature_k)
{
    if (temperature_k <= 0.0) {
        return -1.0;
    }

    return pressure_pa / (R_AIR * temperature_k);
}

double speed_of_sound(double temperature_k)
{
    if (temperature_k <= 0.0) {
        return -1.0;
    }

    return sqrt(GAMMA * R_AIR * temperature_k);
}

double mach_number(double velocity_ms, double speed_of_sound_ms)
{
    if (speed_of_sound_ms <= 0.0) {
        return -1.0;
    }

    return velocity_ms / speed_of_sound_ms;
}

double stagnation_temperature(double static_temperature_k, double mach)
{
    double factor = 1.0 + ((GAMMA - 1.0) / 2.0) * mach * mach;

    return static_temperature_k * factor;
}

double stagnation_pressure(double static_pressure_pa, double mach)
{
    double factor = 1.0 + ((GAMMA - 1.0) / 2.0) * mach * mach;

    return static_pressure_pa * pow(factor, P0_EXPONENT);
}

void print_mach_result(double mach, double speed_of_sound_ms, double velocity_ms)
{
    printf("Stage 1 / 3: Mach calculation\n");
    printf("%-20s %.2f m/s\n", "Velocity:", velocity_ms);
    printf("%-20s %.2f m/s\n", "Speed of sound:", speed_of_sound_ms);
    printf("%-20s %.3f\n\n", "Mach number:", mach);
}

void print_atmosphere_result(double altitude_m)
{
    double temperature_k = isa_temperature(altitude_m);
    double pressure_pa = isa_pressure(altitude_m);
    double density_kg_m3 = isa_density(pressure_pa, temperature_k);
    double speed_ms = speed_of_sound(temperature_k);

    printf("Stage 2: ISA atmosphere\n");
    printf("%-20s %.0f m\n", "Altitude:", altitude_m);
    printf("%-20s %.2f K\n", "Temperature:", temperature_k);
    printf("%-20s %.1f Pa\n", "Pressure:", pressure_pa);
    printf("%-20s %.4f kg/m3\n", "Density:", density_kg_m3);
    printf("%-20s %.2f m/s\n\n", "Speed of sound:", speed_ms);
}


void print_stagnation_result(double altitude_m, double mach)
{
    double temperature_k = isa_temperature(altitude_m);
    double pressure_pa = isa_pressure(altitude_m);
    double total_temperature_k = stagnation_temperature(temperature_k, mach);
    double total_pressure_pa = stagnation_pressure(pressure_pa, mach);

    printf("Stage 4: Stagnation conditions\n");
    printf("%-20s %.3f\n", "Mach number:", mach);
    printf("%-20s %.2f K\n", "Static temperature:", temperature_k);
    printf("%-20s %.2f K\n", "Stagnation temp:", total_temperature_k);
    printf("%-20s %.1f Pa\n", "Static pressure:", pressure_pa);
    printf("%-20s %.1f Pa\n\n", "Stagnation pressure:", total_pressure_pa);
}

void print_regime(double mach)
{
    printf("Stage 5: Flight regime\n");
    printf("%-20s %.3f\n", "Mach number:", mach);

    if (mach < 0.8) {
        printf("Subsonic flight regime\n\n");
    } else if (mach < 1.2) {
        printf("Transonic flight regime\n\n");
    } else if (mach < 5.0) {
        printf("Supersonic flight regime\n\n");
    } else {
        printf("Hypersonic flight regime\n\n");
    }
}

void print_atmosphere_table(double reference_velocity_ms)
{
    printf("Stage 6: ISA atmosphere table\n");
    printf("%8s  %7s  %10s  %11s  %8s  %6s  %-10s\n",
           "Alt(m)", "T(K)", "P(Pa)", "rho(kg/m3)", "a(m/s)", "M", "Regime");
    printf("--------  -------  ----------  -----------  --------  ------  ----------\n");

    for (double alt = 0.0; alt <= TABLE_MAX_ALTITUDE_M; alt += TABLE_STEP_ALTITUDE_M) {
        double temperature_k = isa_temperature(alt);
        double pressure_pa = isa_pressure(alt);
        double density_kg_m3 = isa_density(pressure_pa, temperature_k);
        double speed_ms = speed_of_sound(temperature_k);
        double mach = mach_number(reference_velocity_ms, speed_ms);
        const char *regime;

        if (mach < 0.8) {
            regime = "SUBSONIC";
        } else if (mach < 1.2) {
            regime = "TRANSONIC";
        } else if (mach < 5.0) {
            regime = "SUPERSONIC";
        } else {
            regime = "HYPERSONIC";
        }

        printf("%8.0f  %7.2f  %10.1f  %11.4f  %8.2f  %6.3f  %-10s\n",
               alt, temperature_k, pressure_pa, density_kg_m3, speed_ms, mach, regime);
    }
}
