/*
 * Copyright 2019 University of Washington, Max Planck Institute for
 * Software Systems, and The University of Texas at Austin
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef TAS_FPIF_H_
#define TAS_FPIF_H_

#include <stdint.h>
#include <tas_utils.h>
#include <tas_packet_defs.h>


#define TAS_FP_HUGE_PREFIX "/dev/hugepages"

/** Name for the info shared memory region. */
#define TAS_FP_NAME_INFO "tas_info"
/** Name for tas buffer shared memory region. */
#define TAS_FP_NAME_BUFMEM "tas_memory"
/** Name for tas internal fp state shared memory region. */
#define TAS_FP_NAME_STATEMEM "tas_internal"

/** Size of the info shared memory region. */
#define TAS_FP_INFO_BYTES 0x1000

/** Indicates that tas is done initializing. */
#define TAS_FP_INFO_FLAG_READY 1
/** Indicates that huge pages should be used for the state and buffer memory */
#define TAS_FP_INFO_FLAG_HUGEPAGES 2

/** Info struct: layout of info shared memory region */
struct tas_fp_info {
  /** Flags: see TAS_FP_INFO_FLAG_* */
  uint64_t flags;
  /** Size of tas buffer memory in bytes. */
  uint64_t buf_mem_size;
  /** Size of fp state memory in bytes. */
  uint64_t state_mem_size;
  /** export mac address */
  uint64_t mac_address;
  /** Number of queues in queue manager */
  uint32_t qmq_num;
  /** Maximum number of cores used for tas fastpath emulator */
  uint32_t cores_num;
} __attribute__((packed));



/******************************************************************************/
/* Kernel RX queue */

#define TAS_FP_SPRX_INVALID 0x0
#define TAS_FP_SPRX_PACKET 0x1

/** Kernel RX queue entry */
struct tas_fp_sprx {
  uint64_t addr;
  union {
    struct {
      uint16_t len;
      uint16_t fn_core;
      uint16_t flow_group;
    } packet;
    uint8_t raw[55];
  } __attribute__((packed)) msg;
  volatile uint8_t type;
} __attribute__((packed));

STATIC_ASSERT(sizeof(struct tas_fp_sprx) == 64, krx_size);


/******************************************************************************/
/* Kernel TX queue */

#define TAS_FP_SPTX_INVALID 0x0
#define TAS_FP_SPTX_PACKET 0x1
#define TAS_FP_SPTX_CONNRETRAN 0x2
#define TAS_FP_SPTX_PACKET_NOTS 0x3

/** Kernel TX queue entry */
struct tas_fp_sptx {
  union {
    struct {
      uint64_t addr;
      uint16_t len;
    } packet;
    struct {
      uint32_t flow_id;
    } connretran;
    uint8_t raw[63];
  } __attribute__((packed)) msg;
  volatile uint8_t type;
} __attribute__((packed));

STATIC_ASSERT(sizeof(struct tas_fp_sptx) == 64, ktx_size);


/******************************************************************************/
/* App RX queue */

#define TAS_FP_ARX_INVALID    0x0
#define TAS_FP_ARX_CONNUPDATE 0x1

#define TAS_FP_ARX_FLRXDONE  0x1

/** Update receive and transmit buffer of flow */
struct tas_fp_arx_connupdate {
  uint64_t opaque;
  uint32_t rx_bump;
  uint32_t rx_pos;
  uint32_t tx_bump;
  uint8_t flags;
} __attribute__((packed));

/** Application RX queue entry */
struct tas_fp_arx {
  union {
    struct tas_fp_arx_connupdate connupdate;
    uint8_t raw[31];
  } __attribute__((packed)) msg;
  volatile uint8_t type;
} __attribute__((packed));

STATIC_ASSERT(sizeof(struct tas_fp_arx) == 32, arx_size);

/******************************************************************************/
/* App TX queue */

#define TAS_FP_ATX_CONNUPDATE 0x1

#define TAS_FP_ATX_FLTXDONE  0x1

/** Application TX queue entry */
struct tas_fp_atx {
  union {
    struct {
      uint32_t rx_bump;
      uint32_t tx_bump;
      uint32_t flow_id;
      uint16_t bump_seq;
      uint8_t  flags;
    } __attribute__((packed)) connupdate;
    uint8_t raw[15];
  } __attribute__((packed)) msg;
  volatile uint8_t type;
} __attribute__((packed));

STATIC_ASSERT(sizeof(struct tas_fp_atx) == 16, atx_size);

/******************************************************************************/
/* Internal fast path state memory */

#define TAS_FP_APPST_NUM        8
#define TAS_FP_APPST_CTX_NUM   31
#define TAS_FP_APPST_CTX_MCS   16
#define TAS_FP_APPCTX_NUM      16
#define TAS_FP_FLOWST_NUM     (128 * 1024)
#define TAS_FP_FLOWHT_ENTRIES (TAS_FP_FLOWST_NUM * 2)
#define TAS_FP_FLOWHT_NBSZ      4

/** Application state */
struct tas_fp_appst {
  /********************************************************/
  /* read-only fields */

  /** Number of contexts */
  uint16_t ctx_num;

  /** IDs of contexts */
  uint16_t ctx_ids[TAS_FP_APPST_CTX_NUM];
} __attribute__((packed));


/** Application context registers */
struct tas_fp_appctx {
  /********************************************************/
  /* read-only fields */
  uint64_t rx_base;
  uint64_t tx_base;
  uint32_t rx_len;
  uint32_t tx_len;
  uint32_t appst_id;
  int	   evfd;

  /********************************************************/
  /* read-write fields */
  uint32_t rx_head;
  uint32_t tx_head;
  uint32_t last_ts;
  uint32_t rx_avail;
} __attribute__((packed));

/** Enable out of order receive processing members */
#define TAS_FP_OOO_RECV 1

#define TAS_FP_FLOWST_SLOWPATH 1
#define TAS_FP_FLOWST_ECN 8
#define TAS_FP_FLOWST_TXFIN 16
#define TAS_FP_FLOWST_RXFIN 32
#define TAS_FP_FLOWST_RX_MASK (~63ULL)

/** Flow state registers */
struct tas_fp_flowst {
  /********************************************************/
  /* read-only fields */

  /** Opaque flow identifier from application */
  uint64_t opaque;

  /** Base address of receive buffer */
  uint64_t rx_base_sp;
  /** Base address of transmit buffer */
  uint64_t tx_base;

  /** Length of receive buffer */
  uint32_t rx_len;
  /** Length of transmit buffer */
  uint32_t tx_len;

  beui32_t local_ip;
  beui32_t remote_ip;

  beui16_t local_port;
  beui16_t remote_port;

  /** Remote MAC address */
  struct eth_addr remote_mac;

  /** Doorbell ID (identifying the app ctx to use) */
  uint16_t db_id;

  /** Flow group for this connection (rss bucket) */
  uint16_t flow_group;
  /** Sequence number of queue pointer bumps */
  uint16_t bump_seq;

  // 56

  /********************************************************/
  /* read-write fields */

  /** spin lock */
  volatile uint32_t lock;

  /** Bytes available for received segments at next position */
  uint32_t rx_avail;
  // 64
  /** Offset in buffer to place next segment */
  uint32_t rx_next_pos;
  /** Next sequence number expected */
  uint32_t rx_next_seq;
  /** Bytes available in remote end for received segments */
  uint32_t rx_remote_avail;
  /** Duplicate ack count */
  uint32_t rx_dupack_cnt;

#ifdef TAS_FP_OOO_RECV
  /* Start of interval of out-of-order received data */
  uint32_t rx_ooo_start;
  /* Length of interval of out-of-order received data */
  uint32_t rx_ooo_len;
#endif

  /** Number of bytes available to be sent */
  uint32_t tx_avail;
  /** Number of bytes up to next pos in the buffer that were sent but not
   * acknowledged yet. */
  uint32_t tx_sent;
  /** Offset in buffer for next segment to be sent */
  uint32_t tx_next_pos;
  /** Sequence number of next segment to be sent */
  uint32_t tx_next_seq;
  /** Timestamp to echo in next packet */
  uint32_t tx_next_ts;

  /** Congestion control rate [kbps] */
  uint32_t tx_rate;
  /** Counter drops */
  uint16_t cnt_tx_drops;
  /** Counter acks */
  uint16_t cnt_rx_acks;
  /** Counter bytes sent */
  uint32_t cnt_rx_ack_bytes;
  /** Counter acks marked */
  uint32_t cnt_rx_ecn_bytes;
  /** RTT estimate */
  uint32_t rtt_est;

// 128
} __attribute__((packed, aligned(64)));

#define TAS_FP_FLOWHTE_VALID  (1 << 31)
#define TAS_FP_FLOWHTE_POSSHIFT 29

/** Flow lookup table entry */
struct tas_fp_flowhte {
  uint32_t flow_id;
  uint32_t flow_hash;
} __attribute__((packed));


#define TAS_FP_MAX_FLOWGROUPS 4096

/** Layout of internal pipeline memory */
struct tas_fp_state {
  /* registers for application context queues */
  struct tas_fp_appctx appctx[TAS_FP_APPST_CTX_MCS][TAS_FP_APPCTX_NUM];

  /* registers for flow state */
  struct tas_fp_flowst flowst[TAS_FP_FLOWST_NUM];

  /* flow lookup table */
  struct tas_fp_flowhte flowht[TAS_FP_FLOWHT_ENTRIES];

  /* registers for kernel queues */
  struct tas_fp_appctx kctx[TAS_FP_APPST_CTX_MCS];

  /* registers for application state */
  struct tas_fp_appst appst[TAS_FP_APPST_NUM];

  uint8_t flow_group_steering[TAS_FP_MAX_FLOWGROUPS];
} __attribute__((packed));


void util_tas_kick(struct tas_fp_appctx *ctx, uint32_t ts_us);

#endif /* ndef TAS_FPIF_H_ */
