#!/usr/bin/env bash
#
# mayhem/build.sh — builds the CherryUSB fuzz harnesses (sanitized, DWARF 3) with their standalone
# reproducers, and the behavioral self-test that mayhem/test.sh runs (project's normal flags).
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${RUST_DEBUG_FLAGS:=}" ; : "${GO_DEBUG_FLAGS:=}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

cd "$SRC"

# mayhem/ comes first so its usb_config.h and usb_log.h (aborting USB_ASSERT) shadow the upstream ones.
INC="-Imayhem -Icore -Icommon -Iclass/hid -Iclass/msc -Iclass/serial -Iclass/cdc -Iclass/hub"
HOST_SRCS="core/usbh_core.c class/msc/usbh_msc.c class/hid/usbh_hid.c class/serial/usbh_serial.c class/serial/usbh_cdc_acm.c class/hub/usbh_hub.c mayhem/osal_fuzz.c mayhem/fake_hc.c"
DEV_SRCS="core/usbd_core.c class/cdc/usbd_cdc_acm.c class/msc/usbd_msc.c class/hid/usbd_hid.c mayhem/osal_fuzz.c mayhem/fake_dc.c"

# objs <dir> <sources...>: object path for each source, flattened into <dir>
objs() { local d="$1"; shift; for f in "$@"; do printf '%s ' "$d/${f//\//_}.o"; done; }

# compile <dir> <flags> <sources...>
compile() {
  local d="$1" flags="$2"; shift 2
  mkdir -p "$d"
  for f in "$@"; do
    [ -f "$d/${f//\//_}.o" ] || $CC $flags -w -c "$f" $INC -o "$d/${f//\//_}.o"
  done
}

ALL_SRCS=$(printf '%s\n' $HOST_SRCS $DEV_SRCS | sort -u | tr '\n' ' ')

# 1) the fuzzed project: sanitized, DWARF 3
SAN="$SANITIZER_FLAGS $DEBUG_FLAGS"
rm -rf mayhem/build
# the libFuzzer flavour carries SanCov (-fsanitize=fuzzer-no-link) so the project code is covered, not just the harness;
# the standalone flavour has no fuzzer runtime to feed SanCov to
compile mayhem/build/san "$SAN -fsanitize=fuzzer-no-link" $ALL_SRCS
compile mayhem/build/standalone "$SAN" $ALL_SRCS
$CXX $SAN -c mayhem/lsan_off.cc -o mayhem/build/san/lsan_off.o
$CC $SAN -c "$STANDALONE_FUZZ_MAIN" -o mayhem/build/san/standalone_main.o

# link_target <name> <harness> <sources...>: the harness is compiled once per flavour alongside its project sources
link_target() {
  local name="$1" src="$2"; shift 2
  $CC $SAN -fsanitize=fuzzer-no-link $LIB_FUZZING_ENGINE $INC "$src" $(objs mayhem/build/san "$@") mayhem/build/san/lsan_off.o -o "/mayhem/$name"
  $CC $SAN $INC "$src" mayhem/build/san/standalone_main.o $(objs mayhem/build/standalone "$@") mayhem/build/san/lsan_off.o -o "/mayhem/$name-standalone"
}

link_target fuzz_desc_check mayhem/fuzz_desc_check.c
link_target fuzz_usbh_enumerate mayhem/fuzz_usbh_enumerate.c $HOST_SRCS
link_target fuzz_usbd_core mayhem/fuzz_usbd_core.c $DEV_SRCS

# 2) the oracle: self-test against the same sources, normal flags, no sanitizers
PLAIN="-O1 $COVERAGE_FLAGS"
compile mayhem/build/plain "$PLAIN" $ALL_SRCS
$CC $PLAIN $INC mayhem/selftest.c \
  $(objs mayhem/build/plain core/usbh_core.c class/msc/usbh_msc.c class/hid/usbh_hid.c class/serial/usbh_serial.c class/serial/usbh_cdc_acm.c class/hub/usbh_hub.c mayhem/fake_hc.c) \
  $(objs mayhem/build/plain core/usbd_core.c class/cdc/usbd_cdc_acm.c class/msc/usbd_msc.c class/hid/usbd_hid.c mayhem/fake_dc.c) \
  mayhem/build/plain/mayhem_osal_fuzz.c.o -o /mayhem/cherryusb_selftest
