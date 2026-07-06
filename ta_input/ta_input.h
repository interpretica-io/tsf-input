/* SPDX-License-Identifier: MIT */
/* Copyright (C) 2026 Interpretica, Unipessoal Lda. All rights reserved. */
/** @file
 * @brief Agent-side Linux input (evdev)
 *
 * Reading the Linux input devices of an agent over **libevdev**
 * (@c libevdev/libevdev.h, @c -levdev): the library is linked into the
 * agent and called in-process, nothing is spawned and @c evtest is not
 * scraped. It lists the @c /dev/input/event* devices and their
 * capabilities, captures the events one emits over a window, and reads
 * the absolute-axis ranges a device declares.
 *
 * Listing and capture are read-only. The one exception is
 * ta_input_inject(), which creates a @c uinput device and writes
 * synthetic events into the system - that generates input, so it is a
 * separate call a caller asks for explicitly.
 *
 * Results come back as newline-separated text, one record per line with
 * tab-separated fields, the same shape tsf-upnp/tsf-usb use - the
 * engine side parses them.
 */

#ifndef __TA_INPUT_H__
#define __TA_INPUT_H__

#include "te_errno.h"
#include "te_string.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * List the agent's input (evdev) devices.
 *
 * One device per line, tab-separated:
 * @c "node\\tname\\tbus\\tvendor\\tproduct\\tversion\\tevtypes\\t
 * nkeys\\tnabs\\tnrel\\tnsw", where @a node is the @c /dev/input/eventN
 * path, the id quad is hex, @a evtypes is a comma-separated list of the
 * supported @c EV_* type codes in hex, and the four counts are how many
 * key/abs/rel/switch codes the device supports.
 *
 * @param[out] count    Number of devices.
 * @param[out] result   The device lines.
 *
 * @return Status code.
 */
extern te_errno ta_input_list(int *count, te_string *result);

/**
 * Capture the events one device emits over a window.
 *
 * One event per line, tab-separated:
 * @c "sec.usec\\ttype\\tcode\\tvalue" (the time is the event's own
 * timestamp; @a type and @a code are numeric @c EV_* / code values).
 *
 * @param[in]  node         The @c /dev/input/eventN path.
 * @param[in]  timeout_ms   How long to collect, ms.
 * @param[out] count        Number of events captured.
 * @param[out] result       The event lines.
 *
 * @return Status code.
 * @retval TE_ENODEV        The node could not be opened as an evdev.
 */
extern te_errno ta_input_capture(const char *node, int timeout_ms,
                                 int *count, te_string *result);

/**
 * Read the absolute-axis ranges a device declares.
 *
 * One axis per line, tab-separated:
 * @c "code\\tvalue\\tmin\\tmax\\tfuzz\\tflat\\tresolution".
 *
 * @param[in]  node     The @c /dev/input/eventN path.
 * @param[out] result   The axis lines (empty when the device has no
 *                      @c EV_ABS axes).
 *
 * @return Status code.
 */
extern te_errno ta_input_absinfo(const char *node, te_string *result);

/**
 * Inject synthetic events through a @c uinput device.
 *
 * Generates input into the system: it creates a @c uinput device able
 * to carry the events in @p spec and writes them, each followed by a
 * @c SYN_REPORT. A caller asks for this explicitly; it is never part of
 * a read.
 *
 * @param[in]  spec     Events as @c "type:code:value" triplets, one per
 *                      line or separated by @c ';'. Values are numeric.
 * @param[in]  name     Name for the created @c uinput device, or
 *                      @c NULL for a default.
 * @param[out] count    Number of events written.
 *
 * @return Status code.
 * @retval TE_EACCES    No access to @c /dev/uinput.
 */
extern te_errno ta_input_inject(const char *spec, const char *name,
                                int *count);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TA_INPUT_H__ */
