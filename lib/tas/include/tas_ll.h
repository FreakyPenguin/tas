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

#ifndef TAS_LL_H_
#define TAS_LL_H_

/**
 * @file tas_ll.h
 * @brief Public low-level interface for application tas stack.
 *
 * @addtogroup app-stack
 * @brief Application library tas stack (low-level interface)
 */

#include <stdint.h>

#define TAS_MAX_CONTEXTS 32
#define TAS_MAX_FTCPCORES 16

/**
 * A tas context is per-thread state for the stack. (opaque)
 * This includes:
 *   - admin queue pair to tas slow path
 *   - notification queue pair to tas fast path
 */
struct tas_context {
  /* incoming queue from the kernel */
  void *kin_base;
  uint32_t kin_len;
  uint32_t kin_head;

  /* outgoing queue to the kernel */
  void *kout_base;
  uint32_t kout_len;
  uint32_t kout_head;

  /* queues from NIC cores */
  uint32_t rxq_len;
  uint32_t txq_len;
  struct {
    void *txq_base;
    void *rxq_base;
    uint32_t rxq_head;
    uint32_t txq_tail;
    uint32_t txq_avail;
    uint32_t last_ts;
  } queues[TAS_MAX_FTCPCORES];

  /* list of connections with pending updates for NIC */
  struct tas_ll_connection *bump_pending_first;
  struct tas_ll_connection *bump_pending_last;

  /* other */
  uint16_t db_id;
  uint16_t ctx_id;

  uint16_t num_queues;
  uint16_t next_queue;

  int epfd, evfd;
};

/** TCP listening "socket". (opaque) */
struct tas_ll_listener {
  struct tas_ll_connection *conns;

  uint16_t local_port;
  uint8_t status;
};

/** TCP connection. (opaque) */
struct tas_ll_connection {
  /* rx buffer */
  uint8_t *rxb_base;
  uint32_t rxb_len;
  /** pointer to next new byte to be received */
  uint32_t rxb_head;
  /** number of received but not yet freed bytes (behind head). */
  uint32_t rxb_used;
  /** pending rx bump to fast path */
  uint32_t rxb_bump;

  /* tx buffer */
  uint8_t *txb_base;
  uint32_t txb_len;
  /** pointer to next byte to be sent */
  uint32_t txb_head;
  /** number of sent but not yet acked bytes (behind head) */
  uint32_t txb_sent;
  /** number of allocated but not yet sent bytes (after head) */
  uint32_t txb_allocated;
  /** pending tx bump to fast path */
  uint32_t txb_bump;

  uint32_t local_ip;
  uint32_t remote_ip;
  uint16_t local_port;
  uint16_t remote_port;

  uint32_t seq_rx;
  uint32_t seq_tx;

  uint32_t flow_id;
  uint32_t bump_seq;

  struct tas_ll_connection *bump_next;
  struct tas_ll_connection *bump_prev;
  uint16_t fn_core;

  uint8_t bump_pending;
  uint8_t status;
  uint8_t flags;
  /* TODO: kill rx closed and merge into flags */
  uint8_t rx_closed;
};

/** Types of events that can occur in tas contexts */
enum tas_ll_event_type {
  /** tas_ll_listen_open() result. */
  TAS_LL_EV_LISTEN_OPEN,
  /** New connection on listening socket arrived. */
  TAS_LL_EV_LISTEN_NEWCONN,
  /** Accept operation completed */
  TAS_LL_EV_LISTEN_ACCEPT,

  /** tas_ll_connection_open() result */
  TAS_LL_EV_CONN_OPEN,
  /** Connection was closed */
  TAS_LL_EV_CONN_CLOSED,
  /** Data arrived on connection */
  TAS_LL_EV_CONN_RECEIVED,
  /** More send buffer available */
  TAS_LL_EV_CONN_SENDBUF,
  /** Receive stream closed */
  TAS_LL_EV_CONN_RXCLOSED,
  /** transmit stream closed */
  TAS_LL_EV_CONN_TXCLOSED,
  /** Connection moved to new context */
  TAS_LL_EV_CONN_MOVED,
};

/** Events that can occur on tas contexts. */
struct tas_ll_event {
  uint8_t event_type;
  union {
    /** For #TAS_LL_EV_LISTEN_OPEN */
    struct {
      int16_t status;
      struct tas_ll_listener *listener;
    } listen_open;
    /** For #TAS_LL_EV_LISTEN_NEWCONN */
    struct {
      uint16_t remote_port;
      uint32_t remote_ip;
      struct tas_ll_listener *listener;
    } listen_newconn;
    /** For #TAS_LL_EV_LISTEN_ACCEPT */
    struct {
      int16_t status;
      struct tas_ll_connection *conn;
    } listen_accept;

    /** For #TAS_LL_EV_CONN_OPEN */
    struct {
      int16_t status;
      struct tas_ll_connection *conn;
    } conn_open;
    /** For #TAS_LL_EV_CONN_RECEIVED */
    struct {
      void *buf;
      size_t len;
      struct tas_ll_connection *conn;
    } conn_received;
    /** For #TAS_LL_EV_CONN_SENDBUF */
    struct {
      struct tas_ll_connection *conn;
    } conn_sendbuf;
    /** For #TAS_LL_EV_CONN_RXCLOSED */
    struct {
      struct tas_ll_connection *conn;
    } conn_rxclosed;
    /** For #TAS_LL_EV_CONN_TXCLOSED */
    struct {
      struct tas_ll_connection *conn;
    } conn_txclosed;
    /** For #TAS_LL_EV_CONN_MOVED */
    struct {
      int16_t status;
      struct tas_ll_connection *conn;
    } conn_moved;
    /** For #TAS_LL_EV_CONN_CLOSED */
    struct {
      int16_t status;
      struct tas_ll_connection *conn;
    } conn_closed;
  } ev;
};

#define TAS_LL_LISTEN_REUSEPORT 0x1

/**
 * Initializes global tas state, must only be called once.
 * @return 0 on success, < 0 on failure
 */
int tas_ll_init(void);

/**
 * Create a tas context.
 */
int tas_context_create(struct tas_context *ctx);

/**
 * Poll events from a tas context.
 */
int tas_context_poll(struct tas_context *ctx, int num,
    struct tas_ll_event *events);

void tas_ll_block(struct tas_context *ctx, int timeout_ms);

/*****************************************************************************/
/* Regular TCP connection management */

/** Open a listening socket (asynchronous). */
int tas_ll_listen_open(struct tas_context *ctx,
    struct tas_ll_listener *lst, uint16_t port, uint32_t backlog,
    uint32_t flags);

/** Accept connections on a listening socket (asynchronous). This can be called
 * more than once to register multiple connection handles. */
int tas_ll_listen_accept(struct tas_context *ctx,
    struct tas_ll_listener *lst, struct tas_ll_connection *conn);


/** Open a connection (asynchronous). */
int tas_ll_connection_open(struct tas_context *ctx,
    struct tas_ll_connection *conn, uint32_t dst_ip, uint16_t dst_port);

/** Close a connection (asynchronous). */
int tas_ll_connection_close(struct tas_context *ctx,
    struct tas_ll_connection *conn);

/** Receive processing for `len' bytes done. */
int tas_ll_connection_rx_done(struct tas_context *ctx, struct tas_ll_connection *conn, size_t len);

/** Allocate transmit buffer for `len' bytes, returns number of bytes
 * allocated.
 *
 * NOTE: short allocs can occur if buffer wraps around
 */
ssize_t tas_ll_connection_tx_alloc(struct tas_ll_connection *conn, size_t len,
    void **buf);

/** Allocate transmit buffer for `len' bytes, returns number of bytes
 * allocated. May be split across two buffers, in case of wrap around. */
ssize_t tas_ll_connection_tx_alloc2(struct tas_ll_connection *conn, size_t len,
    void **buf_1, size_t *len_1, void **buf_2);

/** Send previously allocated bytes in transmit buffer */
int tas_ll_connection_tx_send(struct tas_context *ctx,
        struct tas_ll_connection *conn, size_t len);

/** Send previously allocated bytes in transmit buffer */
int tas_ll_connection_tx_close(struct tas_context *ctx,
        struct tas_ll_connection *conn);

/** Make sure there is room in the context send queue (not send buffer)
 *
 * Returns 0 if transmit is possible, -1 otherwise.
 */
int tas_ll_connection_tx_possible(struct tas_context *ctx,
    struct tas_ll_connection *conn);

/** Move connection to specfied context */
int tas_ll_connection_move(struct tas_context *ctx,
        struct tas_ll_connection *conn);

#endif /* ndef TAS_LL_H_ */
