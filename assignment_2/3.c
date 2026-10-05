#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define MAX_QUEUE_SIZE 100000

typedef struct Job {
    int service_time;
} Job;

typedef struct SingleQueue {
    Job buffer[MAX_QUEUE_SIZE];
    int head;
    int tail;
    int current_occupancy;
    
    long long occupancy_counter;
    long long busy_cycles;
    long long num_arrivals;
    
    int remaining_service_time;
    bool server_busy;
} SingleQueue;

void initialize_routine(SingleQueue *obj) {
    obj->head = 0;
    obj->tail = 0;
    obj->current_occupancy = 0;
    obj->occupancy_counter = 0;
    obj->busy_cycles = 0;
    obj->num_arrivals = 0;
    obj->remaining_service_time = 0;
    obj->server_busy = false;
}

double rndm(void) {
#ifdef __APPLE__
    return (double)arc4random() / UINT32_MAX;
#else
    return (double)rand() / (double)RAND_MAX;
#endif
}

bool enqueue(SingleQueue *obj, Job new_job) {
    if (obj->current_occupancy >= MAX_QUEUE_SIZE) return false;
    obj->buffer[obj->tail] = new_job;
    obj->tail = (obj->tail + 1) % MAX_QUEUE_SIZE;
    obj->current_occupancy++;
    return true;
}

Job dequeue(SingleQueue *obj) {
    Job job = obj->buffer[obj->head];
    obj->head = (obj->head + 1) % MAX_QUEUE_SIZE;
    obj->current_occupancy--;
    return job;
}

void simulate_d_geo(SingleQueue *obj, double mu, long long cycle) {
    if (cycle % 2 == 0) {
        Job new_job = { .service_time = 0 };
        enqueue(obj, new_job);
        obj->num_arrivals++;
    }

    if (obj->server_busy) {
        if (rndm() < mu) {
            obj->server_busy = false; // Completion
        }
    }

    if (!obj->server_busy && obj->current_occupancy > 0) {
        dequeue(obj);
        obj->server_busy = true;
    }

    if (obj->server_busy) obj->busy_cycles++;
    obj->occupancy_counter += obj->current_occupancy + (obj->server_busy ? 1 : 0);
}

void simulate_geo_d_fixed(SingleQueue *obj, int fixed_D) {
    if (rndm() < 0.5) {
        Job new_job = { .service_time = fixed_D };
        enqueue(obj, new_job);
        obj->num_arrivals++;
    }

    if (obj->server_busy) {
        obj->remaining_service_time--;
        if (obj->remaining_service_time == 0) {
            obj->server_busy = false;
        }
    }

    if (!obj->server_busy && obj->current_occupancy > 0) {
        Job current_job = dequeue(obj);
        obj->server_busy = true;
        obj->remaining_service_time = current_job.service_time - 1;
        if (obj->remaining_service_time == 0) {
            obj->server_busy = false; // Finishes in same cycle if D = 1
        }
    }

    if (obj->server_busy) obj->busy_cycles++;
    obj->occupancy_counter += obj->current_occupancy + (obj->server_busy ? 1 : 0);
}

void generate_plots(float *occ_d_geo, float *occ_geo_d1, float *occ_geo_d2,
                    float *util_d_geo, float *util_geo_d1, float *util_geo_d2,
                    double *service_rates, int size) {
    
						FILE *gp1 = popen("gnuplot -persistent", "w");
    if (gp1) {
        fprintf(gp1, "set title 'Queue Occupancy Comparison (Log Scale)'\n");
        fprintf(gp1, "set xlabel 'Mean Service Rate (mu)'\n");
        fprintf(gp1, "set ylabel 'Average System Occupancy (L)'\n");
        fprintf(gp1, "set grid xtics ytics mytics\n");
        fprintf(gp1, "set logscale y 10\n");
        fprintf(gp1, "set yrange [0.1:*]\n"); 
        fprintf(gp1, "set xrange [0.45:1.05]\n");
        fprintf(gp1, "set key top right\n");
        fprintf(gp1, "plot '-' with linespoints lw 3 pt 7 ps 1.5 title 'D/Geo/1', "
                     "'-' with linespoints lw 3 pt 5 ps 1.5 title 'Geo/D/1 (D = 1)', "
                     "'-' with linespoints lw 3 pt 9 ps 1.5 title 'Geo/D/1 (D = 2)'\n");

        for (int i = 0; i < size; i++) fprintf(gp1, "%f %f\n", service_rates[i], occ_d_geo[i]);
        fprintf(gp1, "e\n");

        for (int i = 0; i < size; i++) fprintf(gp1, "%f %f\n", service_rates[i], occ_geo_d1[i]);
        fprintf(gp1, "e\n");

        for (int i = 0; i < size; i++) fprintf(gp1, "%f %f\n", service_rates[i], occ_geo_d2[i]);
        fprintf(gp1, "e\n");

        pclose(gp1);
    }

    FILE *gp2 = popen("gnuplot -persistent", "w");
    if (gp2) {
        fprintf(gp2, "set title 'Server Utilization Comparison'\n");
        fprintf(gp2, "set xlabel 'Mean Service Rate (mu)'\n");
        fprintf(gp2, "set ylabel 'Server Utilization (rho)'\n");
        fprintf(gp2, "set grid\n");
        fprintf(gp2, "set yrange [0.0:1.15]\n");
        fprintf(gp2, "set xrange [0.45:1.05]\n");
        fprintf(gp2, "set ytics 0.1\n");
        fprintf(gp2, "set key top right\n");
        fprintf(gp2, "plot '-' with linespoints lw 3 pt 7 ps 1.5 title 'D/Geo/1', "
                     "'-' with linespoints lw 3 pt 5 ps 1.5 title 'Geo/D/1 (D = 1)', "
                     "'-' with linespoints lw 3 pt 9 ps 1.5 title 'Geo/D/1 (D = 2)'\n");

        for (int i = 0; i < size; i++) fprintf(gp2, "%f %f\n", service_rates[i], util_d_geo[i]);
        fprintf(gp2, "e\n");

        for (int i = 0; i < size; i++) fprintf(gp2, "%f %f\n", service_rates[i], util_geo_d1[i]);
        fprintf(gp2, "e\n");

        for (int i = 0; i < size; i++) fprintf(gp2, "%f %f\n", service_rates[i], util_geo_d2[i]);
        fprintf(gp2, "e\n");

        pclose(gp2);
    }
}

int main(void) {
    double service_rates[] = {0.5, 0.6, 0.7, 0.8, 0.9, 1.0};
    int num_rates = sizeof(service_rates) / sizeof(double);
    long long sim_length = 1000000;

    float occ_d_geo[num_rates], util_d_geo[num_rates];
    float occ_geo_d1[num_rates], util_geo_d1[num_rates];
    float occ_geo_d2[num_rates], util_geo_d2[num_rates];

    SingleQueue *obj = malloc(sizeof(SingleQueue));

    // 1. D/Geo/1 Simulation
    for (int i = 0; i < num_rates; i++) {
        double mu = service_rates[i];
        initialize_routine(obj);
        for (long long c = 0; c < sim_length; c++) {
            simulate_d_geo(obj, mu, c);
        }
        occ_d_geo[i] = (float)((double)obj->occupancy_counter / sim_length);
        util_d_geo[i] = (float)((double)obj->busy_cycles / sim_length);
    }

    // 2. Geo/D/1 (D = 1) Simulation
    initialize_routine(obj);
    for (long long c = 0; c < sim_length; c++) {
        simulate_geo_d_fixed(obj, 1);
    }
    float occ_d1_val = (float)((double)obj->occupancy_counter / sim_length);
    float util_d1_val = (float)((double)obj->busy_cycles / sim_length);
    for (int i = 0; i < num_rates; i++) {
        occ_geo_d1[i] = occ_d1_val;
        util_geo_d1[i] = util_d1_val;
    }

    // 3. Geo/D/1 (D = 2) Simulation
    initialize_routine(obj);
    for (long long c = 0; c < sim_length; c++) {
        simulate_geo_d_fixed(obj, 2);
    }
    float occ_d2_val = (float)((double)obj->occupancy_counter / sim_length);
    float util_d2_val = (float)((double)obj->busy_cycles / sim_length);
    for (int i = 0; i < num_rates; i++) {
        occ_geo_d2[i] = occ_d2_val;
        util_geo_d2[i] = util_d2_val;
    }

    // Generate Scaled Plots
    generate_plots(occ_d_geo, occ_geo_d1, occ_geo_d2,
                   util_d_geo, util_geo_d1, util_geo_d2,
                   service_rates, num_rates);

    free(obj);
    return 0;
}