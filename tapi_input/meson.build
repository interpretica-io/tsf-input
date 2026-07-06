# SPDX-License-Identifier: MIT
# Copyright (C) 2026 Interpretica, Unipessoal Lda. All rights reserved.
#
# External TE library (see TE_EXT_REPO): engine-side TAPI for reading
# Linux input (evdev) devices on agents. Built as a shared library on
# the engine platform. It needs the input_* RPCs in rpcxdr, so add
# ../ta_input/input_rpc.x.m4 to the rpcxdr definitions of the engine
# platform.

if get_option('engine')
    build_lib_shared = true
endif
build_lib_static = not get_option('engine')
install_lib = install_dev

headers += files(
    'tapi_input.h',
    'tapi_input_rpc.h',
)

sources += files(
    'tapi_input.c',
    'tapi_input_rpc.c',
)

te_libs += [
    'tapi',
    'tapi_rpc',
    'rcfrpc',
    'confapi',
    'rcfapi',
    'rpcxdr',
    'logger_core',
    'tools',
]
