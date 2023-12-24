/*
 * project2 -n SIZE [-a ALTERNATE] [-s THRESHOLD] [-r SEED] [-m MULTITHREAD] [-p PIECES] [-t MAXTHREADS] [-m3 MEDIAN] [-e EARLY]
 * SIZE: [1 <= SIZE <= 1000000000]
 * ALTERNATE: [S/s/I/i]
 * THRESHOLD: [3 ≤ THRESHOLD < SIZE]
 * SEED: 
 * MULTITHREADED: [Y/y/N/n]
 * PIECES: [integer]
 * MAXTHREADS: [integer], (applies only if MULTITHREAD is 'Y'), (default: 4)
 * MEDIAN: [Y/y/N/n]
 * EARLY: [Y/y/N/n]
 * */

#define _GNU_SOURCE

#include <stdio.h>      /* perror, printf */
#include <stdlib.h>     /* exit */
#include <string.h>     /* strcmp */
#include <sys/time.h>   /* gettimeofday */
#include <pthread.h>    /* pthread */
#include <limits.h>     /* INT_MIN */
#include <errno.h>      /* ETIMEDOUT */

/*****************************************************
 *                      DEFINES                      *
 ****************************************************/
/* Minimal size of the array */
#define MIN_SIZE 1
/* Maximum size of the array */
#define MAX_SIZE 1000000000

/* Minimum value of the threshold option */
#define MIN_THRESHOLD 3

#define NUM_LOCATIONS 11

/* Early size in percantage */
#define EARLY_SIZE 0.25f

#define TIMEOUT_SEC 0.25f
#define THREAD_WAIT 0.25f

#define RATIO_SPLIT 0.7F

/* Path to the file */
#define DATA_FILE "random.dat"

#define TRUE 1
#define FALSE 0

/*****************************************************
 *                     TYPEDEFS                      *
 ****************************************************/
typedef struct cmd_options cmd_options_t;
typedef struct thread_info thread_info_t;
typedef struct segment segment_t;
typedef struct pq pq_t;
typedef struct pq_node pq_node_t;

struct cmd_options
{
    size_t size;        /* The size of the array */
    char alternate;     /* Specified which algorithm will be used */
    int threshold;      /* The size of the segment */
    int seed;           /* The start point of reading a file */
    int multithread;    /* Whether to use multiple threads or not */
    size_t pieces;      /* The number of pieces */
    int maxthreads;     /* The number of threads */
    int median;         /* To determine whether each segment will be partitioned */
    int early;
};

struct segment
{
    int *array;
    size_t left;
    size_t right; 
};

struct thread_info
{
    size_t pieces;
    int threshold;
    int median;
    int is_early;
};

struct pq_node 
{
    segment_t data;
    int priority;
    pq_node_t *next;
};

struct pq
{
    pq_node_t *head;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

/*****************************************************
 *                  Global variables                 *
 ****************************************************/
pthread_mutex_t lock;
pthread_cond_t cv;

/* Early thread */
pthread_t early_thread;
segment_t early_segment = {0};
thread_info_t early_thread_args = {0};

/* Multithreading */
pthread_t *threads = NULL;
segment_t *segments = NULL;
thread_info_t threads_info = {0};

pq_t *queue = NULL;

struct timeval load_start_time, load_end_time;
struct timeval sorting_start_time, sorting_end_time;
clock_t start, end;


/*****************************************************
 *                Function declarations              *
 ****************************************************/

/********************* Sorting ********************/
void Quicksort(int *array, int low, int high, int threshold, int median);
void Partition(int *array, int low, int high, size_t *i, size_t *j);
void ShellSort(int *array, int low, int high);
void MergeSortedSegments(int *array, segment_t *segments, int num_segments);

/********************* Parsing ********************/
void LoadArray(int *arr, size_t size, int seed);
int SecondOfTenPartition(int *arr, size_t size);
void DivideArray(int *array, const cmd_options_t *options, segment_t *segments);
int MedianOfThree(int *array, int low, int mid, int high);
size_t FindMaxIndex(int *arr, size_t len);
int *SplitArray(size_t len, size_t pieces, float ratio);

/********************* Threads ********************/
void Multithreaded(int *array, cmd_options_t *options);
void *QuicksortThread(void *thread_info);

/************** Additional functions **************/

/********************* Sorting ********************/
void Swap(int *a, int *b);
int IsSorted(int *array, size_t size);

/**************** Priority queue ******************/
int compare(const void *a, const void *b);
pq_t *CreateQueue();
void DestroyQueue(pq_t *queue);
void Push(pq_t *queue, segment_t data, int priority);
segment_t Pop(pq_t *queue);
segment_t Peek(pq_t *queue);
size_t Size(pq_t *queue);
int IsEmpty(pq_t *queue);

/********************* Parsing ********************/
int ParseArgv(const char **argv, size_t size, cmd_options_t *options);

/*****************************************************
 *              Function implementation              *
 ****************************************************/
void LoadArray(int *arr, size_t size, int seed) 
{
    FILE *fp = fopen(DATA_FILE, "rb");

    if (!fp) 
    {
        perror("Error opening data file");
        exit(EXIT_FAILURE);
    }

    if (seed >= 0) 
    {
        fseek(fp, seed * sizeof(int), SEEK_SET);
    } 
    else 
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        srand(tv.tv_usec);
        seed = rand() % 1000000000;
        fseek(fp, seed * sizeof(int), SEEK_SET);
    }

    gettimeofday(&load_start_time, NULL);

    size_t read_count = fread(arr, sizeof(int), size, fp);
    if (read_count < size) 
    {
        fseek(fp, 0, SEEK_SET);
        fread(arr + read_count, sizeof(int), size - read_count, fp);
    }

    gettimeofday(&load_end_time, NULL);

    fclose(fp);
}

void ShellSort(int *array, int low, int high)
{
    int n = high - low + 1;
    int h = 1;

    while (h < (n / 2)) 
    {
        h = 2 * h + 1;
    }

    while (h >= 1) 
    {
        for (size_t idx = low + h; idx <= high; ++idx) 
        {
            int key = array[idx];
            int j = idx - h;
            while (j >= low && array[j] > key) 
            {
                array[j + h] = array[j];
                j -= h;
            }
            array[j + h] = key;
        }
        h /= 2;
    }
}

void Quicksort(int *array, int low, int high, int threshold, int median)
{
    size_t size = high - low + 1;

    if (2 > size)
    {
        return;
    }
    else if (2 == size)
    {
        if (array[low] > array[high - 1])
        {
            Swap(&array[low], &array[high - 1]);
        }
    }
    else if (size <= threshold)
    {
        ShellSort(array, low, high);
    }

    if (1 == median) 
    {
        int mid = low + (high - low) / 2;
        int median_index = MedianOfThree(array, low, mid, high);
        Swap(&array[low], &array[median_index]);
    }

    size_t i = 0;
    size_t j = 0;
    Partition(array, low, high, &i, &j);

    if ((j - low) < (high - i)) 
    {
        Quicksort(array, low, j - 1, threshold, median);
        Quicksort(array, i, high, threshold, median);
    } 
    else 
    {
        Quicksort(array, i, high, threshold, median);
        Quicksort(array, low, j - 1, threshold, median);
    }
}

void Partition(int *array, int low, int high, size_t *i, size_t *j)
{
    /* Choose the first element in the subarray as the pivot */
    int pivot = array[low];

    /* Initialize i to the second element */ 
    *i = low + 1;
    /* Initialize j to the last element in the subarray */
    *j = high;

    /* Continue the loop until i and j pointers cross each other */
    while (TRUE) 
    {
        /* Move i pointer to the right until an element greater than the pivot is found */
        while (*i <= *j && array[*i] <= pivot) 
        {
            ++(*i);
        }

        /* Move j pointer to the left until an element less than the pivot is found */
        while (*i <= *j && array[*j] >= pivot) 
        {
            --(*j);
        }

        /* Exit the loop if the i and j pointers have crossed each other */
        if (*i > *j) 
        {
            break;
        }

        /* Swap the elements at positions i and j as they are on the wrong side of the pivot */
        Swap(&array[*i], &array[*j]);
    }

    /* Swap the pivot element with the element at position j, placing the pivot in its correct position */
    Swap(&array[low], &array[*j]);
}

int MedianOfThree(int *array, int low, int mid, int high) 
{
    /* Check if the value at 'lo' is the median of the three values */
    /* (array[low] - array[mid]) * (array[high] - array[low]) >= 0 is equivalent to 
            (array[low] >= array[mid] && array[low] <= array[high]) || (array[low] <= array[mid] && array[low] >= array[high]) */
    if ((array[low] - array[mid]) * (array[high] - array[low]) >= 0) 
    {
        return low;
    } 
    /* Check if the value at 'mid' is the median of the three values */
    /* (array[mid] - array[low]) * (array[high] - array[mid]) >= 0 is equivalent to 
            (array[mid] >= array[low] && array[mid] <= array[high]) || (array[mid] <= array[low] && array[mid] >= array[high]) */
    else if ((array[mid] - array[low]) * (array[high] - array[mid]) >= 0) 
    {
        return mid;
    } 
    /* If neither low nor mid is the median, then the value at high must be the median */
    else 
    {
        return high;
    }
}

int SecondOfTenPartition(int *arr, size_t size) 
{
    int locations[11];
    int values[11];
    int interval = size / 10;

    /* Set up the locations array */
    for (size_t idx = 0; idx < 11; ++idx) 
    {
        locations[idx] = idx * interval;
    }
    locations[10] = size - 1;

    /* Set up the values array */
    for (size_t idx = 0; idx < 11; ++idx) 
    {
        values[idx] = arr[locations[idx]];
    }

    /* Find the smallest value and its index */
    int smallest_value = values[0];
    int smallest_index = 0;

    for (size_t idx = 1; idx < 11; ++idx) 
    {
        if (values[idx] < smallest_value) 
        {
            smallest_value = values[idx];
            smallest_index = idx;
        }
    }

    /* Find the second smallest value and its index */
    int second_smallest_value = INT_MAX;
    int second_smallest_index = -1;

    for (size_t idx = 0; idx < 11; ++idx) 
    {
        if (values[idx] < second_smallest_value && idx != smallest_index) 
        {
            second_smallest_value = values[idx];
            second_smallest_index = idx;
        }
    }

    /* Use the index of the second smallest value from the values array to find its location in the locations array */
    int X = locations[second_smallest_index];

    return X;
}

void *QuicksortThread(void *thread_info) 
{
    thread_info_t t_info = *(thread_info_t *)thread_info;
    segment_t s_info = {0};
    size_t processed = 0;
    size_t size = 0;
    struct timespec timeout;
    int ret;

    while (TRUE)
    {
        pthread_mutex_lock(&lock);

        while (IsEmpty(queue))
        {
            clock_gettime(CLOCK_REALTIME, &timeout);
            timeout.tv_sec += THREAD_WAIT;

            if (pthread_cond_timedwait(&cv, &lock, &timeout) == ETIMEDOUT)
            {
                /* Timeout occurred, unlock the mutex and exit the thread */
                pthread_mutex_unlock(&lock);
                return NULL;
            }
        }
        
        s_info = Pop(queue);
        pthread_mutex_unlock(&lock);

        if (NULL == s_info.array)
        {
            return NULL;
        }

        size = s_info.right - s_info.left + 1;
        if (1 != t_info.is_early)
        {
            printf("%20s %10lu - %-10lu %2s %lu %2s\n", "Launching thread to sort", s_info.left, s_info.right, "(", size, ")");
        }
        
        /* Call the QuickSort function to sort the partition */
        Quicksort(s_info.array, s_info.left, s_info.right, t_info.threshold, t_info.median);

        pthread_cond_signal(&cv); /* Signal other threads that a new partition may be available. */
    }

    return NULL;
}

/*****************************************************
 *                    Main function                  *
 ****************************************************/
int main(int argc, const char *argv[]) 
{
    /****************************************** Declaration ******************************************************/

    queue = CreateQueue();

    /* The structure contains values of all options */
    cmd_options_t options = {0};

    /* Time structures for start and end points */
    struct timeval start_time;
    struct timeval end_time;

    /* The default values of the options */
    options.size = 0;        
    options.alternate = 'S'; 
    options.threshold = 10;  
    options.seed = -1;       
    options.multithread = TRUE; 
    options.pieces = 10;      
    options.maxthreads = 4;     
    options.median = FALSE;     
    options.early = FALSE;

    /****************************************** Preparation ******************************************************/

    /* Parsing of the argv array */
    if (0 != ParseArgv(argv, argc, &options))
    {
        return 1;
    }

    /* Creation of the array with specified size */
    int *array = (int *)malloc(sizeof(int) * options.size);
    if (NULL == array)
    {
        perror("Memory allocation is failure!");
        return 1;
    }

    /* Loading values for the array from the file */
    LoadArray(array, options.size, options.seed);

    /****************************************** Execution ******************************************************/

    /* To get a start time point */
    gettimeofday(&start_time, NULL);

    /* If EARLY is enabled */
    if (TRUE == options.early) 
    {
        size_t start = 0;
        size_t end = 0;
        size_t early_size = 0;

        /* Perform the "second of ten" partitioning */
        int X = SecondOfTenPartition(array, options.size);

        /* Swap Array[X] and Array[0] */
        Swap(&array[0], &array[X]);


        int total;
        size_t part;
        Partition(array, 0, options.size - 1, &start, &end);

        early_segment.array = array;
        early_segment.left = 0;
        early_segment.right = end;

        early_size = early_segment.right - early_segment.left + 1;

        early_thread_args.threshold = options.threshold;
        early_thread_args.median = options.median;
        early_thread_args.pieces = options.pieces;
        early_thread_args.is_early = 1;

        Push(queue, early_segment, early_size);

        /* Start the EARLY thread to process the segment */
        if (0 != pthread_create(&early_thread, NULL, QuicksortThread, &early_thread_args))
        {
            perror("Creation of the early thread is failure!");
            return 1;
        }

        printf("EARLY launching %lu to %lu (%.2f%%)\n", early_segment.left, early_segment.right, (early_size / (float)options.size) * 100);
    }

    size_t processed = 0;

    /* Launch threads */
    if (TRUE == options.multithread)
    {
        start = clock(); /* Get the starting CPU time */
        gettimeofday(&sorting_start_time, NULL);
        Multithreaded(array, &options);
    }
    /* If multithreaded is FALSE */
    else
    {
        start = clock(); /* Get the starting CPU time */
        gettimeofday(&sorting_start_time, NULL);
        Quicksort(array, 0, options.size - 1, options.threshold, options.median);
        gettimeofday(&sorting_end_time, NULL);
        end = clock(); /* Get the ending CPU time */
    }

    /* Wait early thread */
    if (TRUE == options.early)
    {
        printf("EARLY THREAD STILL RUNNING AT END\n");
        pthread_join(early_thread, NULL);
    }

    if (TRUE == options.multithread)
    {
        /* Wait for the other threads as you did before */
        /* Wait for all threads to finish */
        for (size_t thread = 0; thread < options.maxthreads; ++thread) 
        {
            pthread_join(threads[thread], NULL);
        }
        gettimeofday(&sorting_end_time, NULL);
        end = clock(); /* Get the ending CPU time */

        MergeSortedSegments(array, segments, options.pieces);
    }

    /****************************************** Resulting ******************************************************/

    /* To check whether the array is sorted or not */
    if (!IsSorted(array, options.size)) 
    {
        printf("ERROR - Data Not Sorted\n");
    }
    else
    {
        printf("\n");
    }
    
    /* To get a time point */
    gettimeofday(&end_time, NULL);

    double load_time = ((load_end_time.tv_sec - load_start_time.tv_sec) * 1e6 + (load_end_time.tv_usec - load_start_time.tv_usec)) / 1e6;
    double sorting_time = ((sorting_end_time.tv_sec - sorting_start_time.tv_sec) * 1e6 + (sorting_end_time.tv_usec - sorting_start_time.tv_usec)) / 1e6;
    double cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    double total_time = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec) / 1e6;

    printf("Load data: %.3f ", load_time);
    printf("Sort (Wall/CPU): %.3f / %.3f ", sorting_time, cpu_time_used);
    printf("Total: %.3f\n", total_time);

    if (TRUE == options.multithread)
    {
        free(threads);
        free(segments);
    }

    free(array);
    DestroyQueue(queue);
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cv);
    return 0;
}

/*****************************************************
 *                 Additional function               *
 ****************************************************/
int IsSorted(int *array, size_t size) 
{
    for (size_t idx = 0; idx < size - 1; idx++) 
    {
        if (array[idx] > array[idx + 1]) 
        {
            return 0;
        }
    }

    return 1;
}

void Swap(int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

int ParseArgv(const char **argv, size_t size, cmd_options_t *options)
{
    char option = '0';
    for (size_t idx = 1; idx < size; idx++) 
    {
        if (strcmp(argv[idx], "-n") == 0 && idx + 1 < size) 
        {
            options->size = atoi(argv[++idx]);
        } 
        else if (strcmp(argv[idx], "-a") == 0 && idx + 1 < size) 
        {
            options->alternate = argv[++idx][0];
        } 
        else if (strcmp(argv[idx], "-s") == 0 && idx + 1 < size) 
        {
            options->threshold = atoi(argv[++idx]);
        } 
        else if (strcmp(argv[idx], "-r") == 0 && idx + 1 < size) 
        {
            options->seed = atoi(argv[++idx]);
        } 
        else if (strcmp(argv[idx], "-m") == 0 && idx + 1 < size) 
        {
            option = argv[++idx][0];
            options->multithread = (option == 'Y' || option == 'y');
        } 
        else if (strcmp(argv[idx], "-p") == 0 && idx + 1 < size) 
        {
            options->pieces = atoi(argv[++idx]);
        } 
        else if (strcmp(argv[idx], "-t") == 0 && idx + 1 < size) 
        {
            options->maxthreads = atoi(argv[++idx]);
        } 
        else if (strcmp(argv[idx], "-m3") == 0 && idx + 1 < size) 
        {
            option = argv[++idx][0];
            options->median = (option == 'Y' || option == 'y');
        } 
        else if (strcmp(argv[idx], "-e") == 0 && idx + 1 < size) 
        {
            option = argv[++idx][0];
            options->early = (option == 'Y' || option == 'y');
        } 
        else 
        {
            printf("Invalid argument: %s\n", argv[idx]);
            return 1;
        }
    }

    if (options->size < MIN_SIZE || options->size > MAX_SIZE) 
    {
        printf("Invalid SIZE value: %lu\n", options->size);
        return 1;
    }

    if ('S' != options->alternate && 's' != options->alternate && 
            'I' != options->alternate && 'i' != options->alternate) 
    {
        printf("Invalid ALTERNATE value: %c\n", options->alternate);
        return 1;
    }

    if (options->threshold < MIN_THRESHOLD || options->threshold >= options->size) 
    {
        printf("Invalid THRESHOLD value: %d\n", options->threshold);
        return 1;
    }

    if (1 > options->pieces) 
    {
        printf("Invalid PIECES value: %lu\n", options->pieces);
        return 1;
    }

    if (1 > options->maxthreads || options->maxthreads > options->pieces) 
    {
        printf("Invalid MAXTHREADS value: %d\n", options->maxthreads);
        return 1;
    }

    return 0;
}

void Multithreaded(int *array, cmd_options_t *options)
{
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cv, NULL);

    threads = (pthread_t *)calloc(options->maxthreads, sizeof(pthread_t));
    if (NULL == threads)
    {
        perror("Allocation memory is failure!");
        return;
    }

    segments = (segment_t *)calloc(options->pieces, sizeof(segment_t));
    if (NULL == segments)
    {
        perror("Allocation memory is failure!");
        return;
    }

    /* Partition the array into segments and store their indices in thread_args */
    DivideArray(array, options, segments);

    /* To fill up the queue */
    qsort(segments, options->pieces, sizeof(segment_t), compare);
    for (size_t segment = 0; segment < options->pieces; ++segment)
    {
        size_t priority = segments[segment].right - segments[segment].left + 1;
        Push(queue, segments[segment], priority);
        printf("%lu %10lu - %-5lu\t(%lu - %5.2f)\n", segment, segments[segment].left, segments[segment].right, priority, ((float) priority / options->size) * 100);
    }

    threads_info.pieces = options->pieces;
    threads_info.threshold = options->threshold;
    threads_info.median = options->median;
    threads_info.is_early = 0;

    start = clock(); /* Get the starting CPU time */
    gettimeofday(&sorting_start_time, NULL);
    for (size_t thread = 0; thread < options->maxthreads; ++thread) 
    {
        if (0 != pthread_create(&threads[thread], NULL, QuicksortThread, &threads_info))
        {
            perror("Creation of the thread is failure!");
            return;
        }
    }
    gettimeofday(&sorting_end_time, NULL);
    end = clock(); /* Get the ending CPU time */
}

void DivideArray(int *array, const cmd_options_t *options, segment_t *segments)
{
    size_t index = 0;
    int *temp = SplitArray(options->size, options->pieces, RATIO_SPLIT);

    for (size_t piece = 0; piece < options->pieces; ++piece)
    {
        size_t size_of_segment = 0;

        segments[piece].array = array;
        segments[piece].left = index;
        segments[piece].right = index + temp[piece] - 1;

        size_of_segment = segments[piece].right - segments[piece].left + 1;

        index += temp[piece];

        size_t f_part = size_of_segment;
    }

    free(temp);
}

size_t FindMaxIndex(int *arr, size_t len)
{
    size_t max_index = 0;
    int max = arr[0];
    size_t i = 0;

    for( ; i < len; ++i)
    {
        if(arr[i] > max)
        {
            max = arr[i];
            max_index = i;
        }
    }

    return max_index;
}

int *SplitArray(size_t len, size_t pieces, float ratio)
{
    size_t i = 0;
    size_t curr_max_idx = 0;
    int temp = 0;
    int *out = (int*)malloc(sizeof(int) * pieces);
    out[0] = len;

    size_t index = 0;
    size_t left = 0;
    size_t right = index + out[0] - 1;
    size_t size_of_segment = right - left + 1;
    
    size_t size = len;
    size_t f_segment = 0;
    size_t s_segment = right;
    double f_share = 0;
    double s_share = 0;

    printf("Creating multithread partitions\n");
    printf("%-15s %10lu - %-8lu %2s %lu %2s %-2s", "Partitioning", left, right, "(", size_of_segment, ")", "...result:");
    for(i = 1; i < pieces; ++i)
    {
        curr_max_idx = FindMaxIndex(out, i);
        
        temp =  out[curr_max_idx];
        out[curr_max_idx] = (temp - 1) * ratio;
        out[i] = temp - out[curr_max_idx]; 

        left = index;
        right = index + out[i] - 1;

        size_of_segment = right - left + 1;

        index += out[i];

        f_segment = (size_t)out[curr_max_idx];
        s_segment = (size_t)out[i];

        f_share = (f_segment / (float)temp) * 100;
        s_share = (s_segment / (float)temp) * 100;

        printf("%10lu - %-10lu(%.2f / %.2f)\n", f_segment, s_segment, f_share, s_share);
        printf("%-15s %10lu - %-8lu %2s %lu %2s %-2s", "Partitioning", left, right, "(", size_of_segment, ")", "...result:");
        if ((i + 1) == pieces)
        {
            printf("%10lu - %-10lu(%.2f / %.2f)\n", f_segment, s_segment, f_share, s_share);
        }

    }

    return out;
}

int compare(const void *a, const void *b) 
{
    segment_t *segA = (segment_t *)a;
    segment_t *segB = (segment_t *)b;

    size_t size_a = segA->right - segA->left + 1;
    size_t size_b = segB->right - segB->left + 1;
    return size_a - size_b;
}

pq_t *CreateQueue() 
{
    pq_t *queue = (pq_t *)malloc(sizeof(pq_t));
    if (NULL == queue)
    {
        perror("Allocation memory is falied!");
        return NULL;
    }

    queue->head = NULL;
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->cond, NULL);
    return queue;
}

void DestroyQueue(pq_t *queue) 
{
    pthread_mutex_lock(&queue->mutex);
    while (!IsEmpty(queue)) 
    {
        Pop(queue);
    }
    pthread_mutex_unlock(&queue->mutex);
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->cond);
    free(queue);
}

void Push(pq_t *queue, segment_t data, int priority) 
{
    pthread_mutex_lock(&queue->mutex);

    pq_node_t *new_node = (pq_node_t *)malloc(sizeof(pq_node_t));
    if (NULL == new_node)
    {
        perror("Allocation memory is failure!");
        return;
    }
    new_node->data = data;
    new_node->priority = priority;
    new_node->next = NULL;

    if (queue->head == NULL || priority > queue->head->priority) 
    {
        new_node->next = queue->head;
        queue->head = new_node;
    } 
    else 
    {
        pq_node_t *current = queue->head;
        while (current->next != NULL && current->next->priority <= priority) 
        {
            current = current->next;
        }
        new_node->next = current->next;
        current->next = new_node;
    }

    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);
}

segment_t Pop(pq_t *queue) 
{
    pthread_mutex_lock(&queue->mutex);

    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += TIMEOUT_SEC;

    while (IsEmpty(queue)) 
    {
        if (pthread_cond_timedwait(&queue->cond, &queue->mutex, &timeout) == ETIMEDOUT) 
        {
            /* Handle the timeout, for example, by setting an "error" flag in the returned data */
            segment_t error_data = {NULL, -1, -1};
            pthread_mutex_unlock(&queue->mutex);
            return error_data;
        }
    }

    pq_node_t *temp = queue->head;
    segment_t dequeued_data = temp->data;

    queue->head = queue->head->next;
    free(temp);

    pthread_mutex_unlock(&queue->mutex);

    return dequeued_data;
}

segment_t Peek(pq_t *queue) 
{
    pthread_mutex_lock(&queue->mutex);

    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += TIMEOUT_SEC;

    while (IsEmpty(queue)) 
    {
        int result = pthread_cond_timedwait(&queue->cond, &queue->mutex, &timeout);
        if (result == ETIMEDOUT) 
        {
            /* Handle the timeout, for example, by setting an "error" flag in the returned data */
            segment_t error_data = {NULL, -1, -1};
            pthread_mutex_unlock(&queue->mutex);
            return error_data;
        }
    }

    segment_t first_data = queue->head->data;

    pthread_mutex_unlock(&queue->mutex);

    return first_data;
}

size_t Size(pq_t *queue) 
{
    pthread_mutex_lock(&queue->mutex);

    int count = 0;
    pq_node_t *current = queue->head;
    while (current != NULL) 
    {
        count++;
        current = current->next;
    }

    pthread_mutex_unlock(&queue->mutex);
    return count;
}

int IsEmpty(pq_t *queue) 
{
    return queue->head == NULL;
}

void MergeSortedSegments(int *array, segment_t *segments, int num_segments)
{
    size_t *indexes = (size_t *)calloc(num_segments, sizeof(size_t));
    int segment_idx = 0;
    size_t output_idx = 0;

    /* Keep looping until all segments have been fully merged */
    while (1)
    {
        int min_value = INT_MAX;
        int min_segment_idx = -1;

        /* Iterate through all segments and find the minimum value among their current elements */
        for (segment_idx = 0; segment_idx < num_segments; ++segment_idx)
        {
            size_t current_index = segments[segment_idx].left + indexes[segment_idx];
            if (current_index <= segments[segment_idx].right)
            {
                int current_value = segments[segment_idx].array[current_index];
                if (current_value < min_value)
                {
                    min_value = current_value;
                    min_segment_idx = segment_idx;
                }
            }
        }

        /* If min_segment_idx remains -1, it means all segments have been fully merged */
        if (min_segment_idx == -1)
        {
            break;
        }

        /* Add the minimum value to the primary array and update the corresponding index */
        segments[0].array[output_idx++] = min_value;
        indexes[min_segment_idx]++;
    }

    free(indexes);
}
