/* SPDX-License-Identifier: MIT */
/* Copyright (C) 2026 Interpretica, Unipessoal Lda. All rights reserved. */
/** @file
 * @brief Input TAPI: RPC client wrappers
 *
 * The rcf_rpc_call() boilerplate behind tapi_input. The RPCs return
 * te_errno; an RPC transport failure is mapped to TE_ECORRUPTED.
 */

#define TE_LGR_USER     "TAPI INPUT RPC"

#include "te_config.h"

#include <string.h>

#include "te_defs.h"
#include "te_errno.h"
#include "te_string.h"
#include "logger_api.h"
#include "tapi_rpc_internal.h"
#include "tarpc.h"

#include "tapi_input_rpc.h"

#define CHECK_RPC_ERRNO_UNCHANGED(_func, _var) \
    CHECK_RETVAL_VAR_ERR_COND(_func, _var, false,                    \
                              TE_RC(TE_TAPI, TE_ECORRUPTED), false)

/* Append an RPC string result, when there is one. */
static void
take_string(te_string *dst, const char *src)
{
    if (dst != NULL && src != NULL)
        te_string_append(dst, "%s", src);
}

/* See description in tapi_input_rpc.h */
te_errno
rpc_input_list(rcf_rpc_server *rpcs, int *count, te_string *result)
{
    tarpc_input_list_in in;
    tarpc_input_list_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    rcf_rpc_call(rpcs, "input_list", &in, &out);
    CHECK_RPC_ERRNO_UNCHANGED(input_list, out.retval);
    TAPI_RPC_LOG(rpcs, input_list, "", "%r count=%d", out.retval, out.count);

    if (out.retval == 0)
    {
        if (count != NULL)
            *count = out.count;
        take_string(result, out.result);
    }
    RETVAL_TE_ERRNO(input_list, out.retval);
}

/* See description in tapi_input_rpc.h */
te_errno
rpc_input_capture(rcf_rpc_server *rpcs, const char *node, int timeout_ms,
                  int *count, te_string *result)
{
    tarpc_input_capture_in in;
    tarpc_input_capture_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    in.node = (char *)node;
    in.timeout_ms = timeout_ms;

    rpcs->timeout = TE_SEC2MS(10) + timeout_ms;

    rcf_rpc_call(rpcs, "input_capture", &in, &out);
    CHECK_RPC_ERRNO_UNCHANGED(input_capture, out.retval);
    TAPI_RPC_LOG(rpcs, input_capture, "%s, %d ms", "%r count=%d",
                 node != NULL ? node : "", timeout_ms, out.retval,
                 out.count);

    if (out.retval == 0)
    {
        if (count != NULL)
            *count = out.count;
        take_string(result, out.result);
    }
    RETVAL_TE_ERRNO(input_capture, out.retval);
}

/* See description in tapi_input_rpc.h */
te_errno
rpc_input_absinfo(rcf_rpc_server *rpcs, const char *node, te_string *result)
{
    tarpc_input_absinfo_in in;
    tarpc_input_absinfo_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    in.node = (char *)node;

    rcf_rpc_call(rpcs, "input_absinfo", &in, &out);
    CHECK_RPC_ERRNO_UNCHANGED(input_absinfo, out.retval);
    TAPI_RPC_LOG(rpcs, input_absinfo, "%s", "%r",
                 node != NULL ? node : "", out.retval);

    if (out.retval == 0)
        take_string(result, out.result);
    RETVAL_TE_ERRNO(input_absinfo, out.retval);
}

/* See description in tapi_input_rpc.h */
te_errno
rpc_input_inject(rcf_rpc_server *rpcs, const char *spec, const char *name,
                 int *count)
{
    tarpc_input_inject_in in;
    tarpc_input_inject_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    in.spec = (char *)(spec != NULL ? spec : "");
    in.name = (char *)(name != NULL ? name : "");

    rcf_rpc_call(rpcs, "input_inject", &in, &out);
    CHECK_RPC_ERRNO_UNCHANGED(input_inject, out.retval);
    TAPI_RPC_LOG(rpcs, input_inject, "name=%s", "%r count=%d",
                 name != NULL ? name : "", out.retval, out.count);

    if (out.retval == 0 && count != NULL)
        *count = out.count;
    RETVAL_TE_ERRNO(input_inject, out.retval);
}
