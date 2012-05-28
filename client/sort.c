/*
 * sort.c
 *
 *  Created on: 16.03.2012
 *      Author: YaroslavLitvinov
 *      Merge sorting recursive algorithm implementation.
 */


#include "sort.h"
#include "logfile.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h> //pid_t
#include <unistd.h> //getpid()


void copy_array( BigArrayPtr dst_array, const BigArrayPtr src_array, int array_len );
BigArrayPtr alloc_copy_array( const BigArrayPtr array, int array_len );



BigArrayPtr
alloc_array_fill_random( int array_len ){
	BigArrayPtr unsorted_array = malloc( sizeof(BigArrayItem)*array_len );

	pid_t pid = getpid();
	//fill array by random numbers
	srand((time_t)pid );
	for (int i=0; i<array_len; i++){
		unsorted_array[i]=rand();
	}
	return unsorted_array;
}


BigArrayPtr
alloc_merge_sort( const BigArrayPtr array, int array_len ){
	if ( array_len <= 1 )
		return alloc_copy_array( array, array_len );

	int middle = array_len/2;
	BigArrayPtr left = alloc_merge_sort( array, middle );
	BigArrayPtr right = alloc_merge_sort( array+middle, array_len-middle );

	BigArrayPtr result = merge( left, middle, right, array_len-middle );
	free(left);
	free(right);
	return result;
}

/**@param global_array_index is used to save result to correct place*/
BigArrayPtr
merge(
		const BigArrayPtr left_array, int left_array_len,
		const BigArrayPtr right_array, int right_array_len ){
	BigArrayPtr larray = left_array;
	BigArrayPtr rarray = right_array;
	BigArrayPtr result = malloc( sizeof(BigArrayItem) *(left_array_len+right_array_len));
	int current_result_index = 0;
	while ( left_array_len > 0 && right_array_len > 0 ){
		if ( larray[0] <= rarray[0]  ){
			result[current_result_index++] = larray[0];
			++larray;
			--left_array_len;
		}
		else{
			result[current_result_index++] = rarray[0];
			++rarray;
			--right_array_len;
		}
	}

	//if merge arrays not empty then it can hold last item
	if ( left_array_len > 0 ){
		copy_array( result+current_result_index, larray, left_array_len );
	}
	if ( right_array_len > 0 ){
		copy_array( result+current_result_index, rarray, right_array_len );
	}

	return result;
}


void copy_array( BigArrayPtr dst_array, const BigArrayPtr src_array, int array_len ){
	for ( int i=0; i < array_len; i++ )
		dst_array[i] = src_array[i];
}


BigArrayPtr alloc_copy_array( const BigArrayPtr array, int array_len ){
	BigArrayPtr newarray = malloc( sizeof(BigArrayItem)*array_len );
	for ( int i=0; i < array_len; i++ )
		newarray[i] = array[i];
	return newarray;
}

void print_array(const char* text, BigArrayPtr array, int len){
	/*use print_buffer to do logfile writing by blocks*/
	WRITE_LOG(LOG_DEBUG, text );
	int print_buf_size = 1024*4;
	char *print_buf = malloc(print_buf_size+1);
	print_buf[print_buf_size] = '\0';
	int printed = 0;
	for (int j=0; j<len; j++){
		int bytes = snprintf( print_buf+printed, print_buf_size - printed, ",%d ", array[j] );
		printed += bytes;
		if ( !bytes || printed >= print_buf_size ){
			WRITE_LOG(LOG_DEBUG, print_buf );
			printed = 0;
		}
	}
}

uint32_t array_crc( BigArrayPtr array, int len ){
	uint32_t crc = 0;
	if ( len >=1 ){
		crc = (crc+array[0]%CRC_ATOM) % CRC_ATOM;
	}
	else return 1; //empty array always sorted
	for ( int i=1; i < len; i++ )
	{
		crc = (crc+array[i]%CRC_ATOM) % CRC_ATOM;
	}
	return crc;
}

int test_sort_result( const BigArrayPtr unsorted, const BigArrayPtr sorted, int len ){
	uint32_t unsorted_crc = 0;
	uint32_t sorted_crc = 0;
	uint32_t initial;
	if ( len >=1 ){
		initial = sorted[0];
		unsorted_crc = (unsorted_crc+unsorted[0]% CRC_ATOM) % CRC_ATOM;
		sorted_crc = (sorted_crc+sorted[0]% CRC_ATOM) % CRC_ATOM;
	}
	else return 1;
	for ( int i=1; i < len; i++ ){
		unsorted_crc = (unsorted_crc+unsorted[i]% CRC_ATOM) % CRC_ATOM;
		sorted_crc = (sorted_crc+sorted[i]% CRC_ATOM) % CRC_ATOM;

		if ( initial > sorted[i] ){
			WRITE_FMT_LOG(LOG_ERR, "initial %u > sorted[i] %u", initial, sorted[i]);
			return 0;
		}
		else initial = sorted[i];
	}

	//crc test
	if ( unsorted_crc != sorted_crc ){
		WRITE_FMT_LOG(LOG_ERR, "CRC_FAILED %u %u", unsorted_crc, sorted_crc);
		return 0;
	}
	else{
		WRITE_FMT_LOG(LOG_ERR, "CRC_OK %u", sorted_crc);
	}

	return 1;
}

int run_sort( BigArrayPtr *unsorted, BigArrayPtr *sorted, int sortlen )
{
	*unsorted = alloc_array_fill_random( sortlen );
	*sorted = alloc_merge_sort( *unsorted, sortlen );

	if ( test_sort_result( *unsorted, *sorted, sortlen ) )
		return 1;
	else
		return 0;
	return 0;
}





