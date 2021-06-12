/*
 * Copyright (c) 2004 Apple Computer, Inc. All rights reserved.
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

#ifndef __IOKIT_IOHIBERNATEPRIVATE_H
#define __IOKIT_IOHIBERNATEPRIVATE_H

#if HIBERNATION

#if defined(__arm64__)

#define HIBERNATE_HAVE_MACHINE_HEADER 1

// enable the hibernation exception handler on DEBUG and DEVELOPMENT kernels
#define HIBERNATE_TRAP_HANDLER (DEBUG || DEVELOPMENT)

#endif /* defined(__arm64__) */

#endif /* HIBERNATION */

#ifndef __ASSEMBLER__

#include <stdint.h>
#include <sys/cdefs.h>

__BEGIN_DECLS


#define HIBERNATE_HMAC_SIZE 48 // SHA384 size in bytes

struct IOHibernateHibSegment {
	uint32_t    iBootMemoryRegion;
	uint32_t    physPage;
	uint32_t    pageCount;
	uint32_t    protection;
};
typedef struct IOHibernateHibSegment IOHibernateHibSegment;

#define NUM_HIBSEGINFO_SEGMENTS 10
struct IOHibernateHibSegInfo {
	struct IOHibernateHibSegment    segments[NUM_HIBSEGINFO_SEGMENTS];
	uint8_t                         hmac[HIBERNATE_HMAC_SIZE];
};
typedef struct IOHibernateHibSegInfo IOHibernateHibSegInfo;

struct IOPolledFileExtent {
	uint64_t    start;
	uint64_t    length;
};
typedef struct IOPolledFileExtent IOPolledFileExtent;

struct IOHibernateImageHeader {
	uint64_t    imageSize;
	uint64_t    image1Size;

	uint32_t    restore1CodePhysPage;
	uint32_t    reserved1;
	uint64_t    restore1CodeVirt;
	uint32_t    restore1PageCount;
	uint32_t    restore1CodeOffset;
	uint32_t    restore1StackOffset;

	uint32_t    pageCount;
	uint32_t    bitmapSize;

	uint32_t    restore1Sum;
	uint32_t    image1Sum;
	uint32_t    image2Sum;

	uint32_t    actualRestore1Sum;
	uint32_t    actualImage1Sum;
	uint32_t    actualImage2Sum;

	uint32_t    actualUncompressedPages;
	uint32_t    conflictCount;
	uint32_t    nextFree;

	uint32_t    signature;
	uint32_t    processorFlags;

	uint32_t    runtimePages;
	uint32_t    runtimePageCount;
	uint64_t    runtimeVirtualPages __attribute__ ((packed));

	uint32_t    performanceDataStart;
	uint32_t    performanceDataSize;

	uint64_t    encryptStart __attribute__ ((packed));
	uint64_t    machineSignature __attribute__ ((packed));

	uint32_t    previewSize;
	uint32_t    previewPageListSize;

	uint32_t    diag[4];

	uint32_t    handoffPages;
	uint32_t    handoffPageCount;

	uint32_t    systemTableOffset;

	uint32_t    debugFlags;
	uint32_t    options;
	uint64_t    sleepTime __attribute__ ((packed));
	uint32_t    compression;

	uint8_t     bridgeBootSessionUUID[16];

	uint64_t    lastHibAbsTime __attribute__ ((packed));
	union {
		uint64_t    lastHibContTime;
		uint64_t    hwClockOffset;
	} __attribute__ ((packed));
	uint64_t    kernVirtSlide __attribute__ ((packed));

	uint32_t    reserved[47];       // make sizeof == 512
	uint32_t    booterTime0;
	uint32_t    booterTime1;
	uint32_t    booterTime2;

	uint32_t    booterStart;
	uint32_t    smcStart;
	uint32_t    connectDisplayTime;
	uint32_t    splashTime;
	uint32_t    booterTime;
	uint32_t    trampolineTime;

	uint64_t    encryptEnd __attribute__ ((packed));
	uint64_t    deviceBase __attribute__ ((packed));
	uint32_t    deviceBlockSize;

#if defined(__arm64__)
	uint32_t    segmentsFileOffset;
	IOHibernateHibSegInfo hibSegInfo;
	uint32_t    imageHeaderHMACSize;
	uint8_t     imageHeaderHMAC[HIBERNATE_HMAC_SIZE];
	uint8_t     handoffHMAC[HIBERNATE_HMAC_SIZE];
	uint8_t     image1PagesHMAC[HIBERNATE_HMAC_SIZE];
	uint8_t     image2PagesHMAC[HIBERNATE_HMAC_SIZE];
#endif /* defined(__arm64__) */

	uint32_t            fileExtentMapSize;
	IOPolledFileExtent  fileExtentMap[2];
};
typedef struct IOHibernateImageHeader IOHibernateImageHeader;

enum{
	kIOHibernateDebugRestoreLogs = 0x00000001
};

// options & IOHibernateOptions property
enum{
	kIOHibernateOptionSSD           = 0x00000001,
	kIOHibernateOptionColor         = 0x00000002,
	kIOHibernateOptionProgress      = 0x00000004,
	kIOHibernateOptionDarkWake      = 0x00000008,
	kIOHibernateOptionHWEncrypt     = 0x00000010,
};

struct hibernate_bitmap_t {
	uint32_t    first_page;
	uint32_t    last_page;
	uint32_t    bitmapwords;
	uint32_t    bitmap[0];
};
typedef struct hibernate_bitmap_t hibernate_bitmap_t;

struct hibernate_page_list_t {
	uint32_t              list_size;
	uint32_t              page_count;
	uint32_t              bank_count;
	hibernate_bitmap_t    bank_bitmap[0];
};
typedef struct hibernate_page_list_t hibernate_page_list_t;

#if defined(_AES_H)

struct hibernate_cryptwakevars_t {
	uint8_t aes_iv[AES_BLOCK_SIZE];
};
typedef struct hibernate_cryptwakevars_t hibernate_cryptwakevars_t;

struct hibernate_cryptvars_t {
	uint8_t aes_iv[AES_BLOCK_SIZE];
	aes_ctx ctx;
};
typedef struct hibernate_cryptvars_t hibernate_cryptvars_t;

#endif /* defined(_AES_H) */

enum{
	kIOHibernateHandoffType                 = 0x686f0000,
	kIOHibernateHandoffTypeEnd              = kIOHibernateHandoffType + 0,
	kIOHibernateHandoffTypeGraphicsInfo     = kIOHibernateHandoffType + 1,
	kIOHibernateHandoffTypeCryptVars        = kIOHibernateHandoffType + 2,
	kIOHibernateHandoffTypeMemoryMap        = kIOHibernateHandoffType + 3,
	kIOHibernateHandoffTypeDeviceTree       = kIOHibernateHandoffType + 4,
	kIOHibernateHandoffTypeDeviceProperties = kIOHibernateHandoffType + 5,
	kIOHibernateHandoffTypeKeyStore         = kIOHibernateHandoffType + 6,
	kIOHibernateHandoffTypeVolumeCryptKey   = kIOHibernateHandoffType + 7,
};

struct IOHibernateHandoff {
	uint32_t type;
	uint32_t bytecount;
	uint8_t  data[];
};
typedef struct IOHibernateHandoff IOHibernateHandoff;

enum{
	kIOHibernateProgressCount         = 19,
	kIOHibernateProgressWidth         = 7,
	kIOHibernateProgressHeight        = 16,
	kIOHibernateProgressSpacing       = 3,
	kIOHibernateProgressOriginY       = 81,

	kIOHibernateProgressSaveUnderSize = 2 * 5 + 14 * 2,

	kIOHibernateProgressLightGray     = 230,
	kIOHibernateProgressMidGray       = 174,
	kIOHibernateProgressDarkGray      = 92
};

enum{
	kIOHibernatePostWriteSleep   = 0,
	kIOHibernatePostWriteWake    = 1,
	kIOHibernatePostWriteHalt    = 2,
	kIOHibernatePostWriteRestart = 3
};


struct hibernate_graphics_t {
	uint64_t physicalAddress; // Base address of video memory
	int32_t  gfxStatus;     // EFI config restore status
	uint32_t rowBytes;              // Number of bytes per pixel row
	uint32_t width;                 // Width
	uint32_t height;                // Height
	uint32_t depth;                 // Pixel Depth

	uint8_t progressSaveUnder[kIOHibernateProgressCount][kIOHibernateProgressSaveUnderSize];
};
typedef struct hibernate_graphics_t hibernate_graphics_t;

#define DECLARE_IOHIBERNATEPROGRESSALPHA                                \
static const uint8_t gIOHibernateProgressAlpha                  \
[kIOHibernateProgressHeight][kIOHibernateProgressWidth] =       \
{                                                               \
    { 0x00,0x63,0xd8,0xf0,0xd8,0x63,0x00 },                     \
    { 0x51,0xff,0xff,0xff,0xff,0xff,0x51 },                     \
    { 0xae,0xff,0xff,0xff,0xff,0xff,0xae },                     \
    { 0xc3,0xff,0xff,0xff,0xff,0xff,0xc3 },                     \
    { 0xc3,0xff,0xff,0xff,0xff,0xff,0xc3 },                     \
    { 0xc3,0xff,0xff,0xff,0xff,0xff,0xc3 },                     \
    { 0xc3,0xff,0xff,0xff,0xff,0xff,0xc3 },                     \
    { 0xc3,0xff,0xff,0xff,0xff,0xff,0xc3 },                     \
    { 0xc3,0xff,0xff,0xff,0xff,0xff,0xc3 },                     \
    { 0xc3,0xff,0xff,0xff,0xff,0xff,0xc3 },                     \
    { 0xc3,0xff,0xff,0xff,0xff,0xff,0xc3 },                     \
    { 0xc3,0xff,0xff,0xff,0xff,0xff,0xc3 },                     \
    { 0xc3,0xff,0xff,0xff,0xff,0xff,0xc3 },                     \
    { 0xae,0xff,0xff,0xff,0xff,0xff,0xae },                     \
    { 0x54,0xff,0xff,0xff,0xff,0xff,0x54 },                     \
    { 0x00,0x66,0xdb,0xf3,0xdb,0x66,0x00 }                      \
};

struct hibernate_preview_t {
	uint32_t  imageCount;   // Number of images
	uint32_t  width;        // Width
	uint32_t  height;       // Height
	uint32_t  depth;        // Pixel Depth
	uint64_t  lockTime;     // Lock time
	uint32_t  reservedG[7]; // reserved
	uint32_t  reservedK[8]; // reserved
};
typedef struct hibernate_preview_t hibernate_preview_t;

struct hibernate_statistics_t {
	uint64_t image1Size;
	uint64_t imageSize;
	uint32_t image1Pages;
	uint32_t imagePages;
	uint32_t booterStart;
	uint32_t smcStart;
	uint32_t booterDuration;
	uint32_t booterConnectDisplayDuration;
	uint32_t booterSplashDuration;
	uint32_t booterDuration0;
	uint32_t booterDuration1;
	uint32_t booterDuration2;
	uint32_t trampolineDuration;
	uint32_t kernelImageReadDuration;

	uint32_t graphicsReadyTime;
	uint32_t wakeNotificationTime;
	uint32_t lockScreenReadyTime;
	uint32_t hidReadyTime;

	uint32_t wakeCapability;
	uint32_t hibCount;
	uint32_t resvA[14];
};
typedef struct hibernate_statistics_t hibernate_statistics_t;

#define kIOSysctlHibernateStatistics    "kern.hibernatestatistics"
#define kIOSysctlHibernateGraphicsReady "kern.hibernategraphicsready"
#define kIOSysctlHibernateWakeNotify    "kern.hibernatewakenotification"
#define kIOSysctlHibernateScreenReady   "kern.hibernatelockscreenready"
#define kIOSysctlHibernateHIDReady      "kern.hibernatehidready"
#define kIOSysctlHibernateCount         "kern.hibernatecount"
#define kIOSysctlHibernateSetPreview    "kern.hibernatepreview"

#define kIOHibernateSetPreviewEntitlementKey "com.apple.private.hibernation.set-preview"


// gIOHibernateState, kIOHibernateStateKey
enum{
	kIOHibernateStateInactive            = 0,
	kIOHibernateStateHibernating         = 1,/* writing image */
	kIOHibernateStateWakingFromHibernate = 2 /* booted and restored image */
};

// gIOHibernateMode, kIOHibernateModeKey
enum{
	kIOHibernateModeOn      = 0x00000001,
	kIOHibernateModeSleep   = 0x00000002,
	kIOHibernateModeEncrypt = 0x00000004,
	kIOHibernateModeDiscardCleanInactive = 0x00000008,
	kIOHibernateModeDiscardCleanActive   = 0x00000010,
	kIOHibernateModeSwitch      = 0x00000020,
	kIOHibernateModeRestart     = 0x00000040,
	kIOHibernateModeSSDInvert   = 0x00000080,
	kIOHibernateModeFileResize  = 0x00000100,
};

// IOHibernateImageHeader.signature
enum{
	kIOHibernateHeaderSignature        = 0x73696d65,
	kIOHibernateHeaderInvalidSignature = 0x7a7a7a7a,
	kIOHibernateHeaderOpenSignature    = 0xf1e0be9d,
	kIOHibernateHeaderDebugDataSignature = 0xfcddfcdd
};

// kind for hibernate_set_page_state()
enum{
	kIOHibernatePageStateFree        = 0,
	kIOHibernatePageStateWiredSave   = 1,
	kIOHibernatePageStateUnwiredSave = 2
};

#define kIOHibernateModeKey             "Hibernate Mode"
#define kIOHibernateFileKey             "Hibernate File"
#define kIOHibernateFileMinSizeKey      "Hibernate File Min"
#define kIOHibernateFileMaxSizeKey      "Hibernate File Max"
#define kIOHibernateFreeRatioKey        "Hibernate Free Ratio"
#define kIOHibernateFreeTimeKey         "Hibernate Free Time"

#define kIOHibernateStateKey            "IOHibernateState"
#define kIOHibernateFeatureKey          "Hibernation"
#define kIOHibernatePreviewBufferKey    "IOPreviewBuffer"

#ifndef kIOHibernatePreviewActiveKey
#define kIOHibernatePreviewActiveKey    "IOHibernatePreviewActive"
// values for kIOHibernatePreviewActiveKey
enum {
	kIOHibernatePreviewActive  = 0x00000001,
	kIOHibernatePreviewUpdates = 0x00000002
};
#endif

#define kIOHibernateOptionsKey      "IOHibernateOptions"
#define kIOHibernateGfxStatusKey    "IOHibernateGfxStatus"
enum {
	kIOHibernateGfxStatusUnknown = ((int32_t) 0xFFFFFFFF)
};

#define kIOHibernateBootImageKey        "boot-image"
#define kIOHibernateBootImageKeyKey     "boot-image-key"
#define kIOHibernateBootSignatureKey    "boot-signature"

#define kIOHibernateMemorySignatureKey    "memory-signature"
#define kIOHibernateMemorySignatureEnvKey "mem-sig"
#define kIOHibernateMachineSignatureKey   "machine-signature"

#define kIOHibernateRTCVariablesKey     "IOHibernateRTCVariables"
#define kIOHibernateSMCVariablesKey     "IOHibernateSMCVariables"

#define kIOHibernateBootSwitchVarsKey   "boot-switch-vars"

#define kIOHibernateBootNoteKey         "boot-note"


#define kIOHibernateUseKernelInterpreter    0x80000000

enum{
	kIOPreviewImageIndexDesktop = 0,
	kIOPreviewImageIndexLockScreen = 1,
	kIOPreviewImageCount = 2
};

enum{
	kIOScreenLockNoLock          = 1,
	kIOScreenLockUnlocked        = 2,
	kIOScreenLockLocked          = 3,
	kIOScreenLockFileVaultDialog = 4,
};

#define kIOScreenLockStateKey       "IOScreenLockState"
#define kIOBooterScreenLockStateKey "IOBooterScreenLockState"

__END_DECLS

#endif /* !__ASSEMBLER__ */

#endif /* ! __IOKIT_IOHIBERNATEPRIVATE_H */
