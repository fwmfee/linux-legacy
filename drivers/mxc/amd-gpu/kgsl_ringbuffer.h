/* Copyright (c) 2002,2007-2009, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Code Aurora nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef __GSL_RINGBUFFER_H
#define __GSL_RINGBUFFER_H

// CONFIGURATION ITEMS
//#define GSL_STATS_RINGBUFFER
#define GSL_RB_USE_MEM_RPTR
#define GSL_RB_USE_MEM_TIMESTAMP
#define GSL_RB_TIMESTAMP_INTERUPT // not qualcomm!
/* #define GSL_RB_USE_WPTR_POLLING */
/* #define GSL_DEVICE_SHADOW_MEMSTORE_TO_USER */

#include "kgsl_types.h"


//////////////////////////////////////////////////////////////////////////////
//  defines
//////////////////////////////////////////////////////////////////////////////

// ringbuffer sizes log2quadword
#define     GSL_RB_SIZE_8                        0
#define     GSL_RB_SIZE_16                       1
#define     GSL_RB_SIZE_32                       2
#define     GSL_RB_SIZE_64                       3
#define     GSL_RB_SIZE_128                      4
#define     GSL_RB_SIZE_256                      5
#define     GSL_RB_SIZE_512                      6
#define     GSL_RB_SIZE_1K                       7
#define     GSL_RB_SIZE_2K                       8
#define     GSL_RB_SIZE_4K                       9
#define     GSL_RB_SIZE_8K                      10
#define     GSL_RB_SIZE_16K                     11
#define     GSL_RB_SIZE_32K                     12
#define     GSL_RB_SIZE_64K                     13
#define     GSL_RB_SIZE_128K                    14
#define     GSL_RB_SIZE_256K                    15
#define     GSL_RB_SIZE_512K                    16
#define     GSL_RB_SIZE_1M                      17
#define     GSL_RB_SIZE_2M                      18
#define     GSL_RB_SIZE_4M                      19

// offsets into memptrs
#define GSL_RB_MEMPTRS_RPTR_OFFSET              (offsetof(struct kgsl_rbmemptrs, rptr))
#define GSL_RB_MEMPTRS_WPTRPOLL_OFFSET          (offsetof(struct kgsl_rbmemptrs, wptr_poll))

// dword base address of the GFX decode space
#define GSL_HAL_SUBBLOCK_OFFSET(reg)            ((unsigned int)((reg) - (0x2000)))

// CP timestamp register
#define REG_CP_TIMESTAMP                          REG_SCRATCH_REG0


//////////////////////////////////////////////////////////////////////////////
// types
//////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
// ----------------
// ringbuffer debug
// ----------------
struct kgsl_rb_debug {
    unsigned int        pm4_ucode_rel;
    unsigned int        pfp_ucode_rel;
    unsigned int        cp_rb_base;
    cp_rb_cntl_u        cp_rb_cntl;
    unsigned int        cp_rb_rptr_addr;
    unsigned int        cp_rb_rptr;
    unsigned int        cp_rb_wptr;
    unsigned int        cp_rb_wptr_base;
    scratch_umsk_u      scratch_umsk;
    unsigned int        scratch_addr;
    cp_me_cntl_u        cp_me_cntl;
    cp_me_status_u      cp_me_status;
    cp_debug_u          cp_debug;
    cp_stat_u           cp_stat;
    rbbm_status_u       rbbm_status;
    unsigned int        sop_timestamp;
    unsigned int        eop_timestamp;
};
#endif // _DEBUG

// -------------------
// ringbuffer watchdog
// -------------------
struct kgsl_rbwatchdog {
    unsigned int   flags;
    unsigned int  rptr_sample;
};

// ------------------
// memory ptr objects
// ------------------
#ifdef __GNUC__
#pragma pack(push, 1)
#else
#pragma pack(push)
#pragma pack(1)
#endif
struct kgsl_rbmemptrs {
    volatile int  rptr;
    int           wptr_poll;
};
#pragma pack(pop)

// -----
// stats
// -----
struct kgsl_rbstats {
    __s64  wraps;
    __s64  issues;
    __s64  wordstotal;
};


// -----------------
// ringbuffer object
// -----------------
struct kgsl_ringbuffer {

    struct kgsl_device      *device;
    unsigned int       flags;
    struct mutex	*mutex;
    struct kgsl_memdesc     buffer_desc;              // allocated memory descriptor
    struct kgsl_memdesc     memptrs_desc;

    struct kgsl_rbmemptrs   *memptrs;

    unsigned int      sizedwords;               // ring buffer size dwords
    unsigned int      blksizequadwords;

    unsigned int      wptr;                     // write pointer offset in dwords from baseaddr
    unsigned int      rptr;                     // read pointer  offset in dwords from baseaddr
    unsigned int   timestamp;


    struct kgsl_rbwatchdog  watchdog;

#ifdef GSL_STATS_RINGBUFFER
    struct kgsl_rbstats     stats;
#endif // GSL_STATS_RINGBUFFER
};


// ----------
// ring write
// ----------
#define GSL_RB_WRITE(ring, data) \
	do { \
		mb(); \
		writel(data, ring); \
		ring++; \
	} while (0)

//      *(unsigned int *)(ring)++ = (unsigned int)(data);

// ---------
// timestamp
// ---------
#ifdef GSL_DEVICE_SHADOW_MEMSTORE_TO_USER
#warning GSL_DEVICE_SHADOW_MEMSTORE_TO_USER
#define GSL_RB_USE_MEM_TIMESTAMP
#endif //GSL_DEVICE_SHADOW_MEMSTORE_TO_USER

#ifdef  GSL_RB_USE_MEM_TIMESTAMP
#warning GSL_RB_USE_MEM_TIMESTAMP
#define GSL_RB_MEMPTRS_SCRATCH_MASK         0x1     // enable timestamp (...scratch0) memory shadowing
#define GSL_RB_INIT_TIMESTAMP(rb)

#else
#define GSL_RB_MEMPTRS_SCRATCH_MASK         0x0     // disable
#define GSL_RB_INIT_TIMESTAMP(rb) \
           kgsl_yamato_regwrite((rb)->device, REG_CP_TIMESTAMP, 0);
//qcom: use yamato directly (why do they pass the id?)
#endif // GSL_RB_USE_MEMTIMESTAMP

// --------
// mem rptr
// --------
#ifdef  GSL_RB_USE_MEM_RPTR
#warning GSL_RB_USE_MEM_RPTR
#define GSL_RB_CNTL_NO_UPDATE               0x0     // enable
#define GSL_RB_GET_READPTR(rb, data) \
	do { \
		*(data) = (rb)->memptrs->rptr; \
	} while (0)
#else
#define GSL_RB_CNTL_NO_UPDATE               0x1     // disable
#define GSL_RB_GET_READPTR(rb, data) \
	do { \
		kgsl_yamato_regread((rb)->device, REG_CP_RB_RPTR,(data)); \
	} while (0)
#endif // GSL_RB_USE_MEMRPTR

// ------------
// wptr polling
// ------------
#ifdef  GSL_RB_USE_WPTR_POLLING
#warning GSL_RB_USE_WPTR_POLLING
#define GSL_RB_CNTL_POLL_EN                 0x1     // enable
#define GSL_RB_UPDATE_WPTR_POLLING(rb) \
	do { (rb)->memptrs->wptr_poll = (rb)->wptr; } while (0)
#else
#define GSL_RB_CNTL_POLL_EN                 0x0     // disable
#define GSL_RB_UPDATE_WPTR_POLLING(rb)
#endif  // GSL_RB_USE_WPTR_POLLING

// -----
// stats
// -----
#ifdef GSL_STATS_RINGBUFFER
#define GSL_RB_STATS(x) x
#else
#define GSL_RB_STATS(x)
#endif // GSL_STATS_RINGBUFFER


//////////////////////////////////////////////////////////////////////////////
//  prototypes
//////////////////////////////////////////////////////////////////////////////
int             kgsl_ringbuffer_init(struct kgsl_device *device);
int             kgsl_ringbuffer_close(struct kgsl_ringbuffer *rb);
int             kgsl_ringbuffer_start(struct kgsl_ringbuffer *rb);
int             kgsl_ringbuffer_stop(struct kgsl_ringbuffer *rb);
unsigned int	kgsl_ringbuffer_issuecmds(struct kgsl_device *device, int pmodeoff, unsigned int *cmdaddr, int sizedwords, unsigned int pid); //pid is not in qcom
int             kgsl_ringbuffer_issueibcmds(struct kgsl_device *device, int drawctxt_index, uint32_t ibaddr, int sizedwords, unsigned int *timestamp, unsigned int flags);
// qcom: missing int kgsl_ringbuffer_gettimestampshadow(struct kgsl_device *device, unsigned int *sopaddr, unsigned int *eopaddr);
void            kgsl_ringbuffer_watchdog(void);
//qcom: missing void kgsl_cp_intrcallback(struct kgsl_device *device);
int             kgsl_ringbuffer_querystats(struct kgsl_ringbuffer *rb, struct kgsl_rbstats *stats);
int             kgsl_ringbuffer_bist(struct kgsl_ringbuffer *rb);

#endif  // __GSL_RINGBUFFER_H
