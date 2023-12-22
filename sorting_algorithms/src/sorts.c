#include <stdlib.h> // malloc, free
#include <string.h> // memcpy
                    
#define True (1)
#define False (0)
                    
#ifdef DEBUG
#include <stdio.h>
#endif
                   
#include "sorts.h"  // sorting algorihms

void _Swap(int *ptr1, int *ptr2);

void BubbleSort(int *arr, size_t size)
{
    for (size_t idx = 0; idx < size; ++idx)
    {
        int is_swapped = False;
        for (size_t jdx = 0; jdx < size - 1; ++jdx)
        {
            if (arr[jdx] > arr[jdx + 1])
            {
#ifdef DEBUG
                printf("arr[j] = %d, arr[j + 1] = %d\n", arr[jdx], arr[jdx + 1]);            
#endif
                _Swap(&arr[jdx], &arr[jdx + 1]);
                is_swapped = True;
            }
        }

        if (False == is_swapped)
        {
            return;
        }
    }
}


void _Swap(int *ptr1, int *ptr2)
{
    int temp = *ptr1;
    *ptr1 = *ptr2;
    *ptr2 = temp;
}
