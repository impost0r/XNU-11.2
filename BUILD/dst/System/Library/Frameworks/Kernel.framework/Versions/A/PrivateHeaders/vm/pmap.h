/*
 * Copyright (c) 2000-2020 Apple Inc. All rights reserved.
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
/*
 * @OSF_COPYRIGHT@
 */
/*
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988,1987 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 */
/*
 *	File:	vm/pmap.h
 *	Author:	Avadis Tevanian, Jr.
 *	Date:	1985
 *
 *	Machine address mapping definitions -- machine-independent
 *	section.  [For machine-dependent section, see "machine/pmap.h".]
 */

#ifndef _VM_PMAP_H_
#define _VM_PMAP_H_

#include <mach/kern_return.h>
#include <mach/vm_param.h>
#include <mach/vm_types.h>
#include <mach/vm_attributes.h>
#include <mach/boolean.h>
#include <mach/vm_prot.h>

#include <kern/trustcache.h>



/*
 *	The following is a description of the interface to the
 *	machine-dependent "physical map" data structure.  The module
 *	must provide a "pmap_t" data type that represents the
 *	set of valid virtual-to-physical addresses for one user
 *	address space.  [The kernel address space is represented
 *	by a distinguished "pmap_t".]  The routines described manage
 *	this type, install and update virtual-to-physical mappings,
 *	and perform operations on physical addresses common to
 *	many address spaces.
 */

/* Copy between a physical page and a virtual address */
/* LP64todo - switch to vm_map_offset_t when it grows */
extern kern_return_t    copypv(
	addr64_t source,
	addr64_t sink,
	unsigned int size,
	int which);
#define cppvPsnk        1
#define cppvPsnkb      31
#define cppvPsrc        2
#define cppvPsrcb      30
#define cppvFsnk        4
#define cppvFsnkb      29
#define cppvFsrc        8
#define cppvFsrcb      28
#define cppvNoModSnk   16
#define cppvNoModSnkb  27
#define cppvNoRefSrc   32
#define cppvNoRefSrcb  26
#define cppvKmap       64       /* Use the kernel's vm_map */
#define cppvKmapb      25

extern boolean_t pmap_has_managed_page(ppnum_t first, ppnum_t last);


extern boolean_t        pmap_is_noencrypt(ppnum_t);
extern void             pmap_set_noencrypt(ppnum_t pn);
extern void             pmap_clear_noencrypt(ppnum_t pn);

/*
 * JMM - This portion is exported to other kernel components right now,
 * but will be pulled back in the future when the needed functionality
 * is provided in a cleaner manner.
 */

extern pmap_t   kernel_pmap;                    /* The kernel's map */
#define         pmap_kernel()   (kernel_pmap)

#define VM_MEM_SUPERPAGE        0x100           /* map a superpage instead of a base page */
#define VM_MEM_STACK            0x200

/* N.B. These use the same numerical space as the PMAP_EXPAND_OPTIONS
 * definitions in i386/pmap_internal.h
 */
#define PMAP_CREATE_64BIT          0x1

#if __x86_64__

#define PMAP_CREATE_EPT            0x2
#define PMAP_CREATE_KNOWN_FLAGS (PMAP_CREATE_64BIT | PMAP_CREATE_EPT)

#else

#define PMAP_CREATE_STAGE2         0
#if __arm64e__
#define PMAP_CREATE_DISABLE_JOP    0x4
#else
#define PMAP_CREATE_DISABLE_JOP    0
#endif
#if __ARM_MIXED_PAGE_SIZE__
#define PMAP_CREATE_FORCE_4K_PAGES 0x8
#else
#define PMAP_CREATE_FORCE_4K_PAGES 0
#endif /* __ARM_MIXED_PAGE_SIZE__ */
#if __arm64__
#define PMAP_CREATE_X86_64         0
#else
#define PMAP_CREATE_X86_64         0
#endif

/* Define PMAP_CREATE_KNOWN_FLAGS in terms of optional flags */
#define PMAP_CREATE_KNOWN_FLAGS (PMAP_CREATE_64BIT | PMAP_CREATE_STAGE2 | PMAP_CREATE_DISABLE_JOP | PMAP_CREATE_FORCE_4K_PAGES | PMAP_CREATE_X86_64)

#endif /* __x86_64__ */

#define PMAP_OPTIONS_NOWAIT     0x1             /* don't block, return
	                                         * KERN_RESOURCE_SHORTAGE
	                                         * instead */
#define PMAP_OPTIONS_NOENTER    0x2             /* expand pmap if needed
	                                         * but don't enter mapping
	                                         */
#define PMAP_OPTIONS_COMPRESSOR 0x4             /* credit the compressor for
	                                         * this operation */
#define PMAP_OPTIONS_INTERNAL   0x8             /* page from internal object */
#define PMAP_OPTIONS_REUSABLE   0x10            /* page is "reusable" */
#define PMAP_OPTIONS_NOFLUSH    0x20            /* delay flushing of pmap */
#define PMAP_OPTIONS_NOREFMOD   0x40            /* don't need ref/mod on disconnect */
#define PMAP_OPTIONS_ALT_ACCT   0x80            /* use alternate accounting scheme for page */
#define PMAP_OPTIONS_REMOVE     0x100           /* removing a mapping */
#define PMAP_OPTIONS_SET_REUSABLE   0x200       /* page is now "reusable" */
#define PMAP_OPTIONS_CLEAR_REUSABLE 0x400       /* page no longer "reusable" */
#define PMAP_OPTIONS_COMPRESSOR_IFF_MODIFIED 0x800 /* credit the compressor
	                                            * iff page was modified */
#define PMAP_OPTIONS_PROTECT_IMMEDIATE 0x1000   /* allow protections to be
	                                         * be upgraded */
#define PMAP_OPTIONS_CLEAR_WRITE 0x2000
#define PMAP_OPTIONS_TRANSLATED_ALLOW_EXECUTE 0x4000 /* Honor execute for translated processes */
#if defined(__arm__) || defined(__arm64__)
#define PMAP_OPTIONS_FF_LOCKED  0x8000
#define PMAP_OPTIONS_FF_WIRED   0x10000
#endif

#if     !defined(__LP64__)
extern vm_offset_t      pmap_extract(pmap_t pmap,
    vm_map_offset_t va);
#endif
extern void             pmap_change_wiring(     /* Specify pageability */
	pmap_t          pmap,
	vm_map_offset_t va,
	boolean_t       wired);

/* LP64todo - switch to vm_map_offset_t when it grows */
extern void             pmap_remove(    /* Remove mappings. */
	pmap_t          map,
	vm_map_offset_t s,
	vm_map_offset_t e);

extern void             pmap_remove_options(    /* Remove mappings. */
	pmap_t          map,
	vm_map_offset_t s,
	vm_map_offset_t e,
	int             options);

extern void             fillPage(ppnum_t pa, unsigned int fill);

#if defined(__LP64__)
extern void pmap_pre_expand(pmap_t pmap, vm_map_offset_t vaddr);
extern kern_return_t pmap_pre_expand_large(pmap_t pmap, vm_map_offset_t vaddr);
extern vm_size_t pmap_query_pagesize(pmap_t map, vm_map_offset_t vaddr);
#endif

mach_vm_size_t pmap_query_resident(pmap_t pmap,
    vm_map_offset_t s,
    vm_map_offset_t e,
    mach_vm_size_t *compressed_bytes_p);

extern void pmap_set_vm_map_cs_enforced(pmap_t pmap, bool new_value);
extern bool pmap_get_vm_map_cs_enforced(pmap_t pmap);

/* Inform the pmap layer that there is a JIT entry in this map. */
extern void pmap_set_jit_entitled(pmap_t pmap);

/* Ask the pmap layer if there is a JIT entry in this map. */
extern bool pmap_get_jit_entitled(pmap_t pmap);

/*
 * Tell the pmap layer what range within the nested region the VM intends to
 * use.
 */
extern void pmap_trim(pmap_t grand, pmap_t subord, addr64_t vstart, uint64_t size);

/*
 * Dump page table contents into the specified buffer.  Returns KERN_INSUFFICIENT_BUFFER_SIZE
 * if insufficient space, KERN_NOT_SUPPORTED if unsupported in the current configuration.
 * This is expected to only be called from kernel debugger context,
 * so synchronization is not required.
 */

extern kern_return_t pmap_dump_page_tables(pmap_t pmap, void *bufp, void *buf_end, unsigned int level_mask, size_t *bytes_copied);

/*
 * Indicates if any special policy is applied to this protection by the pmap
 * layer.
 */
bool pmap_has_prot_policy(pmap_t pmap, bool translated_allow_execute, vm_prot_t prot);

/*
 * Causes the pmap to return any available pages that it can return cheaply to
 * the VM.
 */
uint64_t pmap_release_pages_fast(void);

#define PMAP_QUERY_PAGE_PRESENT                 0x01
#define PMAP_QUERY_PAGE_REUSABLE                0x02
#define PMAP_QUERY_PAGE_INTERNAL                0x04
#define PMAP_QUERY_PAGE_ALTACCT                 0x08
#define PMAP_QUERY_PAGE_COMPRESSED              0x10
#define PMAP_QUERY_PAGE_COMPRESSED_ALTACCT      0x20
extern kern_return_t pmap_query_page_info(
	pmap_t          pmap,
	vm_map_offset_t va,
	int             *disp);

#if CONFIG_PGTRACE
int pmap_pgtrace_add_page(pmap_t pmap, vm_map_offset_t start, vm_map_offset_t end);
int pmap_pgtrace_delete_page(pmap_t pmap, vm_map_offset_t start, vm_map_offset_t end);
kern_return_t pmap_pgtrace_fault(pmap_t pmap, vm_map_offset_t va, arm_saved_state_t *ss);
#endif


struct pmap_legacy_trust_cache;

extern kern_return_t pmap_load_legacy_trust_cache(struct pmap_legacy_trust_cache *trust_cache,
    const vm_size_t trust_cache_len);

typedef enum {
	PMAP_TC_TYPE_PERSONALIZED,
	PMAP_TC_TYPE_PDI,
	PMAP_TC_TYPE_CRYPTEX,
	PMAP_TC_TYPE_ENGINEERING,
	PMAP_TC_TYPE_GLOBAL_FF00,
	PMAP_TC_TYPE_GLOBAL_FF01,
} pmap_tc_type_t;

#define PMAP_IMAGE4_TRUST_CACHE_HAS_TYPE 1
struct pmap_image4_trust_cache {
	// Filled by pmap layer.
	struct pmap_image4_trust_cache const *next;             // linked list linkage
	struct trust_cache_module1 const *module;                       // pointer into module (within data below)

	// Filled by caller.
	// data is either an image4,
	// or just the trust cache payload itself if the image4 manifest is external.
	pmap_tc_type_t type;
	size_t bnch_len;
	uint8_t const bnch[48];
	size_t data_len;
	uint8_t const data[];
};

typedef enum {
	PMAP_TC_SUCCESS = 0,
	PMAP_TC_UNKNOWN_FORMAT = -1,
	PMAP_TC_TOO_SMALL_FOR_HEADER = -2,
	PMAP_TC_TOO_SMALL_FOR_ENTRIES = -3,
	PMAP_TC_UNKNOWN_VERSION = -4,
	PMAP_TC_ALREADY_LOADED = -5,
	PMAP_TC_TOO_BIG = -6,
	PMAP_TC_RESOURCE_SHORTAGE = -7,
	PMAP_TC_MANIFEST_TOO_BIG = -8,
	PMAP_TC_MANIFEST_VIOLATION = -9,
	PMAP_TC_PAYLOAD_VIOLATION = -10,
	PMAP_TC_EXPIRED = -11,
	PMAP_TC_CRYPTO_WRONG = -12,
	PMAP_TC_OBJECT_WRONG = -13,
	PMAP_TC_UNKNOWN_CALLER = -14,
	PMAP_TC_UNKNOWN_FAILURE = -15,
} pmap_tc_ret_t;

#define PMAP_HAS_LOCKDOWN_IMAGE4_SLAB 1
extern void pmap_lockdown_image4_slab(vm_offset_t slab, vm_size_t slab_len, uint64_t flags);

extern pmap_tc_ret_t pmap_load_image4_trust_cache(
	struct pmap_image4_trust_cache *trust_cache, vm_size_t trust_cache_len,
	uint8_t const *img4_manifest,
	vm_size_t img4_manifest_buffer_len,
	vm_size_t img4_manifest_actual_len,
	bool dry_run);

extern bool pmap_is_trust_cache_loaded(const uuid_t uuid);
extern uint32_t pmap_lookup_in_static_trust_cache(const uint8_t cdhash[CS_CDHASH_LEN]);
extern bool pmap_lookup_in_loaded_trust_caches(const uint8_t cdhash[CS_CDHASH_LEN]);

extern bool pmap_in_ppl(void);

extern void *pmap_claim_reserved_ppl_page(void);
extern void pmap_free_reserved_ppl_page(void *kva);

extern void pmap_ledger_alloc_init(size_t);
extern ledger_t pmap_ledger_alloc(void);
extern void pmap_ledger_free(ledger_t);

extern kern_return_t pmap_cs_allow_invalid(pmap_t pmap);

#if __arm64__
extern bool pmap_is_exotic(pmap_t pmap);
#else /* __arm64__ */
#define pmap_is_exotic(pmap) false
#endif /* __arm64__ */


#endif  /* _VM_PMAP_H_ */
