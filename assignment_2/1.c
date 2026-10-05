#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define NUM_SAMPLES 100000
#define CONTINUOUS_BINS 50

double generate_uniform() {
    return (double)rand() / (double)RAND_MAX;
}

double generate_continuous_rv() {
    double U = generate_uniform();
    double theta = acos(2.0 * U - 1.0);
    return 2.0 * cos((theta + 4.0 * M_PI) / 3.0) - 1.0;
}

int generate_binomial_rv(int n, double p) {
    int successes = 0;
    for (int i = 0; i < n; i++) {
        if (generate_uniform() <= p) {
            successes++;
        }
    }
    return successes;
}

int main() {
    srand((unsigned int)time(NULL));

    // 1. Continuous Distribution Sampling & Binning
    int cont_bins[CONTINUOUS_BINS] = {0};
    double bin_width = 1.0 / CONTINUOUS_BINS;

    // 2. Discrete Binomial Sampling
    int bin_counts[31] = {0};
    int n = 30;
    double p = 0.4;

    for (int i = 0; i < NUM_SAMPLES; i++) {
        // Continuous
        double x_cont = generate_continuous_rv();
        if (x_cont >= 0.0 && x_cont < 1.0) {
            int bin = (int)(x_cont / bin_width);
            cont_bins[bin]++;
        }

        // Discrete
        int x_disc = generate_binomial_rv(n, p);
        if (x_disc >= 0 && x_disc <= n) {
            bin_counts[x_disc]++;
        }
    }

    // Continuous Histogram Data for gnuplot
    FILE *f_cont = fopen("continuous_data.dat", "w");
    for (int i = 0; i < CONTINUOUS_BINS; i++) {
        double bin_center = (i + 0.5) * bin_width;
        double density = (double)cont_bins[i] / (NUM_SAMPLES * bin_width);
        fprintf(f_cont, "%f %f\n", bin_center, density);
    }
    fclose(f_cont);

    // Binomial PMF Data for gnuplot
    FILE *f_disc = fopen("binomial_data.dat", "w");
    for (int k = 0; k <= n; k++) {
        double prob = (double)bin_counts[k] / NUM_SAMPLES;
        fprintf(f_disc, "%d %f\n", k, prob);
    }
    fclose(f_disc);
    return 0;
}