#include <stdio.h>	// printf
#include <stdlib.h> // srand

#include "sorts.h"	// sorting algorithms
			
#define True (1)
#define False (0)

#ifndef ACCURACY
#define ACCURACY (10)
#endif

#ifndef LENGTH
#define LENGTH (10)
#endif


void PrintArray(int *arr, size_t size);
int IsArraySorted(int *arr, size_t size);
void GenerateArray(int *arr, size_t size);
void BubbleSortTest(int is_print);

int main(void)
{
    int arr[10] = {345, -123, 0, 43, -472384, 9999, 9, 5, 11, -1};

    BubbleSortTest(1);
    return 0;
}


void BubbleSortTest(int is_print)
{
    int arr[LENGTH] = {0};

    GenerateArray(arr, LENGTH);

    if (True == is_print)
    {
        PrintArray(arr, LENGTH);
    }

    BubbleSort(arr, LENGTH);

    if (True == is_print)
    {
        PrintArray(arr, LENGTH);
    }

    if (False == IsArraySorted(arr, LENGTH))
    {
        printf("ERROR: Array was not sorted!\n");
    }
}


void PrintArray(int *arr, size_t size)
{
    printf("{");
    for (size_t idx = 0; idx < size; ++idx)
    {
        printf("%d", arr[idx]);
        if ((idx + 1) != size)
        {
            printf(", ");
        }
    }
    printf("}\n");
}


int IsArraySorted(int *arr, size_t size)
{
    for (size_t idx = 0; idx < size - 1; ++idx)
    {
        if (arr[idx] > arr[idx + 1])
        {
            return False;
        }
    }

    return True;
}


void GenerateArray(int *arr, size_t size)
{
    srand(100);

    for (size_t idx = 0; idx < size; ++idx)
    {
        arr[idx] = rand() % ACCURACY;
    }
}

