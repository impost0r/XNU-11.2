/*
 * Copyright (c) 2006-2018 Apple Computer, Inc. All rights reserved.
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

#ifndef SYS_MEMORYSTATUS_FREEZE_H
#define SYS_MEMORYSTATUS_FREEZE_H

#include <stdint.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/param.h>
#include <sys/kern_memorystatus.h>
#include <mach/resource_monitors.h>     // command/proc_name_t

typedef struct memorystatus_freeze_entry {
	int32_t pid;
	uint32_t flags;
	uint32_t pages;
} memorystatus_freeze_entry_t;

#define FREEZE_PROCESSES_MAX 20


/* Lists all the processes that are currently in the freezer. */
#define FREEZER_CONTROL_GET_PROCS        (2)

#define FREEZER_CONTROL_GET_PROCS_MAX_COUNT (FREEZE_PROCESSES_MAX * 2)

typedef struct _global_frozen_procs {
	size_t gfp_num_frozen;
	struct {
		pid_t fp_pid;
		proc_name_t fp_name;
	} gfp_procs[FREEZER_CONTROL_GET_PROCS_MAX_COUNT];
} global_frozen_procs_t;


#endif /* SYS_MEMORYSTATUS_FREEZE_H */
