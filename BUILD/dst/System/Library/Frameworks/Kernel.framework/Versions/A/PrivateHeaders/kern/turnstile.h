/*
 * Copyright (c) 2017 Apple Inc. All rights reserved.
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

#ifndef _TURNSTILE_H_
#define _TURNSTILE_H_

#include <mach/mach_types.h>
#include <mach/kern_return.h>
#include <sys/cdefs.h>

#define TURNSTILE_MAX_HOP_DEFAULT (10)
struct turnstile_stats {
	uint64_t ts_priority_propagation;
	uint64_t ts_no_inheritor;
	uint64_t ts_thread_runnable;
	uint64_t ts_no_priority_change_required;
	uint64_t ts_above_ui_pri_change;
	uint64_t ts_no_turnstile;
};

#include <kern/queue.h>
#include <sys/queue.h>
#include <kern/waitq.h>
#include <kern/priority_queue.h>
#include <os/refcnt.h>
#include <kern/assert.h>
#include <kern/kern_types.h>
#include <kern/mpsc_queue.h>
#include <kern/locks.h>

/*
 * turnstile_type_t : Indicates the type of primitive the turnstile is associated with
 *                    Please populate turnstile_promote_policy array if a new type is added here.
 */
typedef enum __attribute__((packed)) turnstile_type {
	TURNSTILE_NONE = 0,
	TURNSTILE_KERNEL_MUTEX = 1,
	TURNSTILE_ULOCK = 2,
	TURNSTILE_PTHREAD_MUTEX = 3,
	TURNSTILE_SYNC_IPC = 4,
	TURNSTILE_WORKLOOPS = 5,
	TURNSTILE_WORKQS = 6,
	TURNSTILE_KNOTE = 7,
	TURNSTILE_SLEEP_INHERITOR = 8,
	TURNSTILE_TOTAL_TYPES = 9,
} turnstile_type_t;

/*
 * For each type of turnstile, following are the type of
 * inheritors passed:
 *
 * TURNSTILE_KERNEL_MUTEX
 *    Interlock: kernel mutex interlock.
 *    Inheritor: threads.
 *    Lock order: turnstile lock, thread lock.
 *
 * TURNSTILE_ULOCK
 *    Interlock: ulocks interlock.
 *    Inheritor: threads.
 *    Lock order: turnstile lock, thread lock.
 *
 * TURNSTILE_PTHREAD_MUTEX
 *    Interlock: pthread mtx interlock.
 *    Inheritor: threads.
 *    Lock order: turnstile lock, thread lock.
 *
 * TURNSTILE_SYNC_IPC
 *    Interlock: port's mqueue lock
 *    Inheritor: turnstile (of port in which we are enqueued or WL turnstile.
 *    Lock order: Our turnstile, then turnstile of the port we are enqueued in.
 *                Port circularity will make sure there is never a cycle formation
 *                and lock order is maintained.
 *
 * TURNSTILE_WORKLOOPS
 *    Interlock:
 *    - kq req lock
 *    - wq lock when "filt_wlworkq_interlock_needed() is true"
 *    Inheritor: thread, turnstile (of workq)
 *    Lock order: turnstile lock, thread lock
 *                WL turnstile lock, Workq turnstile lock
 *
 * TURNSTILE_WORKQS
 *    Interlock: workqueue lock
 *    Inheritor: thread
 *    Lock order: turnstile lock, thread lock.
 *
 * TURNSTILE_KNOTE
 *    Interlock: the knote lock
 *    Inheritor: WL turnstile
 *
 * TURNSTILE_SLEEP_INHERITOR
 *    Interlock: turnstile_htable bucket spinlock.
 *    Inheritor: threads.
 *    Lock order: turnstile lock, thread lock.
 *
 */

typedef enum __attribute__((flag_enum)) turnstile_promote_policy {
	TURNSTILE_PROMOTE_NONE = 0,
	TURNSTILE_KERNEL_PROMOTE = 0x1,
	TURNSTILE_USER_PROMOTE = 0x2,
	TURNSTILE_USER_IPC_PROMOTE = 0x4,
} turnstile_promote_policy_t;

typedef enum __attribute__((flag_enum)) turnstile_hash_lock_policy {
	TURNSTILE_HASH_LOCK_POLICY_NONE = 0,
	TURNSTILE_IRQ_UNSAFE_HASH = 0x1,
	TURNSTILE_LOCKED_HASH = 0x2,
} turnstile_hash_lock_policy_t;

/*
 * Turnstile state flags
 *
 * The turnstile state flags represent the current ownership of a turnstile.
 * The supported flags are:
 * - TURNSTILE_STATE_THREAD	: Turnstile is attached to a thread
 * - TURNSTILE_STATE_FREELIST	: Turnstile is hanging off the freelist of another turnstile
 * - TURNSTILE_STATE_HASHTABLE	: Turnstile is in the global hash table as the turnstile for a primitive
 * - TURNSTILE_STATE_PROPRIETOR : Turnstile is attached to a proprietor
 *
 * The flag updates are done while holding the primitive interlock.
 * */

#define TURNSTILE_STATE_THREAD          0x1
#define TURNSTILE_STATE_FREELIST        0x2
#define TURNSTILE_STATE_HASHTABLE       0x4
#define TURNSTILE_STATE_PROPRIETOR      0x8

/* Helper macros to set/unset turnstile state flags */
#if DEVELOPMENT || DEBUG

#define turnstile_state_init(ts, state)         \
MACRO_BEGIN                                     \
	        ts->ts_state = state;           \
MACRO_END

#define turnstile_state_add(ts, state)          \
MACRO_BEGIN                                     \
	        assert((ts->ts_state & (state)) == 0);  \
	        ts->ts_state |= state;                  \
MACRO_END

#define turnstile_state_remove(ts, state)       \
MACRO_BEGIN                                     \
	        assert(ts->ts_state & (state));         \
	        ts->ts_state &= ~(state);               \
MACRO_END

#else  /* DEVELOPMENT || DEBUG */

#define turnstile_state_init(ts, state)         \
MACRO_BEGIN                                     \
	        (void)ts;                       \
MACRO_END

#define turnstile_state_add(ts, state)          \
MACRO_BEGIN                                     \
	        (void)ts;                       \
MACRO_END

#define turnstile_state_remove(ts, state)       \
MACRO_BEGIN                                     \
	        (void)ts;                       \
MACRO_END

#endif /* DEVELOPMENT || DEBUG */

struct knote;
struct turnstile;

/*
 * Turnstile update flags
 *
 * TURNSTILE_IMMEDIATE_UPDATE
 *    When passed to turnstile_update_inheritor
 *    update the inheritor of the turnstile in
 *    the same call.
 *
 * TURNSTILE_DELAYED_UPDATE
 *    When passed to turnstile_update_inheritor
 *    it stashed the inheritor on the thread and
 *    turnstile's inheritor is updated in
 *    assert wait.
 *
 * TURNSTILE_INHERITOR_THREAD
 *    The turnstile inheritor is of type thread.
 *
 * TURNSTILE_INHERITOR_TURNSTILE
 *    The turnstile inheritor is of type turnstile.
 *
 * TURNSTILE_INHERITOR_WORKQ
 *    The turnstile inheritor is of type workqueue
 *
 * TURNSTILE_INHERITOR_NEEDS_PRI_UPDATE
 *    The inheritor needs a chain priority update.
 *
 * TURNSTILE_NEEDS_PRI_UPDATE
 *    Current turnstile needs a chain priority update.
 *
 * Locking order for passing thread and turnstile as inheritor
 *
 * Thread as an inheritor:
 *    When thread is passed as an inheritor of a turnstile
 *    turnstile lock is taken and then thread lock.
 *
 * Turnstile as in inheritor:
 *    When turnstile (T1) is passed as an inheritor of
 *    a turnstile (T2), turnstile lock of T2 is taken
 *    and then turnstile lock of T1 is taken.
 *
 * Caution: While passing turnstile as an inheritor, its
 *    job of the adopter to make sure that there is no
 *    lock inversion.
 */
typedef enum __attribute__((flag_enum)) __attribute__((packed)) turnstile_update_flags {
	TURNSTILE_UPDATE_FLAGS_NONE = 0,
	TURNSTILE_IMMEDIATE_UPDATE = 0x1,
	TURNSTILE_DELAYED_UPDATE = 0x2,
	TURNSTILE_INHERITOR_THREAD = 0x4,
	TURNSTILE_INHERITOR_TURNSTILE = 0x8,
	TURNSTILE_INHERITOR_NEEDS_PRI_UPDATE = 0x10,
	TURNSTILE_NEEDS_PRI_UPDATE = 0x20,
	TURNSTILE_INHERITOR_WORKQ = 0x40,
	TURNSTILE_UPDATE_BOOST = 0x80,
} turnstile_update_flags_t;

#define TURNSTILE_NULL ((struct turnstile *)0)

typedef void * turnstile_inheritor_t;

#define TURNSTILE_INHERITOR_NULL NULL


/* Interface */

/*
 * Name: turnstile_hash_bucket_lock
 *
 * Description: locks the spinlock associated with proprietor's bucket.
 *              if proprietor is specified the index for the hash will be
 *              recomputed and returned in index_proprietor,
 *              otherwise the value save in index_proprietor is used as index.
 *
 * Args:
 *   Arg1: proprietor (key) for hashing
 *   Arg2: index for proprietor in the hash
 *   Arg3: turnstile type
 *
 * Returns: old value of irq if irq were disabled before acquiring the lock.
 */
unsigned
turnstile_hash_bucket_lock(uintptr_t proprietor, uint32_t *index_proprietor, turnstile_type_t type);

/*
 * Name: turnstile_hash_bucket_unlock
 *
 * Description: unlocks the spinlock associated with proprietor's bucket.
 *              if proprietor is specified the index for the hash will be
 *              recomputed and returned in index_proprietor,
 *              otherwise the value save in index_proprietor is used as index.
 *
 * Args:
 *   Arg1: proprietor (key) for hashing
 *   Arg2: index for proprietor in the hash
 *   Arg3: turnstile type
 *   Arg4: irq value returned by turnstile_hash_bucket_lock
 *
 */
void
turnstile_hash_bucket_unlock(uintptr_t proprietor, uint32_t *index_proprietor, turnstile_type_t type, unsigned s);

/*
 * Name: turnstile_prepare
 *
 * Description: Transfer current thread's turnstile to primitive or it's free turnstile list.
 *              Function is called holding the interlock (spinlock) of the primitive.
 *              The turnstile returned by this function is safe to use untill the thread calls turnstile_complete.
 *              When no turnstile is provided explicitly, the calling thread will not have a turnstile attached to
 *              it untill it calls turnstile_complete.
 *
 * Args:
 *   Arg1: proprietor
 *   Arg2: pointer in primitive struct to store turnstile
 *   Arg3: turnstile to use instead of taking it from thread.
 *   Arg4: type of primitive
 *
 * Returns:
 *   turnstile.
 */
struct turnstile *
turnstile_prepare(
	uintptr_t proprietor,
	struct turnstile **tstore,
	struct turnstile *turnstile,
	turnstile_type_t type);

/*
 * Name: turnstile_complete
 *
 * Description: Transfer the primitive's turnstile or from it's freelist to current thread.
 *              Function is called holding the interlock (spinlock) of the primitive.
 *              Current thread will have a turnstile attached to it after this call.
 *
 * Args:
 *   Arg1: proprietor
 *   Arg2: pointer in primitive struct to update turnstile
 *   Arg3: pointer to store the returned turnstile instead of attaching it to thread
 *   Arg4: type of primitive
 *
 * Returns:
 *   None.
 */
void
turnstile_complete(
	uintptr_t proprietor,
	struct turnstile **tstore,
	struct turnstile **turnstile,
	turnstile_type_t type);

/*
 * Name: turnstile_update_inheritor
 *
 * Description: Update the inheritor of the turnstile and boost the
 *              inheritor. It will take a thread reference on the inheritor.
 *              Called with the interlock of the primitive held.
 *
 * Args:
 *   Arg1: turnstile
 *   Arg2: inheritor
 *   Arg3: flags - TURNSTILE_DELAYED_UPDATE - update will happen later in assert_wait
 *
 * Returns:
 *   old inheritor reference is stashed on current thread's struct.
 */
void
turnstile_update_inheritor(
	struct turnstile *turnstile,
	turnstile_inheritor_t new_inheritor,
	turnstile_update_flags_t flags);

typedef enum turnstile_update_complete_flags {
	TURNSTILE_INTERLOCK_NOT_HELD = 0x1,
	TURNSTILE_INTERLOCK_HELD = 0x2,
} turnstile_update_complete_flags_t;

/*
 * Name: turnstile_update_inheritor_complete
 *
 * Description: Update turnstile inheritor's priority and propagate the
 *              priority if the inheritor is blocked on a turnstile.
 *              Consumes thread ref of old inheritor returned by
 *              turnstile_update_inheritor. Recursive priority update
 *              will only happen when called with interlock dropped.
 *
 * Args:
 *   Arg1: turnstile
 *   Arg2: interlock held
 *
 * Returns: None.
 */
void
turnstile_update_inheritor_complete(
	struct turnstile *turnstile,
	turnstile_update_complete_flags_t flags);


/*
 * Name: turnstile_kernel_update_inheritor_on_wake_locked
 *
 * Description: Set thread as the inheritor of the turnstile and
 *		boost the inheritor.
 * Args:
 *   Arg1: turnstile
 *   Arg2: new_inheritor
 *   Arg3: flags
 *
 * Called with turnstile locked
 */
void
turnstile_kernel_update_inheritor_on_wake_locked(
	struct turnstile *turnstile,
	turnstile_inheritor_t new_inheritor,
	turnstile_update_flags_t flags);

/*
 * Internal KPI for sleep_with_inheritor, wakeup_with_inheritor, change_sleep_inheritor
 * meant to allow specifing the turnstile type to use to have different policy
 * on how to push on the inheritor.
 *
 * Differently from the "standard" KPI in locks.h these are meant to be used only
 * if you know what you are doing with turnstile.
 */

extern wait_result_t
lck_mtx_sleep_with_inheritor_and_turnstile_type(lck_mtx_t *lock, lck_sleep_action_t lck_sleep_action, event_t event, thread_t inheritor, wait_interrupt_t interruptible, uint64_t deadline, turnstile_type_t type);

extern wait_result_t
lck_rw_sleep_with_inheritor_and_turnstile_type(lck_rw_t *lock, lck_sleep_action_t lck_sleep_action, event_t event, thread_t inheritor, wait_interrupt_t interruptible, uint64_t deadline, turnstile_type_t type);

extern kern_return_t
wakeup_with_inheritor_and_turnstile_type(event_t event, turnstile_type_t type, wait_result_t result, bool wake_one, lck_wake_action_t action, thread_t *thread_wokenup);

extern kern_return_t
change_sleep_inheritor_and_turnstile_type(event_t event, thread_t inheritor, turnstile_type_t type);


#endif /* _TURNSTILE_H_ */
