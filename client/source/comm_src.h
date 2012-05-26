/*
 * comm_src.h
 *
 *  Created on: 30.04.2012
 *      Author: yaroslav
 */

#ifndef COMM_SRC_H_
#define COMM_SRC_H_

#include "distr_common.h"

void
init_request_data_array( struct request_data_t *req_data, int len );

void
write_crc(const char *path, uint32_t crc );

void
write_histogram(const char *path, const struct Histogram *histogram );

int
read_requests_write_detailed_histograms( const char* readf, const char* writef, int nodeid,
		const BigArrayPtr source_array, int array_len);

int
read_range_request( const char *readf, struct request_data_t* sequence );

void
write_sorted_ranges( const char *writef, const char *readf,
		const struct request_data_t* sequence, const BigArrayPtr src_array);

#endif /* COMM_SRC_H_ */
