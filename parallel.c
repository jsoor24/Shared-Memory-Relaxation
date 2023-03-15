#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

double average(double n, double e, double s, double w);
void *relaxation_technique(void *arg);

// Struct to pass to the start routine
struct matrix_rectangle {
    int rows;
    int columns;
    int col_min;
    int col_max;
    int b_done;
    int counter;
    double precision;
    double *matrix;
};

double average(double n, double e, double s, double w) {
    return (n + e + s + w) / 4.0;
}

// Thread start routine
void *relaxation_technique(void *arg) {
    struct matrix_rectangle *rectangle = arg;
    int columns = rectangle->columns;
    int rows = rectangle->rows;
    rectangle->b_done = 1;

    // Create a temporary array to store the values
    // Ensures we don't use updated values to compute new values
    int k = 0;
    double *temp =
        malloc((size_t)(columns - 2) * (size_t)(rows - 2) * sizeof(double));
    if (temp == NULL) {
        printf("Malloc failed");
        return NULL;
    }

    // Loop over the 'inside' of the array
    // Don't change the 'padded' values
    for (int i = 1; i < rows - 1; i++) {
        for (int j = 1; j < columns - 1; j++) {
            double n = rectangle->matrix[(i - 1) * columns + j];
            double e = rectangle->matrix[i * columns + (j + 1)];
            double s = rectangle->matrix[(i + 1) * columns + j];
            double w = rectangle->matrix[i * columns + (j - 1)];
            double result = average(n, e, s, w);
            double prev = rectangle->matrix[i * columns + j];

            // Store the result
            temp[k++] = result;
            // Check if the difference is less than the required precision
            if (fabs(result - prev) > rectangle->precision) {
                rectangle->b_done = 0;
            }
        }
    }

    // Set the newly computed values
    k = 0;
    for (int i = 1; i < rows - 1; i++) {
        for (int j = 1; j < columns - 1; j++) {
            rectangle->matrix[i * columns + j] = temp[k++];
        }
    }

    // Tidy up
    free(temp);
    return 0;
}

int main(int argc, char *argv[]) {
    struct timespec start, stop;

    if (clock_gettime(CLOCK_REALTIME, &start) == -1) {
        printf("Error getting clock time");
        return 1;
    }

    // Variables needed throughout code
    int matrix_n = 10;
    int n_threads = 3;
    double precision = 0.00001;
    int file_n, a;

    // Read in command line arguments
    if (argc == 3) {
        char *p1, *p2;
        long conv1 = strtol(argv[1], &p1, 10);
        long conv2 = strtol(argv[2], &p2, 10);

        if (errno != 0 || *p1 != '\0' || *p2 != '\0') {
            printf("Error converting command line arguments");
            return 1;
        }

        matrix_n = (int)conv1;
        n_threads = (int)conv2;
    }

    printf(
        "Running program with %dx%d matrix and %d threads to %f precision\n\n",
        matrix_n, matrix_n, n_threads, precision);

    if (n_threads > matrix_n) {
        printf("Program doesn't support more threads than matrix size\n");
        return 1;
    }

    // Create sub-arrays
    // Splits the matrix into columns depending on the number of threads
    // Packages each sub-array with +2 padding
    // One column on the left and one column on the right
    // These padded columns aren't going to be changed by the sub-array but it
    // means that we don't have to lock a common resource as each sub-array is
    // independent

    // max_c is the index of the final column in the previous sub-array
    int max_c = 0;
    // div tells us how many elements to put in each column
    double div = (double)(matrix_n - 2) / (double)n_threads;
    int c;

    // Each sub-array is stored inside the 'matrix_rectangle' struct to pass to
    // the routine
    struct matrix_rectangle *rectangles =
        malloc((size_t)n_threads * sizeof(struct matrix_rectangle));
    if (rectangles == NULL) {
        printf("Malloc failed");
        return 1;
    }

    // Initialise each rectangle struct
    for (int i = 0; i < n_threads; i++) {
        c = (int)floor((i + 1) * div);

        // If its the last column, take the remaining columns from the matrix
        if (i == n_threads - 1)
            c = matrix_n - 2;

        // Tells us how many columns are going to be in the sub-array
        int columns = c + 2 - max_c;

        rectangles[i].columns = columns;
        rectangles[i].rows = matrix_n;
        rectangles[i].col_min = max_c;
        rectangles[i].col_max = c + 1;
        rectangles[i].b_done = 0;
        rectangles[i].counter = 0;
        rectangles[i].precision = precision;

        rectangles[i].matrix =
            malloc((size_t)columns * (size_t)matrix_n * sizeof(double));

        if (rectangles[i].matrix == NULL) {
            printf("Malloc failed");
            return 1;
        }

        max_c = c;
    }

    FILE *fr = fopen("numbers.txt", "r");
    if (fr == NULL) {
        printf("fopen failed");
        return 1;
    }
    // Read in the first value of the file
    // Gives info on how many numbers are present in file
    if (fscanf(fr, "%d", &file_n) == EOF) {
        printf("Error reading from file");
        return 1;
    }
    if (matrix_n * matrix_n > file_n) {
        printf("Provided n: %d. ", matrix_n);
        printf("Matrix %dx%d is larger than file size (%d)", matrix_n, matrix_n,
               file_n);
        return 1;
    }

    // Set the values of the sub-arrays from the file
    for (int i = 0; i < matrix_n; i++) {
        for (int j = 0; j < matrix_n; j++) {
            if (fscanf(fr, "%d", &a) == EOF) {
                printf("Error reading from file");
                return 1;
            }

            // printf("[%d, %d]: %.1f\n", i, j, (double)a);

            for (int r = 0; r < n_threads; r++) {
                int lb = rectangles[r].col_min;
                int ub = rectangles[r].col_max;

                // printf("r: %d: looking for [%d, %d].\nCols: %d\n", r, lb, ub,
                // column);

                // if lb <= j <= ub
                if (lb <= j && j <= ub) {
                    // Counter is a simple method but other methods resulted in
                    // memory leaks
                    rectangles[r].matrix[rectangles[r].counter++] = a;
                    // printf("I'll take it. Inserting into %dth position\n",
                    // rectangles[r].counter);
                }
            }
            // printf("\n");
        }
        // printf("\n");
    }
    // printf("\n");
    fclose(fr);

    // Start creating threads
    int b_break = 0;
    pthread_t *threads = malloc((size_t)n_threads * sizeof(pthread_t));
    if (threads == NULL) {
        printf("Malloc failed");
        return 1;
    }

    // Loop over until we reach required precision
    while (b_break == 0) {
        for (int r = 0; r < n_threads; r++) {
            // Create a new thread
            pthread_create(&threads[r], NULL, &relaxation_technique,
                           &rectangles[r]);
        }

        // Wait for each thread to finish
        // Check it's return value to make it didn't fail
        for (int r = 0; r < n_threads; r++) {
            void *retval;
            pthread_join(threads[r], &retval);
            if (retval != 0) {
                printf("Thread #%d failed\n", r);
                return 1;
            }
        }

        b_break = 1;
        for (int r = 0; r < n_threads; r++) {
            // Check to see if each thread is finished updating values
            if (rectangles[r].b_done == 0) {
                b_break = 0;
            }

            int rows = rectangles[r].rows;
            int columns = rectangles[r].columns;

            // Update the values of the padded columns from the other sub-arrays
            // we know that there is only ever 1 column overlap in the
            // sub-arrays so we can use this information to update the values
            for (int i = 0; i < rows; i++) {
                struct matrix_rectangle rect = rectangles[r];
                // If first rect; don't do this
                if (r != 0) {
                    struct matrix_rectangle prev = rectangles[r - 1];
                    // printf("updating rect #%d, col: %d with prev rect#%d,
                    // col: "
                    //        "%d\n",
                    //        r, 0, r - 1, prev.columns - 2);
                    rect.matrix[i * columns + 0] =
                        prev.matrix[i * prev.columns + prev.columns - 2];
                }
                // If last rect; don't do this
                if (r != n_threads - 1) {
                    struct matrix_rectangle next = rectangles[r + 1];
                    // printf("updating rect #%d, col: %d with next rect#%d,
                    // col: "
                    //        "%d\n",
                    //        r, rect.columns - 1, r + 1, 1);
                    rect.matrix[i * columns + rect.columns - 1] =
                        next.matrix[i * next.columns + 1];
                }
            }
        }
    }
    // We're done with the threads now
    free(threads);

    // Print out each sub-array
    // for (int r = 0; r < n_threads; r++) {
    //     printf("%d: ", r);
    //     int max_i = rectangles[r].rows;
    //     int max_j = rectangles[r].columns;
    //
    //     printf("%dx%d\n", max_i, max_j);
    //     for (int i = 0; i < max_i; i++) {
    //         for (int j = 0; j < max_j; j++) {
    //             printf("%f\t", rectangles[r].matrix[max_j * i + j]);
    //         }
    //         printf("\n");
    //     }
    // }
    // printf("END\n");

    // Create a matrix for the final result
    double *matrix =
        malloc((size_t)matrix_n * (size_t)matrix_n * sizeof(double));
    if (matrix == NULL) {
        printf("Malloc failed");
        return 1;
    }
    printf("Result:\n");
    for (int i = 0; i < matrix_n; i++) {
        for (int j = 0; j < matrix_n; j++) {
            for (int r = 0; r < n_threads; r++) {
                struct matrix_rectangle rect = rectangles[r];
                // Skip the sub-array if we're looking at information that it
                // couldn't contain
                if (j > rect.col_max) {
                    continue;
                }
                // printf("Getting value for [%d, %d] from rect #%d\n", i, j,
                // r);
                // Take the information we need
                matrix[i * matrix_n + j] =
                    rect.matrix[i * rect.columns + j - rect.col_min];
                break;
            }
            printf("%f\t", matrix[i * matrix_n + j]);
        }
        printf("\n");
    }

    // Free up memory
    for (int r = 0; r < n_threads; r++) {
        free(rectangles[r].matrix);
    }
    free(rectangles);
    free(matrix);

    // Get total time to compute
    if (clock_gettime(CLOCK_REALTIME, &stop) == -1) {
        printf("Error getting clock time");
        return 1;
    }

    double total_time =
        (double)(stop.tv_sec - start.tv_sec) +
        (double)(stop.tv_nsec - start.tv_nsec) / (double)1000000000L;

    printf("\n\nTotal time: %lf\n", total_time);

    return 0;
}