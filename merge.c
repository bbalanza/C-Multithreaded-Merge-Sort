#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <ctype.h>
#include <omp.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>

#define MIN_TASK_SIZE 1000

long max(long a, long b)
{
    return (a >= b) ? a : b;
}

void swap_longs(long *a, long *b)
{
    long store = *a;
    *a = *b;
    *b = store;
    return;
}

long binary_search(int key, int *list, long first, long last)
{
    long low = first;
    long high = max(first, last + 1);

    while (low < high)
    {
        long mid = (low + high) / 2;
        if (key <= list[mid])
        {
            high = mid;
        }
        else
        {
            low = mid + 1;
        }
    }
    return high;
}

void merge(int *input_array,
           long left_subarray_first_index,
           long left_subarray_last_index,
           long right_subarray_first_index,
           long right_subarray_last_index,
           int *merged_array,
           long merged_array_first_index)
{
    long left_subarray_size = left_subarray_last_index - left_subarray_first_index + 1;
    long right_subarray_size = right_subarray_last_index - right_subarray_first_index + 1;

    if (left_subarray_size < right_subarray_size)
    {
        swap_longs(&left_subarray_first_index, &right_subarray_first_index);
        swap_longs(&left_subarray_last_index, &right_subarray_last_index);
        swap_longs(&left_subarray_size, &right_subarray_size);
    }

    if (left_subarray_size == 0)
    {
        return;
    }

    bool doOmp = left_subarray_size >= MIN_TASK_SIZE;

    long left_subarray_mid_index = (left_subarray_first_index + left_subarray_last_index) / 2;
    long right_subarray_mid_index = binary_search(input_array[left_subarray_mid_index],
                                                  input_array,
                                                  right_subarray_first_index,
                                                  right_subarray_last_index);

    long merged_array_mid_index = merged_array_first_index +
                                  (left_subarray_mid_index - left_subarray_first_index) +
                                  (right_subarray_mid_index - right_subarray_first_index);

    merged_array[merged_array_mid_index] = input_array[left_subarray_mid_index];

#pragma omp task shared(input_array, merged_array)                    \
    firstprivate(left_subarray_first_index,                           \
                 left_subarray_mid_index, right_subarray_first_index, \
                 right_subarray_mid_index, merged_array_first_index) if (doOmp)
    merge(input_array,
          left_subarray_first_index,
          left_subarray_mid_index - 1,
          right_subarray_first_index,
          right_subarray_mid_index - 1,
          merged_array,
          merged_array_first_index);

    // #pragma omp task shared(input_array, merged_array) \
    //                  firstprivate(left_subarray_mid_index, \
    //                                 left_subarray_last_index, right_subarray_mid_index, \
    //                                 right_subarray_last_index, merged_array_mid_index) \
    //                  if (doOmp)
    merge(input_array,
          left_subarray_mid_index + 1,
          left_subarray_last_index,
          right_subarray_mid_index,
          right_subarray_last_index,
          merged_array,
          merged_array_mid_index + 1);

#pragma omp taskwait
}

void merge_sort_recurse(int *input_array,
                        long input_array_first_index,
                        long input_array_last_index,
                        int *output_array,
                        long output_array_fist_index)
{

    long input_array_size = input_array_last_index - input_array_first_index + 1;
    bool doOmp = input_array_last_index - input_array_first_index >= MIN_TASK_SIZE;

    if (input_array_size == 1)
    {
        output_array[output_array_fist_index] = input_array[input_array_first_index];
    }
    else
    {
        int *temp = (int *)malloc(input_array_size * sizeof(int));
        long midpoint = (input_array_first_index + input_array_last_index) / 2;
        long temp_half_size = midpoint - input_array_first_index + 1;

#pragma omp task shared(input_array, temp) \
    firstprivate(input_array_first_index, midpoint) if (doOmp)
        merge_sort_recurse(input_array, input_array_first_index, midpoint, temp, 0);

#pragma omp task shared(input_array, temp) \
    firstprivate(input_array_last_index, midpoint, temp_half_size) if (doOmp)
        merge_sort_recurse(input_array, midpoint + 1, input_array_last_index, temp, temp_half_size);

#pragma omp taskwait

        merge(temp, 0, temp_half_size - 1, temp_half_size,
              input_array_size - 1, output_array, output_array_fist_index);

        free(temp);
    }
}

int cmp(const void *a, const void *b)
{
    return (*(int *)a - *(int *)b);
}

bool verify(int *initial, int *result, long length)
{
    qsort(initial, length, sizeof(int), cmp);

    for (int i = 0; i < length; i++)
    {
        if (initial[i] != result[i])
            return false;
    }

    return true;
}

void print_v(int *v, long l)
{
    printf("\n");
    for (long i = 0; i < l; i++)
    {
        if (i != 0 && (i % 10 == 0))
        {
            printf("\n");
        }
        printf("%d ", v[i]);
    }
    printf("\n");
}

typedef struct
{
    int *input;
    int *output;
    int64_t length;
} vectors_t;

void vectors_destroy(vectors_t *vectors)
{
    if (vectors->input)
        free(vectors->input);
    if (vectors->output)
        free(vectors->output);
}
int terminate_program(int exit_code, vectors_t *vectors)
{
    vectors_destroy(vectors);
    exit(EXIT_SUCCESS);
}

void vectors_allocate(vectors_t *vectors, int length)
{
    vectors->input = malloc(length * sizeof(int));
    vectors->output = malloc(length * sizeof(int));
    vectors->length = length;
    if (vectors->input == NULL || vectors->output == NULL)
    {
        fprintf(stderr, "Malloc failed...\n");
        terminate_program(EXIT_FAILURE, vectors);
    }
}

void vectors_fill_random_input(vectors_t *vectors)
{
    int seed = 42;
    srand(seed);
    for (size_t i = 0; i < vectors->length; i++)
    {
        vectors->input[i] = rand();
    }
}

void vectors_fill_descending_input(vectors_t *vectors)
{
    for (size_t i = vectors->length, k = 0; i > 0; i--, k++)
    {
        vectors->input[k] = i;
    }
}

void vectors_verify(vectors_t vectors)
{
    for (size_t i = 1; i < vectors.length; i++)
    {
        if (vectors.output[i] < vectors.output[i - 1])
        {
            printf("Fail\n");
            return;
        }
    }
    printf("Success\n");
    return;
}

void vectors_print_input(vectors_t vectors)
{
    print_v(vectors.input, vectors.length);
}

void vectors_print_output(vectors_t vectors)
{
    print_v(vectors.output, vectors.length);
}

void merge_sort(vectors_t *vectors)
{
    merge_sort_recurse(vectors->input, 0, vectors->length - 1, vectors->output, 0);
}

typedef struct
{
    int argc;
    char *const *argv;
    int64_t length;
    int num_threads;
    int debug;

} program_options_t;

void proram_options_copy_args(int argc, char *const *argv, program_options_t *program_options)
{
    program_options->argc = argc;
    program_options->argv = argv;
    program_options->length = 1000;
    program_options->num_threads = 8;
    program_options->debug = 0;
}

void program_options_parse_args(program_options_t *program_options)
{
    int c;
    while ((c = getopt(program_options->argc, program_options->argv, "gp:l:")) != -1)
    {
        switch (c)
        {
        case 'l':
            program_options->length = atol(optarg);
            break;
        case 'p':
            program_options->num_threads = atoi(optarg);
            break;
        case 'g':
            program_options->debug = 1;
            break;
        case '?':
            if (optopt == 'l' || optopt == 's')
            {
                fprintf(stderr, "Option -%c requires an argument.\n", optopt);
            }
            else if (isprint(optopt))
            {
                fprintf(stderr, "Unknown option '-%c'.\n", optopt);
            }
            else
            {
                fprintf(stderr, "Unknown option character '\\x%x'.\n", optopt);
            }
            exit(EXIT_FAILURE);
        default:
            return exit(EXIT_FAILURE);
        }
    }
}

void program_options_set_omp_thread_number(program_options_t program_options)
{
    omp_set_num_threads(program_options.num_threads);
}

int main(int argc, char **argv)
{
    vectors_t vectors;
    program_options_t program_options;
    struct timespec before, after;

    proram_options_copy_args(argc, argv, &program_options);
    program_options_parse_args(&program_options);
    program_options_set_omp_thread_number(program_options);
    vectors_allocate(&vectors, program_options.length);

    if (program_options.debug)
    {
        vectors_fill_descending_input(&vectors);
        vectors_print_input(vectors);
    }
    else
    {

        vectors_fill_random_input(&vectors);
    }

#pragma omp parallel shared(vectors, before, after)
    {
#pragma omp single
        {
            printf("Allocated %d threads\n", omp_get_num_threads());
            clock_gettime(CLOCK_MONOTONIC, &before);

            merge_sort(&vectors);

            clock_gettime(CLOCK_MONOTONIC, &after);
        }
    }

    double time = (double)(after.tv_sec - before.tv_sec) +
                  (double)(after.tv_nsec - before.tv_nsec) / 1e9;

    if (program_options.debug)
    {
        vectors_print_output(vectors);
    }

    vectors_verify(vectors);
    printf("Mergesort took: % .6e seconds \n", time);
}
