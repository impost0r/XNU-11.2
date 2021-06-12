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
#ifndef _SYS_PERSONA_H_
#define _SYS_PERSONA_H_

#include <sys/param.h>

__enum_decl(persona_type_t, int, {
	PERSONA_INVALID      = 0,
	PERSONA_GUEST        = 1,
	PERSONA_MANAGED      = 2,
	PERSONA_PRIV         = 3,
	PERSONA_SYSTEM       = 4,
	PERSONA_DEFAULT      = 5,
	PERSONA_SYSTEM_PROXY = 6,
	PERSONA_SYS_EXT      = 7,
	PERSONA_ENTERPRISE   = 8,

	PERSONA_TYPE_MAX     = PERSONA_ENTERPRISE,
});

#define PERSONA_ID_NONE ((uid_t)-1)

struct kpersona_info {
	uint32_t persona_info_version;

	uid_t    persona_id; /* overlaps with UID */
	int      persona_type;
	gid_t    persona_gid;
	uint32_t persona_ngroups;
	gid_t    persona_groups[NGROUPS];
	uid_t    persona_gmuid;
	char     persona_name[MAXLOGNAME + 1];

	/* TODO: MAC policies?! */
};

#define PERSONA_INFO_V1       1
#define PERSONA_INFO_V1_SIZE  (sizeof(struct kpersona_info))


#define PERSONA_OP_ALLOC    1
#define PERSONA_OP_PALLOC   2
#define PERSONA_OP_DEALLOC  3
#define PERSONA_OP_GET      4
#define PERSONA_OP_INFO     5
#define PERSONA_OP_PIDINFO  6
#define PERSONA_OP_FIND     7
#define PERSONA_OP_GETPATH  8
#define PERSONA_OP_FIND_BY_TYPE  9

#define PERSONA_MGMT_ENTITLEMENT "com.apple.private.persona-mgmt"


/* XNU + kext private interface */
#include <sys/cdefs.h>
#include <sys/kauth.h>
#include <libkern/libkern.h>
#include <os/refcnt.h>

#ifdef PERSONA_DEBUG
#include <os/log.h>
#define persona_dbg(fmt, ...) \
	os_log(OS_LOG_DEFAULT, "[%4d] %s:  " fmt "\n", \
	       current_proc() ? current_proc()->p_pid : -1, \
	       __func__, ## __VA_ARGS__)
#else
#define persona_dbg(fmt, ...) do { } while (0)
#endif

/*
 * Persona
 */
/* kexts should only see an opaque persona structure */
struct persona;

__BEGIN_DECLS

#ifndef _KAUTH_CRED_T
#define _KAUTH_CRED_T
typedef struct ucred *kauth_cred_t;
#endif  /* !_KAUTH_CRED_T */

/* returns the persona ID for the given pesona structure */
uid_t persona_get_id(struct persona *persona);

/* returns the type of the persona (see enum above: PERSONA_GUEST, etc.) */
int persona_get_type(struct persona *persona);

/* returns ref on kauth_cred_t that must be dropped via kauth_cred_unref() */
kauth_cred_t persona_get_cred(struct persona *persona);

/* returns a reference that must be released with persona_put() */
struct persona *persona_lookup(uid_t id);

/*
 * Search for personas based on name or uid
 *
 * Parameters:
 *       name: Local login name of the persona.
 *             Set this to NULL to find personas by 'uid'.
 *
 *        uid: UID of the persona.
 *             Set this to -1 to find personas by 'name'
 *
 *    persona: output - array of persona pointers. Each non-NULL value
 *             must* be released with persona_put. This can be NULL.
 *
 *       plen: input - size of 'persona' buffer (in number of pointers)
 *             output - the total required size of the 'persona' buffer (could be larger than input value)
 *
 * Return:
 *           0: Success
 *        != 0: failure (BSD errno value ESRCH or EINVAL)
 */
int persona_find(const char *login, uid_t uid,
    struct persona **persona, size_t *plen);

/* returns a reference that must be released with persona_put() */
struct persona *persona_proc_get(pid_t pid);

/* returns a reference to the persona tied to the current thread (also uses adopted voucher) */
struct persona *current_persona_get(void);

/* get a reference to a persona structure */
struct persona *persona_get(struct persona *persona);

/* release a reference to a persona structure */
void persona_put(struct persona *persona);

/*
 * Search for personas of a given type, 'persona_type'.
 *
 * Parameters:
 *   persona_type: Type of persona (see enum)
 *
 *        persona: output - array of persona pointers. Each non-NULL value
 *        must* be released with persona_put. This can be NULL.
 *
 *           plen: input - size of 'persona' buffer (in number of pointers)
 *                 output - the total required size of the 'persona' buffer (could be larger than input value)
 *
 * Return:
 *           0: Success
 *        != 0: failure (BSD errno value ESRCH or EINVAL)
 */
int persona_find_by_type(persona_type_t persona_type, struct persona **persona,
    size_t *plen);

__END_DECLS


#endif /* _SYS_PERSONA_H_ */
