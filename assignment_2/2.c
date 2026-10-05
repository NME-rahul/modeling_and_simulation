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
    int head; // Index of the front job
    int tail; // Index where the next job enters
    int current_occupancy; // Number of jobs currently in queue
    
    long long occupancy_counter; // Cumulative sum for average queue length
    long long num_arrivals;
    long long utilization_counter;
    
    int remaining_service_time; // Remaining time for job in server
    bool server_busy;
} SingleQueue;

void initialize_routine(SingleQueue *obj) {
    obj->head = 0;
    obj->tail = 0;
    obj->current_occupancy = 0;
    obj->occupancy_counter = 0;
    obj->num_arrivals = 0;
    obj->utilization_counter = 0;
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

int draw_service_time(int select_service, int default_service_time) {
    if (select_service == 1) {
        return default_service_time;
    } else {
        double r = rndm();
        if (r < 1.0 / 3.0) return 1;
        else if (r < 2.0 / 3.0) return 2;
        else return 3;
    }
}

bool enqueue(SingleQueue *obj, Job new_job) {
    if (obj->current_occupancy >= MAX_QUEUE_SIZE) {
        fprintf(stderr, "Error: Queue Overflow!\n");
        return false;
    }
    obj->buffer[obj->tail] = new_job;
    obj->tail = (obj->tail + 1) % MAX_QUEUE_SIZE;
    obj->current_occupancy++;
    return true;
}

// Pop job from the queue
Job dequeue(SingleQueue *obj) {
    Job job = obj->buffer[obj->head];
    obj->head = (obj->head + 1) % MAX_QUEUE_SIZE;
    obj->current_occupancy--;
    return job;
}

bool arrival_routine(SingleQueue *obj, double injection_rate, int select_service, int service_time) {
    if (rndm() < injection_rate) {
        Job new_job;
        new_job.service_time = draw_service_time(select_service, service_time);
        
        enqueue(obj, new_job);
        obj->num_arrivals++;
        return true;
    }
    return false;
}

void runner(SingleQueue *obj) {
    obj->occupancy_counter += obj->current_occupancy;

    if (obj->server_busy) {
        obj->remaining_service_time--;
        obj->utilization_counter++;
        if (obj->remaining_service_time == 0) {
            obj->server_busy = false;
        }
    }

    if (!obj->server_busy && obj->current_occupancy > 0) {
        Job current_job = dequeue(obj); 
        obj->server_busy = true;
        obj->remaining_service_time = current_job.service_time;
    }
}

float **simulate(int service_type, double *injection_rate, int size_injection_rate,
                 int service_time, long long *simulation_length, int size_simulation_length) {

    SingleQueue *obj = malloc(sizeof(SingleQueue));
    float **average = malloc(size_injection_rate * sizeof(float *));

    for (int eir = 0; eir < size_injection_rate; eir++) {
        average[eir] = malloc(size_simulation_length * sizeof(float));

        for (int esml = 0; esml < size_simulation_length; esml++) {
            initialize_routine(obj);

            for (long long c = 0; c < simulation_length[esml]; c++) {
                arrival_routine(obj, injection_rate[eir], service_type, service_time);
                runner(obj);
            }

            average[eir][esml] = (double)obj->occupancy_counter / simulation_length[esml];
        }
    }

    free(obj);
    return average;
}

FILE *intialize_plot_routine(int size_simulation_length) {
    FILE *gp = popen("gnuplot -persistent", "w");

    fprintf(gp, "set title 'Geo/D/1 Vs PMF (Geo/G/1)'\n");
    fprintf(gp, "set xlabel 'Injection Rate (lambda)'\n");
    fprintf(gp, "set ylabel 'Queue Occupancy'\n");
    fprintf(gp, "set grid\n");
    fprintf(gp,
        "plot '-' with linespoints title 'Geo/D/1', "
        "'-' with linespoints title 'Custom Geo/G/1'\n");
    return gp;
}

void plot(FILE *gp, float **average, double *injection_rate, int size_injection_rate, int size_simulation_length) {
    for (int s = 0; s < size_simulation_length; s++) {
        for (int i = 0; i < size_injection_rate; i++) {
            double val = average[i][s];
            fprintf(gp, "%f %f\n", injection_rate[i], val);
        }
        fprintf(gp, "e\n");
    }
}

int main(int argc, char **argv) {
    int service_time = 2;
    double injection_rate[] = {0.1, 0.2, 0.3, 0.4, 0.5};
    int size_injection_rate = sizeof(injection_rate) / sizeof(double);

    long long simulation_length[] = {1000000};
    int size_simulation_length = sizeof(simulation_length) / sizeof(long long);

    float **average_d = simulate(1, injection_rate, size_injection_rate,
                                 service_time, simulation_length, size_simulation_length);

    float **average_geo = simulate(2, injection_rate, size_injection_rate,
                                   service_time, simulation_length, size_simulation_length);
    
    FILE *gp = intialize_plot_routine(size_simulation_length);

    plot(gp, average_d, injection_rate, size_injection_rate, size_simulation_length);
    plot(gp, average_geo, injection_rate, size_injection_rate, size_simulation_length);

    return 0;
}