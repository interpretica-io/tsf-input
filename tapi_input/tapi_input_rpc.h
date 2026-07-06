/* SPDX-License-Identifier: MIT */
/* Copyright (C) 2026 Interpretica, Unipessoal Lda. All rights reserved. */
/** @file
 * @brief Input TAPI: RPC client wrappers
 *
 * Client wrappers of the input_* RPCs, see input_rpc.x.m4. Tests use
 * tapi_input.h; these are the calls behind it, one per RPC.
 */

#ifndef __TAPI_INPUT_RPC_H__
#define __TAPI_INPUT_RPC_H__

#include "te_errno.h"
#include "te_string.h"
#include "rcf_rpc.h"

#ifdef __cplusplus
extern "C" {
#endif

/** List the agent's input devices (raw record text). */
extern te_errno rpc_input_list(rcf_rpc_server *rpcs, int *count,
                               te_string *result);

/** Capture a device's events over a window (raw record text). */
extern te_errno rpc_input_capture(rcf_rpc_server *rpcs, const char *node,
                                  int timeout_ms, int *count,
                                  te_string *result);

/** Read a device's absolute-axis ranges (raw record text). */
extern te_errno rpc_input_absinfo(rcf_rpc_server *rpcs, const char *node,
                                  te_string *result);

/** Inject synthetic events through uinput. */
extern te_errno rpc_input_inject(rcf_rpc_server *rpcs, const char *spec,
                                 const char *name, int *count);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TAPI_INPUT_RPC_H__ */
