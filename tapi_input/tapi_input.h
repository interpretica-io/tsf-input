/* SPDX-License-Identifier: MIT */
/* Copyright (C) 2026 Interpretica, Unipessoal Lda. All rights reserved. */
/** @file
 * @brief Reading Linux input devices on an agent from a test
 *
 * @defgroup tapi_input Linux input / evdev (tapi_input)
 * @{
 *
 * Reading the Linux input subsystem of a Test Agent over libevdev in
 * the agent's RPC server (not by scraping @c evtest): what input
 * devices are present and what each can emit, the events a device emits
 * over a window, and the ranges of its absolute axes. Listing and
 * capture are read-only; tapi_input_inject() writes synthetic events
 * through @c uinput and is a call a test makes deliberately.
 *
 * @code
 * te_vec devices = TE_VEC_INIT(tapi_input_device);
 * const tapi_input_device *d;
 *
 * CHECK_RC(tapi_input_list(rpcs, &devices));
 * TE_VEC_FOREACH(&devices, d)
 *     RING("%s '%s' keys=%d abs=%d", d->node, d->name, d->n_keys, d->n_abs);
 * tapi_input_list_free(&devices);
 * @endcode
 */

#ifndef __TAPI_INPUT_H__
#define __TAPI_INPUT_H__

#include <stdint.h>

#include "te_defs.h"
#include "te_errno.h"
#include "te_string.h"
#include "te_vector.h"
#include "rcf_rpc.h"

#ifdef __cplusplus
extern "C" {
#endif

/** One input device as enumerated on the agent. */
typedef struct tapi_input_device {
    /** The @c /dev/input/eventN path. */
    char *node;
    /** The device name, or @c NULL. */
    char *name;
    /** Bus type (@c BUS_*). */
    uint16_t bus;
    /** Vendor ID. */
    uint16_t vendor;
    /** Product ID. */
    uint16_t product;
    /** Version. */
    uint16_t version;
    /** Bitmask of supported @c EV_* type codes (bit N = type N). */
    uint32_t ev_types;
    /** Number of supported key/button codes. */
    int n_keys;
    /** Number of supported absolute axes. */
    int n_abs;
    /** Number of supported relative axes. */
    int n_rel;
    /** Number of supported switches. */
    int n_sw;
} tapi_input_device;

/** One captured input event. */
typedef struct tapi_input_event {
    /** Seconds of the event timestamp. */
    long sec;
    /** Microseconds of the event timestamp. */
    long usec;
    /** Event type (@c EV_*). */
    int type;
    /** Event code. */
    int code;
    /** Event value. */
    int value;
} tapi_input_event;

/** One absolute axis's range. */
typedef struct tapi_input_abs {
    /** Axis code (@c ABS_*). */
    int code;
    /** Current value. */
    int value;
    /** Minimum. */
    int minimum;
    /** Maximum. */
    int maximum;
    /** Noise filter. */
    int fuzz;
    /** Flat region around zero. */
    int flat;
    /** Resolution (units per mm / per radian). */
    int resolution;
} tapi_input_abs;

/**
 * Snapshot the agent's input devices.
 *
 * @param[in]  rpcs     RPC server on the agent.
 * @param[out] devices  Vector of #tapi_input_device; release with
 *                      tapi_input_list_free().
 *
 * @return Status code.
 */
extern te_errno tapi_input_list(rcf_rpc_server *rpcs, te_vec *devices);

/**
 * Capture a device's events over a window.
 *
 * @param[in]  rpcs         RPC server on the agent.
 * @param[in]  node         The @c /dev/input/eventN path.
 * @param[in]  timeout_ms   How long to collect, ms.
 * @param[out] events       Vector of #tapi_input_event; release with
 *                          te_vec_free().
 *
 * @return Status code.
 */
extern te_errno tapi_input_capture(rcf_rpc_server *rpcs, const char *node,
                                   int timeout_ms, te_vec *events);

/**
 * Read a device's absolute-axis ranges.
 *
 * @param[in]  rpcs     RPC server on the agent.
 * @param[in]  node     The @c /dev/input/eventN path.
 * @param[out] axes     Vector of #tapi_input_abs; release with
 *                      te_vec_free().
 *
 * @return Status code.
 */
extern te_errno tapi_input_absinfo(rcf_rpc_server *rpcs, const char *node,
                                   te_vec *axes);

/**
 * Inject synthetic events through a @c uinput device.
 *
 * Generates input into the agent's system.
 *
 * @param[in]  rpcs     RPC server on the agent.
 * @param[in]  spec     Events as @c "type:code:value" triplets, one per
 *                      line or @c ';'-separated.
 * @param[in]  name     Name for the @c uinput device, or @c NULL.
 * @param[out] count    Number of events written, or @c NULL.
 *
 * @return Status code.
 */
extern te_errno tapi_input_inject(rcf_rpc_server *rpcs, const char *spec,
                                  const char *name, int *count);

/**
 * Does a device support a given event type?
 *
 * @param device        Device.
 * @param ev_type       An @c EV_* type code.
 *
 * @return @c true when the device supports it.
 */
extern bool tapi_input_has_type(const tapi_input_device *device,
                                int ev_type);

/**
 * Spell out an @c EV_* type.
 *
 * @param ev_type       An @c EV_* type code.
 *
 * @return A static string, never @c NULL.
 */
extern const char *tapi_input_type2str(int ev_type);

/**
 * Write one device into the log.
 *
 * @param device        Device.
 */
extern void tapi_input_device_log(const tapi_input_device *device);

/**
 * Release a device snapshot.
 *
 * @param devices       Vector from tapi_input_list().
 */
extern void tapi_input_list_free(te_vec *devices);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TAPI_INPUT_H__ */

/**@} <!-- END tapi_input --> */
