#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

/*BUFFER AND CAPACITY CONSTANTS*/
#define INITIAL_CAPACITY 64
#define LINE_BUFFER_SIZE 256
#define TIMESTAMP_LEN 32
#define MAX_FILENAME_LEN 256
#define N_CHANNELS 4

/*SAFETY THERSOLDS - ANOMALY DETECTION LIMITS*/
#define T_max 850.0
#define T_min -40.0
#define P_max 120.0
#define P_min 0.5
#define V_max 15.0
#define N_max 15000.0
#define N_min 1000.0 

/*STATISTICAL AND NUMERICAL CONSTANTS*/
#define eps 1e-9
#define INVALID_SENTINEL -9999.0

typedef struct{
    char timestamp[TIMESTAMP_LEN]; /*string -- char array, not a pointer*/
    double temp_c; /* temp in celcius*/
    double pressure_bar; /*pressure in bar*/
    double vibration_g; /*vibration in g-force*/
    double rpm; /*shaft speed in RPM*/
    int is_anomaly; /*0=normal, 1= anomaly detected*/ 
} SensorReading;

typedef struct
{
    double min;
    double max;
    double mean;
    double rms;
    int n_anomalies;
} ChannelStats;

typedef struct{
    SensorReading *data; /*pointer to dynamically allocated array*/
    int count; /*how many readings are currently stored*/ 
    int capacity; /*how many slots are allocated*/
} LogBuffer;


/* Sets the LogBuffer to a known empty state before anyhing is allocated*/
void init_buffer(LogBuffer *buf)
{
    buf -> data = NULL; // points to nothing - starting state
    buf -> count = 0; // no reading stored yet
    buf -> capacity = 0; // no memory allocated yet
}

/* Releases all heap memory and resets the buffer to the same state as init_buffer*/
void free_buffer(LogBuffer *buf)
{
    free(buf -> data);
    buf -> data = NULL;
    buf -> count = 0;
    buf -> capacity = 0; 
}

/*Adds one SensorReading into the buffer. Grows the buffer automatically if its full*/
void push_reading(LogBuffer *buf, SensorReading reading)
{
    /* check if the buffer is full — count reached the allocated capacity */
    if (buf->count == buf->capacity)
    {
        /* decide new capacity:
           if this is the very first allocation (capacity == 0), use INITIAL_CAPACITY
           otherwise double the current capacity */
        int new_cap = (buf->capacity == 0) ? INITIAL_CAPACITY: buf->capacity * 2;
        /* attempt to resize the memory block to hold new_cap SensorReadings
           realloc either expands the existing block or allocates a new one and copies
           we use a temp pointer — never realloc directly into buf->data
           if realloc fails and we wrote into buf->data, we would lose the original pointer */
        SensorReading *tmp = realloc(buf->data, new_cap * sizeof(SensorReading));
        /* realloc returns NULL if it failed to allocate the requested memory
           the original buf->data is still valid at this point — so we free it
           then exit — there is no safe way to continue without memory */
        if (tmp == NULL)
        {
            perror("realloc"); /* prints the system error message to stderr */
            free(buf->data);  /* release the original block — do not leak it */
            exit(1); /* terminate the program — no recovery possible */
        }
        /* realloc succeeded — update the buffer to point to the new memory block
           tmp may point to a different address than buf->data did before */
        buf->data = tmp;
        /* update capacity to reflect the new number of available slots */
        buf->capacity = new_cap;
    }
    /* store the reading in the next available slot
       this is a full struct copy — the buffer owns its own copy of the data */
    buf->data[buf->count] = reading;
    /* increment count to reflect that one more reading is now stored */
    buf->count++;
}


void print_reading(SensorReading *r, int index)
{
    printf("[%4d] %-24s T=%.1f°C, P=%.1f bar V=%.1f g N=%.0f RPM\n", 
        index, r -> timestamp, r -> temp_c, r -> pressure_bar, r -> vibration_g, r-> rpm);
    
}

/*
Opens tge CSV file, reads every data row,
parses each into a SensorReading and calls
push_reading to store it
*/
int parse_csv(const char *filename, LogBuffer *buf)
{
    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
    {
        perror(filename);
        return -1;
    }

    char line[LINE_BUFFER_SIZE];
    if (fgets(line, sizeof(line), fp) == NULL)
    {
        fclose(fp);
        return 0;
    }

    int row_count = 0;
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        SensorReading r;
        r.is_anomaly = 0; // always initialise before sscanf

        int n = sscanf(line, "%31[^,],%lf,%lf,%lf,%lf",
                        r.timestamp,
                        &r.temp_c,
                        &r.pressure_bar,
                        &r.vibration_g,
                        &r.rpm);

        if (n != 5) continue; // skip malforemed rows 
        
        push_reading(buf, r);
        row_count++;
    } 
    
    fclose(fp);
    return row_count;

}

void compute_stats(LogBuffer *buf, ChannelStats stats[N_CHANNELS])
{
    stats[0].min = buf->data[0].temp_c;
    stats[0].max = buf->data[0].temp_c;

    stats[1].min = buf->data[0].pressure_bar;
    stats[1].max = buf->data[0].pressure_bar;

    stats[2].min = buf->data[0].vibration_g;
    stats[2].max = buf->data[0].vibration_g;

    stats[3].min = buf->data[0].rpm;
    stats[3].max = buf->data[0].rpm;

    double sum[N_CHANNELS] = {0.0};
    double sum_sq[N_CHANNELS] = {0.0};

    for (int i = 0; i < buf->count; i++)
    {
        SensorReading *r = &buf->data[i];


        /* channel 0 — temperature */
        if (r->temp_c < stats[0].min) stats[0].min = r->temp_c;
        if (r->temp_c > stats[0].max) stats[0].max = r->temp_c;
        sum[0]    += r->temp_c;
        sum_sq[0] += r->temp_c * r->temp_c;

        /* channel 1 — pressure */
        if (r->pressure_bar < stats[1].min) stats[1].min = r->pressure_bar;
        if (r->pressure_bar > stats[1].max) stats[1].max = r->pressure_bar;
        sum[1]    += r->pressure_bar;
        sum_sq[1] += r->pressure_bar * r->pressure_bar;

        /* channel 2 — vibration */
        if (r->vibration_g < stats[2].min) stats[2].min = r->vibration_g;
        if (r->vibration_g > stats[2].max) stats[2].max = r->vibration_g;
        sum[2]    += r->vibration_g;
        sum_sq[2] += r->vibration_g * r->vibration_g;

        /* channel 3 — rpm */
        if (r->rpm < stats[3].min) stats[3].min = r->rpm;
        if (r->rpm > stats[3].max) stats[3].max = r->rpm;
        sum[3]    += r->rpm;
        sum_sq[3] += r->rpm * r->rpm;

    }

    for (int i = 0; i < N_CHANNELS; i++)
    {
        stats[i].mean = sum[i] / buf->count;
        stats[i].rms = sqrt(sum_sq[i] / buf->count);
    }
}   

int detect_anomalies(LogBuffer *buf, ChannelStats stats[N_CHANNELS])
{
    int total = 0;

    for (int i = 0; i < buf->count; i++)
    {
        SensorReading *r = &buf->data[i];
        r->is_anomaly = 0;

        /* check temperature */
        if (r->temp_c > T_max || r->temp_c < T_min)
        {
            r->is_anomaly = 1;
            stats[0].n_anomalies++;
        }

        /* check pressure */
        if (r->pressure_bar > P_max || r->pressure_bar < P_min)
        {
            r->is_anomaly = 1;
            stats[1].n_anomalies++;
        }

        /* check vibration */
        if (r->vibration_g > V_max)
        {
            r->is_anomaly = 1;
            stats[2].n_anomalies++;
        }

        /* check RPM */
        if (r->rpm > N_max || r->rpm < N_min)
        {
            r->is_anomaly = 1;
            stats[3].n_anomalies++;
        }

        if (r->is_anomaly) total++;

    }

    return total;
}

void write_report(const char *filename, LogBuffer *buf,
                  ChannelStats stats[N_CHANNELS], int total_anomalies)
{

    FILE *fp = fopen(filename, "w");
    if (fp == NULL)
    {
        perror(filename);
        return;
    }

    /* Section 1 — header */
    fprintf(fp, "TURBINE TELEMETRY REPORT\n");
    fprintf(fp, "Total readings:  %d\n", buf->count);
    fprintf(fp, "Total anomalies: %d\n", total_anomalies);

    /* Section 2 — stats table */
    char *channel_names[N_CHANNELS] = {"Temperature(C)",
                                        "Pressure(bar)",
                                        "Vibration(g)",
                                        "RPM"};
    fprintf(fp, "\n%-20s %10s %10s %10s %10s %10s\n",
            "Channel", "Min", "Max", "Mean", "RMS", "Anomalies");

    for (int i = 0; i < N_CHANNELS; i++)
    {
        fprintf(fp, "%-20s %10.3f %10.3f %10.3f %10.3f %10d\n",
                channel_names[i],
                stats[i].min, stats[i].max,
                stats[i].mean, stats[i].rms,
                stats[i].n_anomalies);
    }

    /* Section 3 — anomaly list */
    fprintf(fp, "\nANOMALY LOG:\n");
    for (int i = 0; i < buf->count; i++)
    {
        SensorReading *r = &buf->data[i];
        if (r->is_anomaly)
        {
            fprintf(fp, "[%4d] %s  T=%.1f  P=%.1f  V=%.1f  N=%.0f\n",
                    i, r->timestamp,
                    r->temp_c, r->pressure_bar,
                    r->vibration_g, r->rpm);
        }
    }

    fclose(fp);
}


int main(void)
{
    /* declare the buffer and stats array */
    LogBuffer buf;
    ChannelStats stats[N_CHANNELS] = {0};

    /* initialise the buffer — sets data=NULL, count=0, capacity=0 */
    init_buffer(&buf);

    /* parse the CSV file into the buffer */
    int rows = parse_csv("telemetry.csv", &buf);

    /* check if any rows were parsed */
    if (rows <= 0)
    {
        printf("Error: no rows parsed from file.\n");
        free_buffer(&buf);
        return 1;
    }
    printf("Rows parsed: %d\n", rows);

    /* compute statistics for all 4 channels */
    compute_stats(&buf, stats);

    /* detect anomalies and get total count */
    int total_anomalies = detect_anomalies(&buf, stats);
    printf("Total anomalies found: %d\n", total_anomalies);

    /* print stats to terminal */
    printf("\n--- CHANNEL STATISTICS ---\n");
    printf("%-20s %10s %10s %10s %10s %10s\n",
           "Channel", "Min", "Max", "Mean", "RMS", "Anomalies");
    char *channel_names[N_CHANNELS] = {"Temperature(C)",
                                        "Pressure(bar)",
                                        "Vibration(g)",
                                        "RPM"};
    for (int i = 0; i < N_CHANNELS; i++)
    {
        printf("%-20s %10.3f %10.3f %10.3f %10.3f %10d\n",
               channel_names[i],
               stats[i].min, stats[i].max,
               stats[i].mean, stats[i].rms,
               stats[i].n_anomalies);
    }

    /* write the full report to file */
    write_report("report.txt", &buf, stats, total_anomalies);
    printf("\nReport written to report.txt\n");

    /* free all heap memory before exit */
    free_buffer(&buf);

    return 0;
}
