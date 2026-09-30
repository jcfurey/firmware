#!/usr/bin/env bash

PYTHON=${PYTHON:-$(command -v python3 || command -v python)}
CHANGE_MODE=false
UPLOAD_PORT=""
FILENAME=${FILENAME:-}

# Constants
FLASH_BAUD=115200
RESET_BAUD=1200
UPDATE_OFFSET=0x10000

# Usage info
show_help() {
	cat <<EOF
Usage: $(basename "$0") [-h] [-p ESPTOOL_PORT] [-P PYTHON] [-f FILENAME|FILENAME] [--change-mode]
Flash image file to device, leave existing system intact.

    -h               Display this help and exit
    -p ESPTOOL_PORT  Set the environment variable for ESPTOOL_PORT.  If not set, ESPTOOL iterates all ports (Dangerous).
    -P PYTHON        Specify alternate python interpreter to use to invoke esptool. (Default: "$PYTHON")
    -f FILENAME      The *.bin file to flash.  Custom to your device type.
    --change-mode    Attempt to place the device in correct mode. Some hardware requires this twice. (1200bps Reset)

EOF
}

# Check for --change-mode and remove it from arguments
NEW_ARGS=()
for arg in "$@"; do
	if [[ $arg == "--change-mode" ]]; then
		CHANGE_MODE=true
	else
		NEW_ARGS+=("$arg")
	fi
done

set -- "${NEW_ARGS[@]}"

while getopts ":hp:P:f:" opt; do
	case "${opt}" in
	h)
		show_help
		exit 0
		;;
	p)
		UPLOAD_PORT=${OPTARG}
		;;
	P)
		PYTHON=${OPTARG}
		;;
	f)
		FILENAME=${OPTARG}
		;;
	*)
		echo "Invalid flag."
		show_help >&2
		exit 1
		;;
	esac
done
shift "$((OPTIND - 1))"

if [[ -z $FILENAME && $# -gt 0 ]]; then
	FILENAME="$1"
	shift
fi

if [[ $CHANGE_MODE != true && (! -f $FILENAME || $FILENAME == *.factory.bin) ]]; then
	show_help >&2
	echo "Invalid file: ${FILENAME}" >&2
	exit 1
fi

# Select the interpreter after parsing -P, and preserve each argument without word splitting.
if "$PYTHON" -m esptool version >/dev/null 2>&1; then
	ESPTOOL_CMD=("$PYTHON" -m esptool)
elif command -v esptool >/dev/null 2>&1; then
	ESPTOOL_CMD=(esptool)
elif command -v esptool.py >/dev/null 2>&1; then
	ESPTOOL_CMD=(esptool.py)
else
	echo "Error: esptool not found" >&2
	exit 1
fi

ESPTOOL_HELP=$("${ESPTOOL_CMD[@]}" --help)
if [[ $ESPTOOL_HELP == *write-flash* ]]; then
	ESPTOOL_WRITE_FLASH=write-flash
	ESPTOOL_READ_FLASH_STATUS=read-flash-status
else
	ESPTOOL_WRITE_FLASH=write_flash
	ESPTOOL_READ_FLASH_STATUS=read_flash_status
fi

if [[ -n $UPLOAD_PORT ]]; then
	ESPTOOL_CMD+=(--port "$UPLOAD_PORT")
fi

if [[ $CHANGE_MODE == true ]]; then
	exec "${ESPTOOL_CMD[@]}" --baud "$RESET_BAUD" --after no_reset "$ESPTOOL_READ_FLASH_STATUS"
fi

echo "Trying to flash update ${FILENAME}"
exec "${ESPTOOL_CMD[@]}" --baud "$FLASH_BAUD" "$ESPTOOL_WRITE_FLASH" "$UPDATE_OFFSET" "$FILENAME"
