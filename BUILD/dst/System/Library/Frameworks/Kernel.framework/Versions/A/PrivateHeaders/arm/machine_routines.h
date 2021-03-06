/*
 * Copyright (c) 2007-2020 Apple Inc. All rights reserved.
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

#ifndef _ARM_MACHINE_ROUTINES_H_
#define _ARM_MACHINE_ROUTINES_H_

#include <mach/mach_types.h>
#include <mach/vm_types.h>
#include <mach/boolean.h>
#include <kern/kern_types.h>
#include <pexpert/pexpert.h>

#include <sys/cdefs.h>
#include <sys/appleapiopts.h>

#include <stdarg.h>

__BEGIN_DECLS


/* Interrupt handling */

void ml_cpu_signal(unsigned int cpu_id);
void ml_cpu_signal_deferred_adjust_timer(uint64_t nanosecs);
uint64_t ml_cpu_signal_deferred_get_timer(void);
void ml_cpu_signal_deferred(unsigned int cpu_id);
void ml_cpu_signal_retract(unsigned int cpu_id);
bool ml_cpu_signal_is_enabled(void);

/* Initialize Interrupts */
void    ml_init_interrupt(void);

/* Get Interrupts Enabled */
boolean_t ml_get_interrupts_enabled(void);

/* Set Interrupts Enabled */
boolean_t ml_set_interrupts_enabled(boolean_t enable);
boolean_t ml_early_set_interrupts_enabled(boolean_t enable);

/* Check if running at interrupt context */
boolean_t ml_at_interrupt_context(void);

/* Generate a fake interrupt */
void ml_cause_interrupt(void);

/* Clear interrupt spin debug state for thread */
#if INTERRUPT_MASKED_DEBUG
extern boolean_t interrupt_masked_debug;
extern uint64_t interrupt_masked_timeout;
extern uint64_t stackshot_interrupt_masked_timeout;

#define INTERRUPT_MASKED_DEBUG_START(handler_addr, type)                                    \
do {                                                                                        \
    if (interrupt_masked_debug) {                                                           \
	    thread_t thread = current_thread();                                                 \
	    thread->machine.int_type = type;                                                    \
	    thread->machine.int_handler_addr = (uintptr_t)VM_KERNEL_STRIP_PTR(handler_addr);    \
	    thread->machine.inthandler_timestamp = ml_get_timebase();                           \
	    thread->machine.int_vector = (uintptr_t)NULL;                                       \
    }                                                                                       \
} while (0)

#define INTERRUPT_MASKED_DEBUG_END()                                                        \
do {                                                                                        \
	if (interrupt_masked_debug) {                                                           \
	    thread_t thread = current_thread();                                                 \
	    ml_check_interrupt_handler_duration(thread);                                        \
	}                                                                                       \
} while (0)

void ml_irq_debug_start(uintptr_t handler, uintptr_t vector);
void ml_irq_debug_end(void);

void ml_spin_debug_reset(thread_t thread);
void ml_spin_debug_clear(thread_t thread);
void ml_spin_debug_clear_self(void);
void ml_check_interrupts_disabled_duration(thread_t thread);
void ml_check_stackshot_interrupt_disabled_duration(thread_t thread);
void ml_check_interrupt_handler_duration(thread_t thread);
#else
#define INTERRUPT_MASKED_DEBUG_START(handler_addr, type)
#define INTERRUPT_MASKED_DEBUG_END()
#endif


/* Type for the Time Base Enable function */
typedef void (*time_base_enable_t)(cpu_id_t cpu_id, boolean_t enable);

#define CacheConfig                     0x00000000UL
#define CacheControl                    0x00000001UL
#define CacheClean                      0x00000002UL
#define CacheCleanRegion                0x00000003UL
#define CacheCleanFlush                 0x00000004UL
#define CacheCleanFlushRegion           0x00000005UL
#define CacheShutdown                   0x00000006UL

#define CacheControlEnable              0x00000000UL

#define CacheConfigCCSIDR               0x00000001UL
#define CacheConfigSize                 0x00000100UL

/* Type for the Processor Idle function */
typedef void (*processor_idle_t)(cpu_id_t cpu_id, boolean_t enter, uint64_t *new_timeout_ticks);

/* Type for the Idle Tickle function */
typedef void (*idle_tickle_t)(void);

/* Type for the Idle Timer function */
typedef void (*idle_timer_t)(void *refcon, uint64_t *new_timeout_ticks);

/* Type for the IPI Hander */
typedef void (*ipi_handler_t)(void);

/* Type for the Lockdown Hander */
typedef void (*lockdown_handler_t)(void *);

/* Type for the Platform specific Error Handler */
typedef void (*platform_error_handler_t)(void *refcon, vm_offset_t fault_addr);

/*
 * The exception callback (ex_cb) module allows kernel drivers to
 * register and receive callbacks for exceptions, and indicate
 * actions to be taken by the platform kernel
 * Currently this is supported for ARM64 but extending support for ARM32
 * should be straightforward
 */

/* Supported exception classes for callbacks */
typedef enum{
	EXCB_CLASS_ILLEGAL_INSTR_SET,
#ifdef CONFIG_XNUPOST
	EXCB_CLASS_TEST1,
	EXCB_CLASS_TEST2,
	EXCB_CLASS_TEST3,
#endif
	EXCB_CLASS_MAX          // this must be last
}
ex_cb_class_t;

/* Actions indicated by callbacks to be taken by platform kernel */
typedef enum{
	EXCB_ACTION_RERUN,      // re-run the faulting instruction
	EXCB_ACTION_NONE,       // continue normal exception handling
#ifdef CONFIG_XNUPOST
	EXCB_ACTION_TEST_FAIL,
#endif
}
ex_cb_action_t;

/*
 * Exception state
 * We cannot use a private kernel data structure such as arm_saved_state_t
 * The CPSR and ESR are not clobbered when the callback function is invoked so
 * those registers can be examined by the callback function;
 * the same is done in the platform error handlers
 */
typedef struct{
	vm_offset_t far;
}
ex_cb_state_t;

/* callback type definition */
typedef ex_cb_action_t (*ex_cb_t) (
	ex_cb_class_t           cb_class,
	void                            *refcon,// provided at registration
	const ex_cb_state_t     *state  // exception state
	);

/*
 * Callback registration
 * Currently we support only one registered callback per class but
 * it should be possible to support more callbacks
 */
kern_return_t ex_cb_register(
	ex_cb_class_t   cb_class,
	ex_cb_t                 cb,
	void                    *refcon );

/*
 * Called internally by platform kernel to invoke the registered callback for class
 */
ex_cb_action_t ex_cb_invoke(
	ex_cb_class_t   cb_class,
	vm_offset_t         far);


void ml_parse_cpu_topology(void);

unsigned int ml_get_cpu_count(void);

unsigned int ml_get_cluster_count(void);

int ml_get_boot_cpu_number(void);

int ml_get_cpu_number(uint32_t phys_id);

int ml_get_cluster_number(uint32_t phys_id);

int ml_get_max_cpu_number(void);

int ml_get_max_cluster_number(void);

unsigned int ml_get_first_cpu_id(unsigned int cluster_id);

#ifdef __arm64__
int ml_get_cluster_number_local(void);
unsigned int ml_get_cpu_number_local(void);
#endif /* __arm64__ */

/* Struct for ml_cpu_get_info */
struct ml_cpu_info {
	unsigned long           vector_unit;
	unsigned long           cache_line_size;
	unsigned long           l1_icache_size;
	unsigned long           l1_dcache_size;
	unsigned long           l2_settings;
	unsigned long           l2_cache_size;
	unsigned long           l3_settings;
	unsigned long           l3_cache_size;
};
typedef struct ml_cpu_info ml_cpu_info_t;

typedef enum {
	CLUSTER_TYPE_SMP,
	CLUSTER_TYPE_E,
	CLUSTER_TYPE_P,
} cluster_type_t;

cluster_type_t ml_get_boot_cluster(void);

/*!
 * @typedef ml_topology_cpu_t
 * @brief Describes one CPU core in the topology.
 *
 * @field cpu_id            Logical CPU ID (EDT: cpu-id): 0, 1, 2, 3, 4, ...
 * @field phys_id           Physical CPU ID (EDT: reg).  Same as MPIDR[15:0], i.e.
 *                          (cluster_id << 8) | core_number_within_cluster
 * @field cluster_id        Cluster ID (EDT: cluster-id)
 * @field die_id            Die ID (EDT: die-id)
 * @field cluster_type      The type of CPUs found in this cluster.
 * @field l2_access_penalty Indicates that the scheduler should try to de-prioritize a core because
 *                          L2 accesses are slower than on the boot processor.
 * @field l2_cache_size     Size of the L2 cache, in bytes.  0 if unknown or not present.
 * @field l2_cache_id       l2-cache-id property read from EDT.
 * @field l3_cache_size     Size of the L3 cache, in bytes.  0 if unknown or not present.
 * @field l3_cache_id       l3-cache-id property read from EDT.
 * @field cpu_IMPL_regs     IO-mapped virtual address of cpuX_IMPL (implementation-defined) register block.
 * @field cpu_IMPL_pa       Physical address of cpuX_IMPL register block.
 * @field cpu_IMPL_len      Length of cpuX_IMPL register block.
 * @field cpu_UTTDBG_regs   IO-mapped virtual address of cpuX_UTTDBG register block.
 * @field cpu_UTTDBG_pa     Physical address of cpuX_UTTDBG register block, if set in DT, else zero
 * @field cpu_UTTDBG_len    Length of cpuX_UTTDBG register block, if set in DT, else zero
 * @field coresight_regs    IO-mapped virtual address of CoreSight debug register block.
 * @field coresight_pa      Physical address of CoreSight register block.
 * @field coresight_len     Length of CoreSight register block.
 * @field self_ipi_irq      AIC IRQ vector for self IPI (cpuX->cpuX).  0 if unsupported.
 * @field other_ipi_irq     AIC IRQ vector for other IPI (cpuX->cpuY).  0 if unsupported.
 * @field pmi_irq           AIC IRQ vector for performance management IRQ.  0 if unsupported.
 * @field die_cluster_id    Cluster ID within the local die (EDT: die-cluster-id)
 * @field cluster_core_id   Core ID within the local cluster (EDT: cluster-core-id)
 */
typedef struct ml_topology_cpu {
	unsigned int                    cpu_id;
	uint32_t                        phys_id;
	unsigned int                    cluster_id;
	unsigned int                    die_id;
	cluster_type_t                  cluster_type;
	uint32_t                        l2_access_penalty;
	uint32_t                        l2_cache_size;
	uint32_t                        l2_cache_id;
	uint32_t                        l3_cache_size;
	uint32_t                        l3_cache_id;
	vm_offset_t                     cpu_IMPL_regs;
	uint64_t                        cpu_IMPL_pa;
	uint64_t                        cpu_IMPL_len;
	vm_offset_t                     cpu_UTTDBG_regs;
	uint64_t                        cpu_UTTDBG_pa;
	uint64_t                        cpu_UTTDBG_len;
	vm_offset_t                     coresight_regs;
	uint64_t                        coresight_pa;
	uint64_t                        coresight_len;
	int                             self_ipi_irq;
	int                             other_ipi_irq;
	int                             pmi_irq;
	unsigned int                    die_cluster_id;
	unsigned int                    cluster_core_id;
} ml_topology_cpu_t;

/*!
 * @typedef ml_topology_cluster_t
 * @brief Describes one cluster in the topology.
 *
 * @field cluster_id        Cluster ID (EDT: cluster-id)
 * @field cluster_type      The type of CPUs found in this cluster.
 * @field num_cpus          Total number of usable CPU cores in this cluster.
 * @field first_cpu_id      The cpu_id of the first CPU in the cluster.
 * @field cpu_mask          A bitmask representing the cpu_id's that belong to the cluster.  Example:
 *                          If the cluster contains CPU4 and CPU5, cpu_mask will be 0x30.
 * @field acc_IMPL_regs     IO-mapped virtual address of acc_IMPL (implementation-defined) register block.
 * @field acc_IMPL_pa       Physical address of acc_IMPL register block.
 * @field acc_IMPL_len      Length of acc_IMPL register block.
 * @field cpm_IMPL_regs     IO-mapped virtual address of cpm_IMPL (implementation-defined) register block.
 * @field cpm_IMPL_pa       Physical address of cpm_IMPL register block.
 * @field cpm_IMPL_len      Length of cpm_IMPL register block.
 */
typedef struct ml_topology_cluster {
	unsigned int                    cluster_id;
	cluster_type_t                  cluster_type;
	unsigned int                    num_cpus;
	unsigned int                    first_cpu_id;
	uint64_t                        cpu_mask;
	vm_offset_t                     acc_IMPL_regs;
	uint64_t                        acc_IMPL_pa;
	uint64_t                        acc_IMPL_len;
	vm_offset_t                     cpm_IMPL_regs;
	uint64_t                        cpm_IMPL_pa;
	uint64_t                        cpm_IMPL_len;
} ml_topology_cluster_t;

// Bump this version number any time any ml_topology_* struct changes, so
// that KPI users can check whether their headers are compatible with
// the running kernel.
#define CPU_TOPOLOGY_VERSION 1

/*!
 * @typedef ml_topology_info_t
 * @brief Describes the CPU topology for all APs in the system.  Populated from EDT and read-only at runtime.
 * @discussion This struct only lists CPU cores that are considered usable by both iBoot and XNU.  Some
 *             physically present CPU cores may be considered unusable due to configuration options like
 *             the "cpus=" boot-arg.  Cores that are disabled in hardware will not show up in EDT at all, so
 *             they also will not be present in this struct.
 *
 * @field version           Version of the struct (set to CPU_TOPOLOGY_VERSION).
 * @field num_cpus          Total number of usable CPU cores.
 * @field max_cpu_id        The highest usable logical CPU ID.
 * @field num_clusters      Total number of AP CPU clusters on the system (usable or not).
 * @field max_cluster_id    The highest cluster ID found in EDT.
 * @field cpus              List of |num_cpus| entries.
 * @field clusters          List of |num_clusters| entries.
 * @field boot_cpu          Points to the |cpus| entry for the boot CPU.
 * @field boot_cluster      Points to the |clusters| entry which contains the boot CPU.
 * @field chip_revision     Silicon revision reported by iBoot, which comes from the
 *                          SoC-specific fuse bits.  See CPU_VERSION_xx macros for definitions.
 */
typedef struct ml_topology_info {
	unsigned int                    version;
	unsigned int                    num_cpus;
	unsigned int                    max_cpu_id;
	unsigned int                    num_clusters;
	unsigned int                    max_cluster_id;
	unsigned int                    max_die_id;
	ml_topology_cpu_t               *cpus;
	ml_topology_cluster_t           *clusters;
	ml_topology_cpu_t               *boot_cpu;
	ml_topology_cluster_t           *boot_cluster;
	unsigned int                    chip_revision;
} ml_topology_info_t;

/*!
 * @function ml_get_topology_info
 * @result A pointer to the read-only topology struct.  Does not need to be freed.  Returns NULL
 *         if the struct hasn't been initialized or the feature is unsupported.
 */
const ml_topology_info_t *ml_get_topology_info(void);

/*!
 * @function ml_map_cpu_pio
 * @brief Maps per-CPU and per-cluster PIO registers found in EDT.  This needs to be
 *        called after arm_vm_init() so it can't be part of ml_parse_cpu_topology().
 */
void ml_map_cpu_pio(void);

/* Struct for ml_processor_register */
struct ml_processor_info {
	cpu_id_t                        cpu_id;
	vm_offset_t                     start_paddr;
	boolean_t                       supports_nap;
	void                            *platform_cache_dispatch;
	time_base_enable_t              time_base_enable;
	processor_idle_t                processor_idle;
	idle_tickle_t                   *idle_tickle;
	idle_timer_t                    idle_timer;
	void                            *idle_timer_refcon;
	vm_offset_t                     powergate_stub_addr;
	uint32_t                        powergate_stub_length;
	uint32_t                        powergate_latency;
	platform_error_handler_t        platform_error_handler;
	uint64_t                        regmap_paddr;
	uint32_t                        phys_id;
	uint32_t                        log_id;
	uint32_t                        l2_access_penalty;
	uint32_t                        cluster_id;
	cluster_type_t                  cluster_type;
	uint32_t                        l2_cache_id;
	uint32_t                        l2_cache_size;
	uint32_t                        l3_cache_id;
	uint32_t                        l3_cache_size;
};
typedef struct ml_processor_info ml_processor_info_t;


/*!
 * @function ml_processor_register
 *
 * @abstract callback from platform kext to register processor
 *
 * @discussion This function is called by the platform kext when a processor is
 * being registered.  This is called while running on the CPU itself, as part of
 * its initialization.
 *
 * @param ml_processor_info provides machine-specific information about the
 * processor to xnu.
 *
 * @param processor is set as an out-parameter to an opaque handle that should
 * be used by the platform kext when referring to this processor in the future.
 *
 * @param ipi_handler is set as an out-parameter to the function that should be
 * registered as the IPI handler.
 *
 * @param pmi_handler is set as an out-parameter to the function that should be
 * registered as the PMI handler.
 *
 * @returns KERN_SUCCESS on success and an error code, otherwise.
 */
kern_return_t ml_processor_register(ml_processor_info_t *ml_processor_info,
    processor_t *processor, ipi_handler_t *ipi_handler,
    perfmon_interrupt_handler_func *pmi_handler);

/* Register a lockdown handler */
kern_return_t ml_lockdown_handler_register(lockdown_handler_t, void *);


/* Initialize Interrupts */
void ml_install_interrupt_handler(
	void *nub,
	int source,
	void *target,
	IOInterruptHandler handler,
	void *refCon);

vm_offset_t
    ml_static_vtop(
	vm_offset_t);

kern_return_t
ml_static_verify_page_protections(
	uint64_t base, uint64_t size, vm_prot_t prot);

vm_offset_t
    ml_static_ptovirt(
	vm_offset_t);

vm_offset_t ml_static_slide(
	vm_offset_t vaddr);

vm_offset_t ml_static_unslide(
	vm_offset_t vaddr);

/* Offset required to obtain absolute time value from tick counter */
uint64_t ml_get_abstime_offset(void);

/* Offset required to obtain continuous time value from tick counter */
uint64_t ml_get_conttime_offset(void);

#ifdef __APPLE_API_UNSTABLE
/* PCI config cycle probing */
boolean_t ml_probe_read(
	vm_offset_t paddr,
	unsigned int *val);
boolean_t ml_probe_read_64(
	addr64_t paddr,
	unsigned int *val);

/* Read physical address byte */
unsigned int ml_phys_read_byte(
	vm_offset_t paddr);
unsigned int ml_phys_read_byte_64(
	addr64_t paddr);

/* Read physical address half word */
unsigned int ml_phys_read_half(
	vm_offset_t paddr);
unsigned int ml_phys_read_half_64(
	addr64_t paddr);

/* Read physical address word*/
unsigned int ml_phys_read(
	vm_offset_t paddr);
unsigned int ml_phys_read_64(
	addr64_t paddr);
unsigned int ml_phys_read_word(
	vm_offset_t paddr);
unsigned int ml_phys_read_word_64(
	addr64_t paddr);

unsigned long long ml_io_read(uintptr_t iovaddr, int iovsz);
unsigned int ml_io_read8(uintptr_t iovaddr);
unsigned int ml_io_read16(uintptr_t iovaddr);
unsigned int ml_io_read32(uintptr_t iovaddr);
unsigned long long ml_io_read64(uintptr_t iovaddr);

extern void ml_io_write(uintptr_t vaddr, uint64_t val, int size);
extern void ml_io_write8(uintptr_t vaddr, uint8_t val);
extern void ml_io_write16(uintptr_t vaddr, uint16_t val);
extern void ml_io_write32(uintptr_t vaddr, uint32_t val);
extern void ml_io_write64(uintptr_t vaddr, uint64_t val);

/* Read physical address double word */
unsigned long long ml_phys_read_double(
	vm_offset_t paddr);
unsigned long long ml_phys_read_double_64(
	addr64_t paddr);

/* Write physical address byte */
void ml_phys_write_byte(
	vm_offset_t paddr, unsigned int data);
void ml_phys_write_byte_64(
	addr64_t paddr, unsigned int data);

/* Write physical address half word */
void ml_phys_write_half(
	vm_offset_t paddr, unsigned int data);
void ml_phys_write_half_64(
	addr64_t paddr, unsigned int data);

/* Write physical address word */
void ml_phys_write(
	vm_offset_t paddr, unsigned int data);
void ml_phys_write_64(
	addr64_t paddr, unsigned int data);
void ml_phys_write_word(
	vm_offset_t paddr, unsigned int data);
void ml_phys_write_word_64(
	addr64_t paddr, unsigned int data);

/* Write physical address double word */
void ml_phys_write_double(
	vm_offset_t paddr, unsigned long long data);
void ml_phys_write_double_64(
	addr64_t paddr, unsigned long long data);

void ml_static_mfree(
	vm_offset_t,
	vm_size_t);

kern_return_t
ml_static_protect(
	vm_offset_t start,
	vm_size_t size,
	vm_prot_t new_prot);

/* virtual to physical on wired pages */
vm_offset_t ml_vtophys(
	vm_offset_t vaddr);

/* Get processor cache info */
void ml_cpu_get_info(ml_cpu_info_t *ml_cpu_info);

#endif /* __APPLE_API_UNSTABLE */

#ifdef __APPLE_API_PRIVATE

/* Zero bytes starting at a physical address */
void bzero_phys(
	addr64_t phys_address,
	vm_size_t length);

void bzero_phys_nc(addr64_t src64, vm_size_t bytes);


void ml_thread_policy(
	thread_t thread,
	unsigned policy_id,
	unsigned policy_info);

#define MACHINE_GROUP                                   0x00000001
#define MACHINE_NETWORK_GROUP                   0x10000000
#define MACHINE_NETWORK_WORKLOOP                0x00000001
#define MACHINE_NETWORK_NETISR                  0x00000002

/* Set the maximum number of CPUs */
void ml_set_max_cpus(
	unsigned int max_cpus);

/* Return the maximum number of CPUs set by ml_set_max_cpus(), waiting if necessary */
unsigned int ml_wait_max_cpus(
	void);

/* Return the maximum memory size */
unsigned int ml_get_machine_mem(void);


extern void     ml_cpu_up(void);
extern void     ml_cpu_down(void);
extern void     ml_arm_sleep(void);

extern uint64_t ml_get_wake_timebase(void);
extern uint64_t ml_get_conttime_wake_time(void);

/* Time since the system was reset (as part of boot/wake) */
uint64_t ml_get_time_since_reset(void);

/*
 * Called by ApplePMGR to set wake time.  Units and epoch are identical
 * to mach_continuous_time().  Has no effect on !HAS_CONTINUOUS_HWCLOCK
 * chips.  If wake_time == UINT64_MAX, that means the wake time is
 * unknown and calls to ml_get_time_since_reset() will return UINT64_MAX.
 */
void ml_set_reset_time(uint64_t wake_time);


/* Bytes available on current stack */
vm_offset_t ml_stack_remaining(void);


extern  uint32_t        arm_debug_read_dscr(void);

extern int      set_be_bit(void);
extern int      clr_be_bit(void);
extern int      be_tracing(void);

/* Please note that cpu_broadcast_xcall is not as simple is you would like it to be.
 * It will sometimes put the calling thread to sleep, and it is up to your callback
 * to wake it up as needed, where "as needed" is defined as "all other CPUs have
 * called the broadcast func". Look around the kernel for examples, or instead use
 * cpu_broadcast_xcall_simple() which does indeed act like you would expect, given
 * the prototype. cpu_broadcast_immediate_xcall has the same caveats and has a similar
 * _simple() wrapper
 */
typedef void (*broadcastFunc) (void *);
unsigned int cpu_broadcast_xcall(uint32_t *, boolean_t, broadcastFunc, void *);
unsigned int cpu_broadcast_xcall_simple(boolean_t, broadcastFunc, void *);
kern_return_t cpu_xcall(int, broadcastFunc, void *);
unsigned int cpu_broadcast_immediate_xcall(uint32_t *, boolean_t, broadcastFunc, void *);
unsigned int cpu_broadcast_immediate_xcall_simple(boolean_t, broadcastFunc, void *);
kern_return_t cpu_immediate_xcall(int, broadcastFunc, void *);


/* Interface to be used by the perf. controller to register a callback, in a
 * single-threaded fashion. The callback will receive notifications of
 * processor performance quality-of-service changes from the scheduler.
 */

#ifdef __arm64__
typedef void (*cpu_qos_update_t)(int throughput_qos, uint64_t qos_param1, uint64_t qos_param2);
void cpu_qos_update_register(cpu_qos_update_t);
#endif /* __arm64__ */

struct going_on_core {
	uint64_t        thread_id;
	uint16_t        qos_class;
	uint16_t        urgency;        /* XCPM compatibility */
	uint32_t        is_32_bit : 1; /* uses 32-bit ISA/register state in userspace (which may differ from address space size) */
	uint32_t        is_kernel_thread : 1;
	uint64_t        thread_group_id;
	void            *thread_group_data;
	uint64_t        scheduling_latency;     /* absolute time between when thread was made runnable and this ctx switch */
	uint64_t        start_time;
	uint64_t        scheduling_latency_at_same_basepri;
	uint32_t        energy_estimate_nj;     /* return: In nanojoules */
	/* smaller of the time between last change to base priority and ctx switch and scheduling_latency */
};
typedef struct going_on_core *going_on_core_t;

struct going_off_core {
	uint64_t        thread_id;
	uint32_t        energy_estimate_nj;     /* return: In nanojoules */
	uint32_t        reserved;
	uint64_t        end_time;
	uint64_t        thread_group_id;
	void            *thread_group_data;
};
typedef struct going_off_core *going_off_core_t;

struct thread_group_data {
	uint64_t        thread_group_id;
	void            *thread_group_data;
	uint32_t        thread_group_size;
	uint32_t        thread_group_flags;
};
typedef struct thread_group_data *thread_group_data_t;

struct perfcontrol_max_runnable_latency {
	uint64_t        max_scheduling_latencies[4 /* THREAD_URGENCY_MAX */];
};
typedef struct perfcontrol_max_runnable_latency *perfcontrol_max_runnable_latency_t;

struct perfcontrol_work_interval {
	uint64_t        thread_id;
	uint16_t        qos_class;
	uint16_t        urgency;
	uint32_t        flags; // notify
	uint64_t        work_interval_id;
	uint64_t        start;
	uint64_t        finish;
	uint64_t        deadline;
	uint64_t        next_start;
	uint64_t        thread_group_id;
	void            *thread_group_data;
	uint32_t        create_flags;
};
typedef struct perfcontrol_work_interval *perfcontrol_work_interval_t;

typedef enum {
	WORK_INTERVAL_START,
	WORK_INTERVAL_UPDATE,
	WORK_INTERVAL_FINISH
} work_interval_ctl_t;

struct perfcontrol_work_interval_instance {
	work_interval_ctl_t     ctl;
	uint32_t                create_flags;
	uint64_t                complexity;
	uint64_t                thread_id;
	uint64_t                work_interval_id;
	uint64_t                instance_id; /* out: start, in: update/finish */
	uint64_t                start;
	uint64_t                finish;
	uint64_t                deadline;
	uint64_t                thread_group_id;
	void                    *thread_group_data;
};
typedef struct perfcontrol_work_interval_instance *perfcontrol_work_interval_instance_t;

/*
 * Structure to export per-CPU counters as part of the CLPC callout.
 * Contains only the fixed CPU counters (instructions and cycles); CLPC
 * would call back into XNU to get the configurable counters if needed.
 */
struct perfcontrol_cpu_counters {
	uint64_t        instructions;
	uint64_t        cycles;
};

/*
 * Structure used to pass information about a thread to CLPC
 */
struct perfcontrol_thread_data {
	/*
	 * Energy estimate (return value)
	 * The field is populated by CLPC and used to update the
	 * energy estimate of the thread
	 */
	uint32_t            energy_estimate_nj;
	/* Perfcontrol class for thread */
	perfcontrol_class_t perfctl_class;
	/* Thread ID for the thread */
	uint64_t            thread_id;
	/* Thread Group ID */
	uint64_t            thread_group_id;
	/*
	 * Scheduling latency for threads at the same base priority.
	 * Calculated by the scheduler and passed into CLPC. The field is
	 * populated only in the thread_data structure for the thread
	 * going on-core.
	 */
	uint64_t            scheduling_latency_at_same_basepri;
	/* Thread Group data pointer */
	void                *thread_group_data;
	/* perfctl state pointer */
	void                *perfctl_state;
};

/*
 * All callouts from the scheduler are executed with interrupts
 * disabled. Callouts should be implemented in C with minimal
 * abstractions, and only use KPI exported by the mach/libkern
 * symbolset, restricted to routines like spinlocks and atomic
 * operations and scheduler routines as noted below. Spinlocks that
 * are used to synchronize data in the perfcontrol_state_t should only
 * ever be acquired with interrupts disabled, to avoid deadlocks where
 * an quantum expiration timer interrupt attempts to perform a callout
 * that attempts to lock a spinlock that is already held.
 */

/*
 * When a processor is switching between two threads (after the
 * scheduler has chosen a new thread), the low-level platform layer
 * will call this routine, which should perform required timestamps,
 * MMIO register reads, or other state switching. No scheduler locks
 * are held during this callout.
 *
 * This function is called with interrupts ENABLED.
 */
typedef void (*sched_perfcontrol_context_switch_t)(perfcontrol_state_t, perfcontrol_state_t);

/*
 * Once the processor has switched to the new thread, the offcore
 * callout will indicate the old thread that is no longer being
 * run. The thread's scheduler lock is held, so it will not begin
 * running on another processor (in the case of preemption where it
 * remains runnable) until it completes. If the "thread_terminating"
 * boolean is TRUE, this will be the last callout for this thread_id.
 */
typedef void (*sched_perfcontrol_offcore_t)(perfcontrol_state_t, going_off_core_t /* populated by callee */, boolean_t);

/*
 * After the offcore callout and after the old thread can potentially
 * start running on another processor, the oncore callout will be
 * called with the thread's scheduler lock held. The oncore callout is
 * also called any time one of the parameters in the going_on_core_t
 * structure changes, like priority/QoS changes, and quantum
 * expiration, so the callout must not assume callouts are paired with
 * offcore callouts.
 */
typedef void (*sched_perfcontrol_oncore_t)(perfcontrol_state_t, going_on_core_t);

/*
 * Periodically (on hundreds of ms scale), the scheduler will perform
 * maintenance and report the maximum latency for runnable (but not currently
 * running) threads for each urgency class.
 */
typedef void (*sched_perfcontrol_max_runnable_latency_t)(perfcontrol_max_runnable_latency_t);

/*
 * When the kernel receives information about work intervals from userland,
 * it is passed along using this callback. No locks are held, although the state
 * object will not go away during the callout.
 */
typedef void (*sched_perfcontrol_work_interval_notify_t)(perfcontrol_state_t, perfcontrol_work_interval_t);

/*
 * Start, update and finish work interval instance with optional complexity estimate.
 */
typedef void (*sched_perfcontrol_work_interval_ctl_t)(perfcontrol_state_t, perfcontrol_work_interval_instance_t);

/*
 * These callbacks are used when thread groups are added, removed or properties
 * updated.
 * No blocking allocations (or anything else blocking) are allowed inside these
 * callbacks. No locks allowed in these callbacks as well since the kernel might
 * be holding the thread/task locks.
 */
typedef void (*sched_perfcontrol_thread_group_init_t)(thread_group_data_t);
typedef void (*sched_perfcontrol_thread_group_deinit_t)(thread_group_data_t);
typedef void (*sched_perfcontrol_thread_group_flags_update_t)(thread_group_data_t);

/*
 * Sometime after the timeout set by sched_perfcontrol_update_callback_deadline has passed,
 * this function will be called, passing the timeout deadline that was previously armed as an argument.
 *
 * This is called inside context-switch/quantum-interrupt context and must follow the safety rules for that context.
 */
typedef void (*sched_perfcontrol_deadline_passed_t)(uint64_t deadline);

/*
 * Context Switch Callout
 *
 * Parameters:
 * event        - The perfcontrol_event for this callout
 * cpu_id       - The CPU doing the context switch
 * timestamp    - The timestamp for the context switch
 * flags        - Flags for other relevant information
 * offcore      - perfcontrol_data structure for thread going off-core
 * oncore       - perfcontrol_data structure for thread going on-core
 * cpu_counters - perfcontrol_cpu_counters for the CPU doing the switch
 */
typedef void (*sched_perfcontrol_csw_t)(
	perfcontrol_event event, uint32_t cpu_id, uint64_t timestamp, uint32_t flags,
	struct perfcontrol_thread_data *offcore, struct perfcontrol_thread_data *oncore,
	struct perfcontrol_cpu_counters *cpu_counters, __unused void *unused);


/*
 * Thread State Update Callout
 *
 * Parameters:
 * event        - The perfcontrol_event for this callout
 * cpu_id       - The CPU doing the state update
 * timestamp    - The timestamp for the state update
 * flags        - Flags for other relevant information
 * thr_data     - perfcontrol_data structure for the thread being updated
 */
typedef void (*sched_perfcontrol_state_update_t)(
	perfcontrol_event event, uint32_t cpu_id, uint64_t timestamp, uint32_t flags,
	struct perfcontrol_thread_data *thr_data, __unused void *unused);

/*
 * Thread Group Blocking Relationship Callout
 *
 * Parameters:
 * blocked_tg           - Thread group blocking on progress of another thread group
 * blocking_tg          - Thread group blocking progress of another thread group
 * flags                - Flags for other relevant information
 * blocked_thr_state    - Per-thread perfcontrol state for blocked thread
 */
typedef void (*sched_perfcontrol_thread_group_blocked_t)(
	thread_group_data_t blocked_tg, thread_group_data_t blocking_tg, uint32_t flags, perfcontrol_state_t blocked_thr_state);

/*
 * Thread Group Unblocking Callout
 *
 * Parameters:
 * unblocked_tg         - Thread group being unblocked from making forward progress
 * unblocking_tg        - Thread group unblocking progress of another thread group
 * flags                - Flags for other relevant information
 * unblocked_thr_state  - Per-thread perfcontrol state for unblocked thread
 */
typedef void (*sched_perfcontrol_thread_group_unblocked_t)(
	thread_group_data_t unblocked_tg, thread_group_data_t unblocking_tg, uint32_t flags, perfcontrol_state_t unblocked_thr_state);

/*
 * Callers should always use the CURRENT version so that the kernel can detect both older
 * and newer structure layouts. New callbacks should always be added at the end of the
 * structure, and xnu should expect existing source recompiled against newer headers
 * to pass NULL for unimplemented callbacks. Pass NULL as the as the callbacks parameter
 * to reset callbacks to their default in-kernel values.
 */

#define SCHED_PERFCONTROL_CALLBACKS_VERSION_0 (0) /* up-to oncore */
#define SCHED_PERFCONTROL_CALLBACKS_VERSION_1 (1) /* up-to max_runnable_latency */
#define SCHED_PERFCONTROL_CALLBACKS_VERSION_2 (2) /* up-to work_interval_notify */
#define SCHED_PERFCONTROL_CALLBACKS_VERSION_3 (3) /* up-to thread_group_deinit */
#define SCHED_PERFCONTROL_CALLBACKS_VERSION_4 (4) /* up-to deadline_passed */
#define SCHED_PERFCONTROL_CALLBACKS_VERSION_5 (5) /* up-to state_update */
#define SCHED_PERFCONTROL_CALLBACKS_VERSION_6 (6) /* up-to thread_group_flags_update */
#define SCHED_PERFCONTROL_CALLBACKS_VERSION_7 (7) /* up-to work_interval_ctl */
#define SCHED_PERFCONTROL_CALLBACKS_VERSION_8 (8) /* up-to thread_group_unblocked */
#define SCHED_PERFCONTROL_CALLBACKS_VERSION_CURRENT SCHED_PERFCONTROL_CALLBACKS_VERSION_6

struct sched_perfcontrol_callbacks {
	unsigned long version; /* Use SCHED_PERFCONTROL_CALLBACKS_VERSION_CURRENT */
	sched_perfcontrol_offcore_t                   offcore;
	sched_perfcontrol_context_switch_t            context_switch;
	sched_perfcontrol_oncore_t                    oncore;
	sched_perfcontrol_max_runnable_latency_t      max_runnable_latency;
	sched_perfcontrol_work_interval_notify_t      work_interval_notify;
	sched_perfcontrol_thread_group_init_t         thread_group_init;
	sched_perfcontrol_thread_group_deinit_t       thread_group_deinit;
	sched_perfcontrol_deadline_passed_t           deadline_passed;
	sched_perfcontrol_csw_t                       csw;
	sched_perfcontrol_state_update_t              state_update;
	sched_perfcontrol_thread_group_flags_update_t thread_group_flags_update;
	sched_perfcontrol_work_interval_ctl_t         work_interval_ctl;
	sched_perfcontrol_thread_group_blocked_t      thread_group_blocked;
	sched_perfcontrol_thread_group_unblocked_t    thread_group_unblocked;
};
typedef struct sched_perfcontrol_callbacks *sched_perfcontrol_callbacks_t;

extern void sched_perfcontrol_register_callbacks(sched_perfcontrol_callbacks_t callbacks, unsigned long size_of_state);

/*
 * Update the scheduler with the set of cores that should be used to dispatch new threads.
 * Non-recommended cores can still be used to field interrupts or run bound threads.
 * This should be called with interrupts enabled and no scheduler locks held.
 */
#define ALL_CORES_RECOMMENDED   (~(uint32_t)0)

extern void sched_perfcontrol_update_recommended_cores(uint32_t recommended_cores);
extern void sched_perfcontrol_thread_group_recommend(void *data, cluster_type_t recommendation);
extern void sched_override_recommended_cores_for_sleep(void);
extern void sched_restore_recommended_cores_after_sleep(void);
extern void sched_perfcontrol_inherit_recommendation_from_tg(perfcontrol_class_t perfctl_class, boolean_t inherit);

extern void sched_usercontrol_update_recommended_cores(uint64_t recommended_cores);

/*
 * Edge Scheduler-CLPC Interface
 *
 * sched_perfcontrol_thread_group_preferred_clusters_set()
 *
 * The Edge scheduler expects thread group recommendations to be specific clusters rather
 * than just E/P. In order to allow more fine grained control, CLPC can specify an override
 * preferred cluster per QoS bucket. CLPC passes a common preferred cluster `tg_preferred_cluster`
 * and an array of size [PERFCONTROL_CLASS_MAX] with overrides for specific perfctl classes.
 * The scheduler translates these preferences into sched_bucket
 * preferences and applies the changes.
 *
 */
/* Token to indicate a particular perfctl class is not overriden */
#define SCHED_PERFCONTROL_PREFERRED_CLUSTER_OVERRIDE_NONE         ((uint32_t)~0)

/*
 * CLPC can also indicate if there should be an immediate rebalancing of threads of this TG as
 * part of this preferred cluster change. It does that by specifying the following options.
 */
#define SCHED_PERFCONTROL_PREFERRED_CLUSTER_MIGRATE_RUNNING       0x1
#define SCHED_PERFCONTROL_PREFERRED_CLUSTER_MIGRATE_RUNNABLE      0x2
typedef uint64_t sched_perfcontrol_preferred_cluster_options_t;

extern void sched_perfcontrol_thread_group_preferred_clusters_set(void *machine_data, uint32_t tg_preferred_cluster,
    uint32_t overrides[PERFCONTROL_CLASS_MAX], sched_perfcontrol_preferred_cluster_options_t options);

/*
 * Edge Scheduler-CLPC Interface
 *
 * sched_perfcontrol_edge_matrix_get()/sched_perfcontrol_edge_matrix_set()
 *
 * The Edge scheduler uses edges between clusters to define the likelihood of migrating threads
 * across clusters. The edge config between any two clusters defines the edge weight and whether
 * migation and steal operations are allowed across that edge. The getter and setter allow CLPC
 * to query and configure edge properties between various clusters on the platform.
 */

extern void sched_perfcontrol_edge_matrix_get(sched_clutch_edge *edge_matrix, bool *edge_request_bitmap, uint64_t flags, uint64_t matrix_order);
extern void sched_perfcontrol_edge_matrix_set(sched_clutch_edge *edge_matrix, bool *edge_changes_bitmap, uint64_t flags, uint64_t matrix_order);

/*
 * Update the deadline after which sched_perfcontrol_deadline_passed will be called.
 * Returns TRUE if it successfully canceled a previously set callback,
 * and FALSE if it did not (i.e. one wasn't set, or callback already fired / is in flight).
 * The callback is automatically canceled when it fires, and does not repeat unless rearmed.
 *
 * This 'timer' executes as the scheduler switches between threads, on a non-idle core
 *
 * There can be only one outstanding timer globally.
 */
extern boolean_t sched_perfcontrol_update_callback_deadline(uint64_t deadline);

typedef enum perfcontrol_callout_type {
	PERFCONTROL_CALLOUT_ON_CORE,
	PERFCONTROL_CALLOUT_OFF_CORE,
	PERFCONTROL_CALLOUT_CONTEXT,
	PERFCONTROL_CALLOUT_STATE_UPDATE,
	/* Add other callout types here */
	PERFCONTROL_CALLOUT_MAX
} perfcontrol_callout_type_t;

typedef enum perfcontrol_callout_stat {
	PERFCONTROL_STAT_INSTRS,
	PERFCONTROL_STAT_CYCLES,
	/* Add other stat types here */
	PERFCONTROL_STAT_MAX
} perfcontrol_callout_stat_t;

uint64_t perfcontrol_callout_stat_avg(perfcontrol_callout_type_t type,
    perfcontrol_callout_stat_t stat);

#ifdef __arm64__
/* The performance controller may use this interface to recommend
 * that CPUs in the designated cluster employ WFE rather than WFI
 * within the idle loop, falling back to WFI after the specified
 * timeout. The updates are expected to be serialized by the caller,
 * the implementation is not required to perform internal synchronization.
 */
uint32_t ml_update_cluster_wfe_recommendation(uint32_t wfe_cluster_id, uint64_t wfe_timeout_abstime_interval, uint64_t wfe_hint_flags);
#endif /* __arm64__ */

#if defined(HAS_APPLE_PAC)
#define ONES(x) (BIT((x))-1)
#define PTR_MASK ONES(64-T1SZ_BOOT)
#define PAC_MASK ~PTR_MASK
#define SIGN(p) ((p) & BIT(55))
#define UNSIGN_PTR(p) \
	SIGN(p) ? ((p) | PAC_MASK) : ((p) & ~PAC_MASK)

uint64_t ml_default_rop_pid(void);
uint64_t ml_default_jop_pid(void);
void ml_task_set_rop_pid(task_t task, task_t parent_task, boolean_t inherit);
void ml_task_set_jop_pid(task_t task, task_t parent_task, boolean_t inherit);
void ml_task_set_jop_pid_from_shared_region(task_t task);
void ml_task_set_disable_user_jop(task_t task, uint8_t disable_user_jop);
void ml_thread_set_disable_user_jop(thread_t thread, uint8_t disable_user_jop);
void ml_thread_set_jop_pid(thread_t thread, task_t task);
void *ml_auth_ptr_unchecked(void *ptr, unsigned key, uint64_t modifier);

uint64_t ml_enable_user_jop_key(uint64_t user_jop_key);

/**
 * Restores the previous JOP key state after a previous ml_enable_user_jop_key()
 * call.
 *
 * @param user_jop_key		The userspace JOP key previously passed to
 *				ml_enable_user_jop_key()
 * @param saved_jop_state       The saved JOP state returned by
 *				ml_enable_user_jop_key()
 */
void ml_disable_user_jop_key(uint64_t user_jop_key, uint64_t saved_jop_state);
#endif /* defined(HAS_APPLE_PAC) */

void ml_enable_monitor(void);



boolean_t machine_timeout_suspended(void);
void ml_get_power_state(boolean_t *, boolean_t *);

uint32_t get_arm_cpu_version(void);
boolean_t user_cont_hwclock_allowed(void);
uint8_t user_timebase_type(void);
boolean_t ml_thread_is64bit(thread_t thread);

#ifdef __arm64__
bool ml_feature_supported(uint32_t feature_bit);
void ml_set_align_checking(void);
extern void wfe_timeout_configure(void);
extern void wfe_timeout_init(void);
#endif /* __arm64__ */

void ml_timer_evaluate(void);
boolean_t ml_timer_forced_evaluation(void);
uint64_t ml_energy_stat(thread_t);
void ml_gpu_stat_update(uint64_t);
uint64_t ml_gpu_stat(thread_t);
#endif /* __APPLE_API_PRIVATE */





void ml_hibernate_active_pre(void);
void ml_hibernate_active_post(void);

__END_DECLS

#endif /* _ARM_MACHINE_ROUTINES_H_ */
