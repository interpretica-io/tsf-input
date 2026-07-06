/* SPDX-License-Identifier: MIT */
/* Copyright (C) 2026 Interpretica, Unipessoal Lda. All rights reserved. */
/** @file
 * @brief Reading Linux input devices on an agent from a test
 *
 * The engine-side layer over the input_* RPCs: it asks the agent to
 * read its input devices over libevdev and parses the newline/tab
 * record text into vectors of #tapi_input_device / #tapi_input_event /
 * #tapi_input_abs.
 */

#define TE_LGR_USER     "TAPI INPUT"

#include "te_config.h"

#include <stdlib.h>
#include <string.h>

#include "te_defs.h"
#include "te_errno.h"
#include "te_alloc.h"
#include "te_str.h"
#include "te_string.h"
#include "te_vector.h"
#include "logger_api.h"

#include "tapi_input.h"
#include "tapi_input_rpc.h"

/**
 * Split @p line (up to @p len) into up to @p n heap fields on tabs;
 * returns how many were found, missing ones left @c NULL.
 */
static size_t
input_split(const char *line, size_t len, char **out, size_t n)
{
    const char *p = line;
    const char *end = line + len;
    size_t i;

    for (i = 0; i < n; i++)
        out[i] = NULL;

    for (i = 0; i < n && p <= end; i++)
    {
        const char *tab = memchr(p, '\t', (size_t)(end - p));
        size_t flen = (tab != NULL && tab < end) ? (size_t)(tab - p) :
                      (size_t)(end - p);

        out[i] = TE_STRNDUP(p, flen);
        if (tab == NULL || tab >= end)
        {
            i++;
            break;
        }
        p = tab + 1;
    }

    return i;
}

/** Set the ev_types bitmask from a "03,01,11" comma-hex list. */
static void
input_parse_types(const char *csv, tapi_input_device *device)
{
    const char *p = csv;

    device->ev_types = 0;
    while (p != NULL && *p != '\0')
    {
        unsigned long t = strtoul(p, NULL, 16);
        const char *comma = strchr(p, ',');

        if (t < 32)
            device->ev_types |= (uint32_t)(1u << t);
        p = comma != NULL ? comma + 1 : NULL;
    }
}

/** Parse one input_list() line into a device; false on a short line. */
static bool
input_parse_device(const char *line, size_t len, tapi_input_device *device)
{
    char *f[11];
    size_t got = input_split(line, len, f, 11);
    bool ok = false;
    size_t i;

    if (got < 11)
        goto out;

    memset(device, 0, sizeof(*device));
    device->node = TE_STRDUP(f[0]);
    device->name = (f[1] != NULL && f[1][0] != '\0') ? TE_STRDUP(f[1]) : NULL;
    device->bus = (uint16_t)strtoul(f[2], NULL, 16);
    device->vendor = (uint16_t)strtoul(f[3], NULL, 16);
    device->product = (uint16_t)strtoul(f[4], NULL, 16);
    device->version = (uint16_t)strtoul(f[5], NULL, 16);
    input_parse_types(f[6], device);
    device->n_keys = (int)strtol(f[7], NULL, 10);
    device->n_abs = (int)strtol(f[8], NULL, 10);
    device->n_rel = (int)strtol(f[9], NULL, 10);
    device->n_sw = (int)strtol(f[10], NULL, 10);
    ok = true;

out:
    for (i = 0; i < got; i++)
        free(f[i]);
    return ok;
}

/* See description in tapi_input.h */
te_errno
tapi_input_list(rcf_rpc_server *rpcs, te_vec *devices)
{
    te_string raw = TE_STRING_INIT;
    const char *line;
    te_errno rc;

    *devices = (te_vec)TE_VEC_INIT(tapi_input_device);

    rc = rpc_input_list(rpcs, NULL, &raw);
    if (rc != 0)
    {
        te_string_free(&raw);
        return rc;
    }

    for (line = te_string_value(&raw); line != NULL && *line != '\0'; )
    {
        const char *nl = strchr(line, '\n');
        size_t len = nl != NULL ? (size_t)(nl - line) : strlen(line);
        tapi_input_device device;

        if (len != 0 && input_parse_device(line, len, &device))
            TE_VEC_APPEND(devices, device);
        line = nl != NULL ? nl + 1 : NULL;
    }

    te_string_free(&raw);

    return 0;
}

/* See description in tapi_input.h */
te_errno
tapi_input_capture(rcf_rpc_server *rpcs, const char *node, int timeout_ms,
                   te_vec *events)
{
    te_string raw = TE_STRING_INIT;
    const char *line;
    te_errno rc;

    *events = (te_vec)TE_VEC_INIT(tapi_input_event);

    rc = rpc_input_capture(rpcs, node, timeout_ms, NULL, &raw);
    if (rc != 0)
    {
        te_string_free(&raw);
        return rc;
    }

    for (line = te_string_value(&raw); line != NULL && *line != '\0'; )
    {
        const char *nl = strchr(line, '\n');
        size_t len = nl != NULL ? (size_t)(nl - line) : strlen(line);
        char *f[4];
        size_t got = input_split(line, len, f, 4);

        if (got >= 4)
        {
            tapi_input_event ev;
            char *dot = strchr(f[0], '.');

            memset(&ev, 0, sizeof(ev));
            ev.sec = strtol(f[0], NULL, 10);
            ev.usec = dot != NULL ? strtol(dot + 1, NULL, 10) : 0;
            ev.type = (int)strtol(f[1], NULL, 10);
            ev.code = (int)strtol(f[2], NULL, 10);
            ev.value = (int)strtol(f[3], NULL, 10);
            TE_VEC_APPEND(events, ev);
        }
        while (got-- > 0)
            free(f[got]);
        line = nl != NULL ? nl + 1 : NULL;
    }

    te_string_free(&raw);

    return 0;
}

/* See description in tapi_input.h */
te_errno
tapi_input_absinfo(rcf_rpc_server *rpcs, const char *node, te_vec *axes)
{
    te_string raw = TE_STRING_INIT;
    const char *line;
    te_errno rc;

    *axes = (te_vec)TE_VEC_INIT(tapi_input_abs);

    rc = rpc_input_absinfo(rpcs, node, &raw);
    if (rc != 0)
    {
        te_string_free(&raw);
        return rc;
    }

    for (line = te_string_value(&raw); line != NULL && *line != '\0'; )
    {
        const char *nl = strchr(line, '\n');
        size_t len = nl != NULL ? (size_t)(nl - line) : strlen(line);
        char *f[7];
        size_t got = input_split(line, len, f, 7);

        if (got >= 7)
        {
            tapi_input_abs a;

            a.code = (int)strtol(f[0], NULL, 10);
            a.value = (int)strtol(f[1], NULL, 10);
            a.minimum = (int)strtol(f[2], NULL, 10);
            a.maximum = (int)strtol(f[3], NULL, 10);
            a.fuzz = (int)strtol(f[4], NULL, 10);
            a.flat = (int)strtol(f[5], NULL, 10);
            a.resolution = (int)strtol(f[6], NULL, 10);
            TE_VEC_APPEND(axes, a);
        }
        while (got-- > 0)
            free(f[got]);
        line = nl != NULL ? nl + 1 : NULL;
    }

    te_string_free(&raw);

    return 0;
}

/* See description in tapi_input.h */
te_errno
tapi_input_inject(rcf_rpc_server *rpcs, const char *spec, const char *name,
                  int *count)
{
    return rpc_input_inject(rpcs, spec, name, count);
}

/* See description in tapi_input.h */
bool
tapi_input_has_type(const tapi_input_device *device, int ev_type)
{
    if (ev_type < 0 || ev_type >= 32)
        return false;
    return (device->ev_types & (uint32_t)(1u << ev_type)) != 0;
}

/* See description in tapi_input.h */
const char *
tapi_input_type2str(int ev_type)
{
    switch (ev_type)
    {
        case 0x00: return "EV_SYN";
        case 0x01: return "EV_KEY";
        case 0x02: return "EV_REL";
        case 0x03: return "EV_ABS";
        case 0x04: return "EV_MSC";
        case 0x05: return "EV_SW";
        case 0x11: return "EV_LED";
        case 0x12: return "EV_SND";
        case 0x14: return "EV_REP";
        case 0x15: return "EV_FF";
        case 0x16: return "EV_PWR";
        case 0x17: return "EV_FF_STATUS";
        default:   return "EV_?";
    }
}

/* See description in tapi_input.h */
void
tapi_input_device_log(const tapi_input_device *device)
{
    te_string types = TE_STRING_INIT;
    int t;
    bool first = true;

    for (t = 0; t < 32; t++)
    {
        if (tapi_input_has_type(device, t))
        {
            te_string_append(&types, "%s%s", first ? "" : " ",
                             tapi_input_type2str(t));
            first = false;
        }
    }

    RING("input %s '%s' [%04x:%04x:%04x v%04x]: %s | keys=%d abs=%d rel=%d "
         "sw=%d", device->node,
         device->name != NULL ? device->name : "?",
         device->bus, device->vendor, device->product, device->version,
         te_string_value(&types), device->n_keys, device->n_abs,
         device->n_rel, device->n_sw);

    te_string_free(&types);
}

/* See description in tapi_input.h */
void
tapi_input_list_free(te_vec *devices)
{
    tapi_input_device *device;

    TE_VEC_FOREACH(devices, device)
    {
        free(device->node);
        free(device->name);
    }
    te_vec_free(devices);
}
