# SPDX-License-Identifier: MIT
# Copyright (C) 2026 Interpretica, Unipessoal Lda. All rights reserved.
#
# External TE library (see TE_EXT_REPO): agent-side Linux input (evdev)
# over libevdev. The agent and its RPC server both link it.

headers += files(
    'ta_input.h',
)

sources += files(
    'ta_input.c',
)

te_libs += [
    'logger_core',
    'tools',
]

cc = meson.get_compiler('c')
evdev_dep = dependency('libevdev', required: false)
if not evdev_dep.found()
    evdev_dep = cc.find_library('evdev', has_headers: 'libevdev/libevdev.h')
endif

deps += [
    evdev_dep,
]
