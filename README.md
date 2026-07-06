# tsf-input

Reading the Linux input subsystem of a Test Agent, packaged as an
external Test Environment (TE) repository (consumed with the
`TE_EXT_REPO` builder directive). It drives evdev from an agent over a
low-level C library — **libevdev, no Python, nothing spawned** — to
enumerate input devices, capture the events they emit, and read their
absolute-axis ranges; optionally it injects synthetic events through
`uinput`.

Three libraries:

- `ta_input` — agent side. Linux input over **libevdev**
  (`libevdev/libevdev.h`, `-levdev`): list the `/dev/input/event*`
  devices with their names, id quads and capabilities; capture a
  device's events over a time window; read its `EV_ABS` axis ranges.
  Listing and capture are read-only. `ta_input_inject()` creates a
  `uinput` device and writes synthetic events — it generates input, so
  it is a separate call. The agent and its RPC server both link it.
- `rpcs_input` — the `input_*` RPCs for the agent's RPC server, thin
  wrappers over `ta_input`.
- `tapi_input` — engine side. `tapi_input.h` lists devices into a
  `tapi_input_device` vector, captures events into a
  `tapi_input_event` vector, reads axes into a `tapi_input_abs` vector,
  and injects; `tapi_input_rpc.h` is the one-per-RPC layer beneath.

TE has no input-device enumeration of its own.

## What it reads

```c
te_vec devices = TE_VEC_INIT(tapi_input_device);
const tapi_input_device *d;

CHECK_RC(tapi_input_list(rpcs, &devices));
TE_VEC_FOREACH(&devices, d)
    RING("%s '%s' keys=%d abs=%d", d->node, d->name, d->n_keys, d->n_abs);
tapi_input_list_free(&devices);
```

Each device carries its node, name, bus/vendor/product/version, a
bitmask of supported `EV_*` types (`tapi_input_has_type()`), and the
number of key/abs/rel/switch codes. `tapi_input_capture()` collects the
events a device emits over a window; `tapi_input_absinfo()` reads the
range, fuzz, flat and resolution of each absolute axis.

## The library is linked, not a program

`ta_input` does not run `evtest` and parse its output. It links
libevdev and calls `libevdev_new_from_fd()`, `libevdev_has_event_code()`,
`libevdev_next_event()` and `libevdev_get_abs_info()` in the agent's RPC
server process, reading the device as libevdev's own structures. Across
the RPC a device/event/axis comes back as a tab-separated record, which
the engine side parses.

## Agent host requirements

- **libevdev** with its development headers (Debian:
  `apt install libevdev-dev`) and read access to `/dev/input/event*`
  (the `input` group, or root).
- Injection (`tapi_input_inject()`) additionally needs `/dev/uinput`
  (the `uinput` module loaded and writable).

## Usage

Declare the repository in an external libraries catalog and pass it to
`dispatcher.sh --external=<catalog.yml>`:

```yaml
repositories:
  - name: tsf_input
    url: https://github.com/interpretica-io/tsf-input.git
    ref: <tag>
    libs:
      - ta_input
      - rpcs_input
      - tapi_input
```

In `builder.conf`, bind `tapi_input` to the engine, list `ta_input` and
`rpcs_input` among the RPC server's libraries, and add the RPC
definitions to both platforms:

```
TE_EXT_REPO_USE([tsf_input], [ta_input rpcs_input], [tapi_input])

TE_LIB_PARMS([rpcxdr], [${TE_HOST}], [],
             [--with-rpcdefs=tarpc_job.x.m4,../ta_input/input_rpc.x.m4])
TE_LIB_PARMS([rpcxdr], [${TE_TA_TYPE}], [],
             [--with-rpcdefs=tarpc_job.x.m4,../ta_input/input_rpc.x.m4])
```

The RPC program number is **34** (20–33 are taken by the other tsf
agent RPCs); change it in `input_rpc.x.m4` if it ever collides.

## What was verified, and what was not

**Nothing was compiled here.** Unlike tsf-usb (whose libusb calls were
syntax-checked against the installed library), libevdev and the Linux
input uapi (`linux/input.h`, `uinput`) are Linux-only and are not
present on this macOS host, so `ta_input.c` could not be compiled or
syntax-checked. The libevdev calls were written against the documented
libevdev API (`libevdev_new_from_fd`, `libevdev_has_event_type/_code`,
`libevdev_event_type_get_max`, `libevdev_next_event`,
`libevdev_get_abs_info`, and `libevdev-uinput` for injection); the
engine side, RPCs and meson wiring follow the tsf-usb template unbuilt.
The first suite to build tsf-input should expect the ordinary
first-build fixes, and in particular confirm the `input_event`
timestamp accessors (`input_event_sec`/`input_event_usec`, the modern
spelling used here) against the agent's kernel headers.

## Scope

- **Read, mostly.** Listing, capture and absinfo only read. The one
  write path, `tapi_input_inject()`, creates a `uinput` device and
  injects events into the agent's system — a test calls it on purpose,
  never as a side effect of a read.
