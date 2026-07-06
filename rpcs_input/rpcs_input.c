/* SPDX-License-Identifier: MIT */
/* Copyright (C) 2026 Interpretica, Unipessoal Lda. All rights reserved. */
/** @file
 * @brief Input RPC server library
 *
 * The input_* RPCs (see input_rpc.x.m4) on top of ta_input.
 * TARPC_FUNC_STATIC() binds an RPC to the function of the same name,
 * so each RPC has a plain C function first and the wrapper after it.
 */

#define TE_LGR_USER     "RPC INPUT"

#include "te_config.h"

#include <string.h>

#include "te_defs.h"
#include "te_errno.h"
#include "te_alloc.h"
#include "te_str.h"
#include "te_string.h"
#include "rpc_server.h"

#include "ta_input.h"

/* Hand a te_string result over to an RPC string field (never NULL). */
static char *
take(te_string *str)
{
    return str->ptr != NULL ? str->ptr : TE_STRDUP("");
}

static te_errno
input_list(int *count, char **result)
{
    te_string r = TE_STRING_INIT;
    te_errno rc = ta_input_list(count, &r);

    *result = take(&r);
    return rc;
}

TARPC_FUNC_STATIC(input_list, {},
{
    int count = 0;

    MAKE_CALL(out->retval = func(&count, &out->result));
    out->count = count;
    out->common.errno_changed = false;
})

static te_errno
input_capture(const char *node, int timeout_ms, int *count, char **result)
{
    te_string r = TE_STRING_INIT;
    te_errno rc = ta_input_capture(node, timeout_ms, count, &r);

    *result = take(&r);
    return rc;
}

TARPC_FUNC_STATIC(input_capture, {},
{
    int count = 0;

    MAKE_CALL(out->retval = func(in->node, in->timeout_ms, &count,
                                 &out->result));
    out->count = count;
    out->common.errno_changed = false;
})

static te_errno
input_absinfo(const char *node, char **result)
{
    te_string r = TE_STRING_INIT;
    te_errno rc = ta_input_absinfo(node, &r);

    *result = take(&r);
    return rc;
}

TARPC_FUNC_STATIC(input_absinfo, {},
{
    MAKE_CALL(out->retval = func(in->node, &out->result));
    out->common.errno_changed = false;
})

static te_errno
input_inject(const char *spec, const char *name, int *count)
{
    return ta_input_inject(spec, (name != NULL && name[0] != '\0') ?
                           name : NULL, count);
}

TARPC_FUNC_STATIC(input_inject, {},
{
    int count = 0;

    MAKE_CALL(out->retval = func(in->spec, in->name, &count));
    out->count = count;
    out->common.errno_changed = false;
})
