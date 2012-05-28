/*
 * comm_src.h
 *
 *  Created on: 30.04.2012
 *      Author: YaroslavLitvinov
 */

#ifndef COMM_SRC_H_
#define COMM_SRC_H_

#include "distr_common.h"

void
repreq_read_sorted_ranges( const char *readf, const char *writef, int nodeid,
		BigArrayPtr dst_array, int dst_array_len, int ranges_count );

void
write_sort_result( const char *writef, int nodeid, BigArrayPtr sorted_array, int len );

#endif /* COMM_SRC_H_ */
