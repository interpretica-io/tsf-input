/* SPDX-License-Identifier: MIT */
/* Copyright (C) 2026 Interpretica, Unipessoal Lda. All rights reserved. */
/** @file
 * @brief RPC for Linux input (evdev)
 *
 * The RPCs of rpcs_input, a thin layer over ta_input, which reads the
 * agent's input devices in the RPC server process over libevdev. Add
 * this file to the rpcxdr definitions of the engine platform and of the
 * agent platform:
 *
 *   TE_LIB_PARMS([rpcxdr], [<platform>], [],
 *                [--with-rpcdefs=tarpc_job.x.m4,../ta_input/input_rpc.x.m4])
 *
 * No handle survives between calls: a device is named by its
 * /dev/input/eventN node. Results that are lists come back as
 * newline-separated text, one record per line with tab-separated
 * fields - the engine side parses them, the same shape tsf-usb uses.
 */

/* input_list(): one summary line per device (see ta_input.h). */
struct tarpc_input_list_in {
    struct tarpc_in_arg common;
};

struct tarpc_input_list_out {
    struct tarpc_out_arg common;

    tarpc_int       retval;
    tarpc_int       count;
    string          result<>;
};

/* input_capture(): events from one device over a window. */
struct tarpc_input_capture_in {
    struct tarpc_in_arg common;

    string          node<>;
    tarpc_int       timeout_ms;
};

struct tarpc_input_capture_out {
    struct tarpc_out_arg common;

    tarpc_int       retval;
    tarpc_int       count;
    string          result<>;
};

/* input_absinfo(): the absolute-axis ranges of one device. */
struct tarpc_input_absinfo_in {
    struct tarpc_in_arg common;

    string          node<>;
};

struct tarpc_input_absinfo_out {
    struct tarpc_out_arg common;

    tarpc_int       retval;
    string          result<>;
};

/*
 * input_inject(): write synthetic events through a uinput device.
 * spec is "type:code:value" triplets, one per line or ';'-separated.
 */
struct tarpc_input_inject_in {
    struct tarpc_in_arg common;

    string          spec<>;
    string          name<>;
};

struct tarpc_input_inject_out {
    struct tarpc_out_arg common;

    tarpc_int       retval;
    tarpc_int       count;
};

program input
{
    version ver0
    {
        RPC_DEF(input_list)
        RPC_DEF(input_capture)
        RPC_DEF(input_absinfo)
        RPC_DEF(input_inject)
    } = 1;
} = 34;
