/*
 * Copyright (c) 2015 Apple Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 *
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

#pragma once

#ifdef CONFIG_PGTRACE
#include <stdbool.h>
#include <mach/vm_types.h>
#include <mach/mach_types.h>

#define RR_NUM_MAX              2
#define PGTRACE_STACK_DEPTH     8

typedef enum {
	PGTRACE_RW_LOAD,
	PGTRACE_RW_STORE,
	PGTRACE_RW_PREFETCH
} pgtrace_rw_t;

typedef struct {
	vm_offset_t ad_addr;
	uint64_t    ad_data;
} pgtrace_addr_data_t;

typedef struct {
	uint64_t            rr_time;
	pgtrace_rw_t        rr_rw;
	uint8_t             rr_num;
	pgtrace_addr_data_t rr_addrdata[RR_NUM_MAX];
} pgtrace_run_result_t;

#ifdef CONFIG_PGTRACE_NONKEXT
#else // CONFIG_PGTRACE_NONKEXT
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
	vm_offset_t addr;
	uint64_t    bytes;
} pgtrace_instruction_info_t;

typedef struct {
	uint64_t                id;
	pgtrace_run_result_t    res;
	void                    *stack[PGTRACE_STACK_DEPTH];
} log_t;

typedef int (*run_func_t)(uint32_t inst, vm_offset_t pa, vm_offset_t va, void *ss, pgtrace_run_result_t *res);
typedef bool (*decode_func_t)(uint32_t inst, void *ss, pgtrace_instruction_info_t *info);
typedef void (*write_func_t)(pgtrace_run_result_t res);

typedef struct {
	uint64_t            magic;
	char                *arch;
	char                *desc;
	decode_func_t       decode;
	run_func_t          run;
} decoder_t;

typedef struct {
	uint64_t        magic;
	char            *arch;
	char            *desc;
	write_func_t    write;
} logger_t;

//------------------------------------
// for pmap fault handler
//------------------------------------
int pgtrace_decode_and_run(uint32_t inst, vm_offset_t fva, vm_map_offset_t *cva_page, arm_saved_state_t *ss, pgtrace_run_result_t *res);
int pgtrace_write_log(pgtrace_run_result_t res);

//------------------------------------
// for kext
//------------------------------------
int pgtrace_init(decoder_t *decoder, logger_t *logger);
int pgtrace_add_probe(thread_t thread, vm_offset_t start, vm_offset_t end);
void pgtrace_clear_probe(void);
void pgtrace_start(void);
void pgtrace_stop(void);
bool pgtrace_active(void);
#ifdef __cplusplus
}
#endif
#endif // CONFIG_PGTRACE_NONKEXT
#endif
