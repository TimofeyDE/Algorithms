#ifndef __TD_SORTS_H__
#define __TD_SORTS_H__

#include <stddef.h>

/*
 * Description: The function sorts a given array of integers.
 * Parameters:
 * 	@arr is an array of integers
 *	@size is a size of the array
 * Return: Nothing
 * Time complexity: 
 * 	@Best:    O(n)
 * 	@Average: O(n^2)
 * 	@Worst:   O(n^2)
 * Space comlexity: O(1)
 */
void BubbleSort(int *arr, size_t size);

/*
 * Description: The function sorts a given array of integers.
 * Parameters:
 * 	@arr is an array of integers
 *	@size is a size of the array
 * Return: Nothing
 * Time complexity: 
 *	@Best:    O(n^2) 
 * 	@Average: O(n^2)
 * 	@Worst:   O(n^2)
 * Space complexity: O(1)
 */
void SelectionSort(int *arr, size_t size); 

/*
 * Description: The function sorts a given array of integers.
 * Parameters:
 * 	@arr is an array of integers
 *	@size is a size of the array
 * Return: Nothing
 * Time complexity: 
 *	@Best:    O(n)
 *	@Average: O(n^2)
 * 	@Worst:   O(n^2)
 * Space complexity: O(1)
 */
void InsertionSort(int *arr, size_t size); 

/*
 * Description: The function sorts a given array of integers.
 * Parameters:
 * 	@arr is an array of integers
 *	@size is a size of the array
 * Return: Nothing
 * Time complexity:
 * 	@Best:    O(n + k)
 *  @Average: O(n + k)
 * 	@Worst:   O(n + k)
 *  	@k = (maximum element - minimum element + 1)
 * Space complexity: O(n)
 */
void CountingSort(int *arr, size_t size);

/*
 * Description: The function sorts a given array of integers.
 * Parameters:
 * 	@arr is an array of integers
 *	@size is a size of the array
 * Return: Nothing
 * Complexity:
 * 	@Best:    O(d * n)
 *  @Average: O(d * (n + k))
 * 	@Worst:   O(n^2)
 *      @d is the number of digits in the maximum value
 *      @k is the number of buckets
 * Space complexity: O()
 */
void RadixSort(int *arr, size_t size);

#endif // __TD_SORTS_H__
