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

#ifndef INTERNAL_H_
#define INTERNAL_H_

#include <tas_ll.h>
#include <tas_fpif.h>
#include <utils_circ.h>

#define OPAQUE_PTR(x) ((void *) (uintptr_t) (x))
#define OPAQUE(x) ((uintptr_t) (x))

#define CONN_FLAG_TXEOS 1
#define CONN_FLAG_TXEOS_ALLOC 2
#define CONN_FLAG_TXEOS_ACK 4
#define CONN_FLAG_RXEOS 8

enum conn_state {
  CONN_CLOSED,
  CONN_OPEN_REQUESTED,
  CONN_ACCEPT_REQUESTED,
  CONN_OPEN,
  CONN_CLOSE_REQUESTED,
};

extern void *flexnic_mem;
extern int flexnic_evfd[TAS_MAX_FTCPCORES];

int flextcp_kernel_connect(void);
int flextcp_kernel_newctx(struct tas_context *ctx);
void flextcp_kernel_kick(void);

int tas_context_tx_alloc(struct tas_context *ctx,
    struct tas_fp_atx **atx, uint16_t core);
void tas_context_tx_done(struct tas_context *ctx, uint16_t core);

uint32_t flextcp_conn_txbuf_available(struct tas_ll_connection *conn);
int flextcp_conn_pushtxeos(struct tas_context *ctx,
        struct tas_ll_connection *conn);

#endif /* ndef INTERNAL_H_ */
