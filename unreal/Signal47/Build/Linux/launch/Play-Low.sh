#!/bin/sh
set -eu
APP_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec "$APP_DIR/Play.sh" -S47Title -S47Quality=low -S47Picture=off -windowed -ResX=1280 -ResY=720 "$@"
