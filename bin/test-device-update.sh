#!/usr/bin/env bash
# Pin device-update.sh exit-status propagation and argument handling with a fake Python/esptool.
# Flash and mode-change failures must not report success, and invalid input must never invoke a write.
set -euo pipefail

ROOT_DIR=$(cd "$(dirname "$0")/.." && pwd)
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

mkdir -p "$WORK/tools with spaces"
FAKE_PYTHON="$WORK/tools with spaces/python"
export MOCK_ESPTOOL_ARGS="$WORK/args"
export MOCK_ESPTOOL_STATUS=0
export MOCK_ESPTOOL_STYLE=dash
cat >"$FAKE_PYTHON" <<'PYTHON'
#!/usr/bin/env bash
[[ "$1" == -m && "$2" == esptool ]] || exit 99
shift 2
if [[ "$1" == version ]]; then
    exit 0
fi
if [[ "$1" == --help ]]; then
    if [[ "$MOCK_ESPTOOL_STYLE" == dash ]]; then
        echo 'write-flash read-flash-status'
    else
        echo 'write_flash read_flash_status'
    fi
    exit 0
fi
printf '%s\n' "$@" >"$MOCK_ESPTOOL_ARGS"
exit "$MOCK_ESPTOOL_STATUS"
PYTHON
chmod +x "$FAKE_PYTHON"
FIRMWARE="$WORK/firmware with spaces.bin"
touch "$FIRMWARE" "$WORK/firmware.factory.bin"

run_case() {
	local name="$1" expected="$2" actual=0
	shift 2
	rm -f "$MOCK_ESPTOOL_ARGS"
	"$ROOT_DIR/bin/device-update.sh" -P "$FAKE_PYTHON" "$@" >"$WORK/output" 2>&1 || actual=$?
	if [[ $actual != "$expected" ]]; then
		echo "FAIL $name: expected exit $expected, got $actual" >&2
		cat "$WORK/output" >&2
		exit 1
	fi
	echo "PASS $name"
}

run_case 'successful update' 0 -p '/dev/serial/port with spaces' -f "$FIRMWARE"
printf '%s\n' --port '/dev/serial/port with spaces' --baud 115200 write-flash 0x10000 "$FIRMWARE" >"$WORK/expected"
cmp "$WORK/expected" "$MOCK_ESPTOOL_ARGS"

MOCK_ESPTOOL_STATUS=7
run_case 'failed update preserves status' 7 "$FIRMWARE"
run_case 'failed mode change preserves status' 7 --change-mode

MOCK_ESPTOOL_STATUS=0
run_case 'successful mode change' 0 --change-mode
printf '%s\n' --baud 1200 --after no_reset read-flash-status >"$WORK/expected"
cmp "$WORK/expected" "$MOCK_ESPTOOL_ARGS"

MOCK_ESPTOOL_STYLE=underscore
run_case 'legacy esptool commands' 0 "$FIRMWARE"
printf '%s\n' --baud 115200 write_flash 0x10000 "$FIRMWARE" >"$WORK/expected"
cmp "$WORK/expected" "$MOCK_ESPTOOL_ARGS"

run_case 'missing firmware' 1 -f "$WORK/missing.bin"
[[ ! -e $MOCK_ESPTOOL_ARGS ]]
run_case 'factory image rejected' 1 -f "$WORK/firmware.factory.bin"
[[ ! -e $MOCK_ESPTOOL_ARGS ]]
run_case 'missing filename' 1
[[ ! -e $MOCK_ESPTOOL_ARGS ]]
run_case 'help' 0 -h
[[ ! -e $MOCK_ESPTOOL_ARGS ]]
