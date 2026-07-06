/* SPDX-License-Identifier: MIT */
/* Copyright (C) 2026 Interpretica, Unipessoal Lda. All rights reserved. */
/** @file
 * @brief Agent-side Linux input (evdev) over libevdev
 *
 * Written against the libevdev API and the Linux input uapi. Listing
 * and capture open a device read-only and read; injection creates a
 * uinput device and writes. The devices are opened and closed inside
 * each call, so nothing has to survive between calls.
 */

#define TE_LGR_USER     "TA INPUT"

#include "te_config.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/time.h>

#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>

#include "te_defs.h"
#include "te_errno.h"
#include "te_alloc.h"
#include "te_str.h"
#include "te_string.h"
#include "logger_api.h"

#include "ta_input.h"

/** The input device directory. */
#define TA_INPUT_DIR "/dev/input"

/** Turn a negative errno (libevdev convention) into a TE status. */
static te_errno
input_rc(int err, const char *what)
{
    int e = err < 0 ? -err : err;

    if (err == 0)
        return 0;

    ERROR("%s: %s", what, strerror(e));
    switch (e)
    {
        case EACCES:  return TE_RC(TE_TA_UNIX, TE_EACCES);
        case ENOENT:  return TE_RC(TE_TA_UNIX, TE_ENODEV);
        case ENODEV:  return TE_RC(TE_TA_UNIX, TE_ENODEV);
        case ENOMEM:  return TE_RC(TE_TA_UNIX, TE_ENOMEM);
        case EINVAL:  return TE_RC(TE_TA_UNIX, TE_EINVAL);
        default:      return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }
}

/** Count how many codes of @p type the device supports. */
static int
input_count_codes(struct libevdev *dev, unsigned int type)
{
    int max = libevdev_event_type_get_max(type);
    int code;
    int n = 0;

    if (max < 0)
        return 0;
    for (code = 0; code <= max; code++)
    {
        if (libevdev_has_event_code(dev, type, (unsigned int)code))
            n++;
    }
    return n;
}

/** Append one device's summary line. */
static void
input_device_line(const char *node, struct libevdev *dev, te_string *result)
{
    te_string types = TE_STRING_INIT;
    bool first = true;
    unsigned int t;

    for (t = 0; t <= EV_MAX; t++)
    {
        if (libevdev_has_event_type(dev, t))
        {
            te_string_append(&types, "%s%02x", first ? "" : ",", t);
            first = false;
        }
    }

    te_string_append(result,
        "%s\t%s\t%04x\t%04x\t%04x\t%04x\t%s\t%d\t%d\t%d\t%d\n",
        node,
        libevdev_get_name(dev) != NULL ? libevdev_get_name(dev) : "",
        libevdev_get_id_bustype(dev), libevdev_get_id_vendor(dev),
        libevdev_get_id_product(dev), libevdev_get_id_version(dev),
        te_string_value(&types),
        input_count_codes(dev, EV_KEY), input_count_codes(dev, EV_ABS),
        input_count_codes(dev, EV_REL), input_count_codes(dev, EV_SW));

    te_string_free(&types);
}

/* See description in ta_input.h */
te_errno
ta_input_list(int *count, te_string *result)
{
    DIR *dir;
    struct dirent *ent;

    *count = 0;

    dir = opendir(TA_INPUT_DIR);
    if (dir == NULL)
        return input_rc(-errno, "opendir " TA_INPUT_DIR);

    while ((ent = readdir(dir)) != NULL)
    {
        te_string node = TE_STRING_INIT;
        struct libevdev *dev = NULL;
        int fd;

        if (strncmp(ent->d_name, "event", 5) != 0)
            continue;

        te_string_append(&node, "%s/%s", TA_INPUT_DIR, ent->d_name);
        fd = open(node.ptr, O_RDONLY | O_NONBLOCK);
        if (fd >= 0)
        {
            if (libevdev_new_from_fd(fd, &dev) == 0)
            {
                input_device_line(node.ptr, dev, result);
                (*count)++;
                libevdev_free(dev);
            }
            close(fd);
        }
        te_string_free(&node);
    }

    closedir(dir);

    return 0;
}

/** Milliseconds between two timevals, clamped at zero. */
static int
input_ms_left(const struct timeval *deadline)
{
    struct timeval now;
    long ms;

    gettimeofday(&now, NULL);
    ms = (deadline->tv_sec - now.tv_sec) * 1000 +
         (deadline->tv_usec - now.tv_usec) / 1000;
    return ms > 0 ? (int)ms : 0;
}

/* See description in ta_input.h */
te_errno
ta_input_capture(const char *node, int timeout_ms, int *count,
                 te_string *result)
{
    struct libevdev *dev = NULL;
    struct timeval deadline;
    struct timeval add;
    int fd;
    int rc;

    *count = 0;

    fd = open(node, O_RDONLY | O_NONBLOCK);
    if (fd < 0)
        return input_rc(-errno, "open input node");

    rc = libevdev_new_from_fd(fd, &dev);
    if (rc != 0)
    {
        close(fd);
        return input_rc(rc, "libevdev_new_from_fd");
    }

    gettimeofday(&deadline, NULL);
    add.tv_sec = timeout_ms / 1000;
    add.tv_usec = (timeout_ms % 1000) * 1000;
    timeradd(&deadline, &add, &deadline);

    for (;;)
    {
        struct pollfd pfd = { .fd = fd, .events = POLLIN };
        int left = input_ms_left(&deadline);
        struct input_event ev;
        int status;

        if (left == 0)
            break;
        if (poll(&pfd, 1, left) <= 0)
            continue;

        status = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
        while (status == LIBEVDEV_READ_STATUS_SUCCESS ||
               status == LIBEVDEV_READ_STATUS_SYNC)
        {
            te_string_append(result, "%ld.%06ld\t%u\t%u\t%d\n",
                             (long)ev.input_event_sec,
                             (long)ev.input_event_usec,
                             ev.type, ev.code, ev.value);
            (*count)++;
            status = libevdev_next_event(dev,
                        status == LIBEVDEV_READ_STATUS_SYNC ?
                        LIBEVDEV_READ_FLAG_SYNC : LIBEVDEV_READ_FLAG_NORMAL,
                        &ev);
        }
    }

    libevdev_free(dev);
    close(fd);

    return 0;
}

/* See description in ta_input.h */
te_errno
ta_input_absinfo(const char *node, te_string *result)
{
    struct libevdev *dev = NULL;
    int max;
    int code;
    int fd;
    int rc;

    fd = open(node, O_RDONLY | O_NONBLOCK);
    if (fd < 0)
        return input_rc(-errno, "open input node");

    rc = libevdev_new_from_fd(fd, &dev);
    if (rc != 0)
    {
        close(fd);
        return input_rc(rc, "libevdev_new_from_fd");
    }

    max = libevdev_event_type_get_max(EV_ABS);
    for (code = 0; max >= 0 && code <= max; code++)
    {
        const struct input_absinfo *abs;

        if (!libevdev_has_event_code(dev, EV_ABS, (unsigned int)code))
            continue;
        abs = libevdev_get_abs_info(dev, (unsigned int)code);
        if (abs == NULL)
            continue;
        te_string_append(result, "%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
                         code, abs->value, abs->minimum, abs->maximum,
                         abs->fuzz, abs->flat, abs->resolution);
    }

    libevdev_free(dev);
    close(fd);

    return 0;
}

/* See description in ta_input.h */
te_errno
ta_input_inject(const char *spec, const char *name, int *count)
{
    struct libevdev *dev;
    struct libevdev_uinput *uidev = NULL;
    const char *p = spec;
    te_errno rc = 0;
    int created;

    *count = 0;

    dev = libevdev_new();
    if (dev == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    libevdev_set_name(dev, name != NULL ? name : "tsf-input-uinput");

    /*
     * First pass: enable every (type, code) the spec mentions, so the
     * uinput device created from it can carry them. EV_ABS codes need an
     * absinfo; a wide default is used.
     */
    while (p != NULL && *p != '\0')
    {
        unsigned int type;
        unsigned int code;
        int value;

        if (sscanf(p, "%u:%u:%d", &type, &code, &value) == 3)
        {
            libevdev_enable_event_type(dev, type);
            if (type == EV_ABS)
            {
                struct input_absinfo abs = {
                    .minimum = 0, .maximum = 65535,
                };
                libevdev_enable_event_code(dev, type, code, &abs);
            }
            else
            {
                libevdev_enable_event_code(dev, type, code, NULL);
            }
        }
        p = strpbrk(p, ";\n");
        if (p != NULL)
            p++;
    }
    libevdev_enable_event_type(dev, EV_SYN);
    libevdev_enable_event_code(dev, EV_SYN, SYN_REPORT, NULL);

    created = libevdev_uinput_create_from_device(
                  dev, LIBEVDEV_UINPUT_OPEN_MANAGED, &uidev);
    if (created != 0)
    {
        rc = input_rc(created, "libevdev_uinput_create_from_device");
        goto out;
    }

    /* Second pass: write each event, then a SYN_REPORT. */
    for (p = spec; p != NULL && *p != '\0'; )
    {
        unsigned int type;
        unsigned int code;
        int value;

        if (sscanf(p, "%u:%u:%d", &type, &code, &value) == 3)
        {
            libevdev_uinput_write_event(uidev, type, code, value);
            libevdev_uinput_write_event(uidev, EV_SYN, SYN_REPORT, 0);
            (*count)++;
        }
        p = strpbrk(p, ";\n");
        if (p != NULL)
            p++;
    }

out:
    if (uidev != NULL)
        libevdev_uinput_destroy(uidev);
    libevdev_free(dev);

    return rc;
}
