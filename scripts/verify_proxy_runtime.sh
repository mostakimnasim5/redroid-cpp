#!/usr/bin/env bash
# ==============================================================================
# verify_proxy_runtime.sh — runtime verification of the proxy/network realism
# chain on a REAL host (WSL2 + binder kernel + Docker + ReDroid + a live proxy).
#
# This script is meant to be run by the USER on their own machine AFTER creating
# a phone with a proxy in the ReDroidCPP GUI. It was written statically and has
# NOT been executed against a live device by the author.
#
# Usage:
#   ./verify_proxy_runtime.sh <adb-serial> [wifi|cellular]
#
#   <adb-serial>   e.g. "localhost:5555" (what `adb devices` shows for the phone)
#   [mode]         "wifi"     = GUI mode 1 (ISP/residential proxy, no-SIM WiFi)
#                  "cellular" = GUI mode 2 (mobile proxy)   [default: cellular]
#
# Requirements on the host: adb in PATH, the ReDroid container booted
# (sys.boot_completed=1), and a proxy assigned (so ip-api.com is reachable).
#
# Exit code: 0 if all checks PASS, 1 otherwise.
# ==============================================================================

set -u  # unset-var is a bug here; do NOT use -e (we want to run all checks)

SERIAL="${1:-}"
MODE="${2:-cellular}"

if [ -z "$SERIAL" ]; then
    echo "Usage: $0 <adb-serial> [wifi|cellular]"
    exit 1
fi

ADB="adb -s ${SERIAL} shell"

PASS=0
FAIL=0

ok()   { PASS=$((PASS+1)); echo "  PASS  $1"; }
bad()  { FAIL=$((FAIL+1)); echo "  FAIL  $1"; }
info() { echo "  ....  $1"; }
hdr()  { echo ""; echo "== $1 =="; }

getprop() { ${ADB} getprop "$1" 2>/dev/null | tr -d '\r'; }

# ------------------------------------------------------------------------------
# 0. Sanity: device reachable + booted
# ------------------------------------------------------------------------------
hdr "0. Device sanity"
BOOT="$(getprop sys.boot_completed)"
if [ "$BOOT" = "1" ]; then ok "device booted (sys.boot_completed=1)"; else
    bad "device not booted (sys.boot_completed='$BOOT') — aborting"; exit 1;
fi

# ------------------------------------------------------------------------------
# 1. Exit IP / country inside the phone (used by checks 2 and 6)
# ------------------------------------------------------------------------------
hdr "1. Exit IP + country from inside the phone"
# ip-api.com is the same endpoint the C++ auto-sync uses (LocaleTimezoneManager).
EXIT_JSON="$(${ADB} curl -s --max-time 10 http://ip-api.com/json/ 2>/dev/null)"
EXIT_IP="$(echo "$EXIT_JSON"    | sed -n 's/.*"query":"\([^"]*\)".*/\1/p')"
EXIT_COUNTRY="$(echo "$EXIT_JSON" | sed -n 's/.*"countryCode":"\([^"]*\)".*/\1/p')"
if [ -n "$EXIT_IP" ]; then ok "exit IP resolved: ${EXIT_IP} (${EXIT_COUNTRY})"; else
    bad "could not resolve exit IP (proxy down or curl missing)"; fi

# Expected MCC for the exit country — mirrors COUNTRY_CARRIERS in
# src/Android/LocaleTimezoneManager.cpp (any row of the country is a valid pick).
mcc_for_country() {
    case "$1" in
        US) echo "310 311 312 313" ;; GB) echo "234" ;; DE) echo "262" ;;
        FR) echo "208" ;; JP) echo "440" ;; KR) echo "450" ;; IN) echo "404 405" ;;
        BD) echo "470" ;; CN) echo "460" ;; AU) echo "505" ;; CA) echo "302" ;;
        BR) echo "724" ;; RU) echo "250" ;; AE) echo "424" ;; SG) echo "525" ;;
        PK) echo "410" ;; SA) echo "420" ;; MX) echo "334" ;; IT) echo "222" ;;
        ES) echo "214" ;; NL) echo "204" ;;
        *) echo "" ;;
    esac
}

# ------------------------------------------------------------------------------
# 2. Carrier / SIM props match the proxy country (Cellular) or absent (WiFi)
# ------------------------------------------------------------------------------
hdr "2. SIM / carrier props"
OPER_NUM="$(getprop gsm.operator.numeric)"
SIM_NUM="$(getprop gsm.sim.operator.numeric)"
SIM_STATE="$(getprop gsm.sim.state)"

if [ "$MODE" = "wifi" ]; then
    # WiFi mode: no-SIM story — operator props must be EMPTY, SIM ABSENT.
    [ -z "$OPER_NUM" ] && ok "gsm.operator.numeric empty (no-SIM)" || bad "gsm.operator.numeric='$OPER_NUM' (expected empty in wifi mode)"
    [ -z "$SIM_NUM" ]  && ok "gsm.sim.operator.numeric empty"      || bad "gsm.sim.operator.numeric='$SIM_NUM' (expected empty)"
    [ "$SIM_STATE" = "ABSENT" ] && ok "gsm.sim.state ABSENT" || bad "gsm.sim.state='$SIM_STATE' (expected ABSENT)"
else
    # Cellular mode: numeric must start with a valid MCC for the exit country.
    VALID_MCCS="$(mcc_for_country "$EXIT_COUNTRY")"
    MCC3="${OPER_NUM:0:3}"
    MATCH="no"
    for m in $VALID_MCCS; do [ "$MCC3" = "$m" ] && MATCH="yes"; done
    if [ "$MATCH" = "yes" ]; then
        ok "gsm.operator.numeric=$OPER_NUM matches country $EXIT_COUNTRY (MCC $MCC3)"
    else
        bad "gsm.operator.numeric='$OPER_NUM' does not match country '$EXIT_COUNTRY' (valid MCCs: ${VALID_MCCS:-unknown})"
    fi
    [ -n "$SIM_NUM" ] && ok "gsm.sim.operator.numeric=$SIM_NUM set" || bad "gsm.sim.operator.numeric empty (expected set in cellular mode)"
fi

# ------------------------------------------------------------------------------
# 3. IP consistency: net.rWbcmLe.localip == CELLULAR_IP == WebRTC local IP
# ------------------------------------------------------------------------------
hdr "3. IP consistency (WebRTC / cellular / local)"
LOCALIP="$(getprop net.rWbcmLe.localip)"
RMNET_IP="$(${ADB} ip addr show rmnet0 2>/dev/null | sed -n 's/.*inet \([0-9.]*\).*/\1/p' | head -1)"
# The authoritative source is the deterministic profile IP stored in the
# container env at boot; read it back if visible, else compare the two props.
CONTAINER_IP="$(${ADB} printenv CELLULAR_IP 2>/dev/null | tr -d '\r')"

info "net.rWbcmLe.localip = '${LOCALIP}'"
info "rmnet0 inet        = '${RMNET_IP}'"
info "CELLULAR_IP (env)  = '${CONTAINER_IP:-<not visible>}'"

if [ -n "$LOCALIP" ] && [ "$LOCALIP" = "$RMNET_IP" ]; then
    ok "localip prop == rmnet0 address ($LOCALIP)"
else
    bad "localip ('$LOCALIP') != rmnet0 ('$RMNET_IP')"
fi
if [ -n "$CONTAINER_IP" ]; then
    [ "$CONTAINER_IP" = "$LOCALIP" ] && ok "CELLULAR_IP env == localip ($CONTAINER_IP)" \
        || bad "CELLULAR_IP env ('$CONTAINER_IP') != localip ('$LOCALIP')"
else
    info "CELLULAR_IP env not visible via adb (normal on some boots) — skipped strict env compare"
fi

# ------------------------------------------------------------------------------
# 4. gsm.cell.id deterministic across a reboot
# ------------------------------------------------------------------------------
hdr "4. gsm.cell.id stability across reboot"
CELL1="$(getprop gsm.cell.id)"
info "current gsm.cell.id = '$CELL1' (reboot the phone, re-run, compare)"
if [ -n "$CELL1" ]; then
    STATE_FILE="/tmp/.vpp_cellid_${SERIAL//[:\/]/_}"
    if [ -f "$STATE_FILE" ]; then
        CELL_PREV="$(cat "$STATE_FILE")"
        [ "$CELL_PREV" = "$CELL1" ] && ok "gsm.cell.id stable across reboot ($CELL1)" \
            || bad "gsm.cell.id changed: $CELL_PREV -> $CELL1 (must be deterministic)"
    else
        echo "$CELL1" > "$STATE_FILE"
        info "first run — recorded gsm.cell.id; reboot and re-run to confirm stability"
    fi
else
    [ "$MODE" = "wifi" ] && ok "gsm.cell.id unset (acceptable in wifi mode)" \
        || bad "gsm.cell.id empty in cellular mode"
fi

# ------------------------------------------------------------------------------
# 5. WiFi transport present (wifi mode)
# ------------------------------------------------------------------------------
hdr "5. Transport"
if [ "$MODE" = "wifi" ]; then
    WIFI_ON="$(${ADB} settings get global wifi_on 2>/dev/null | tr -d '\r')"
    MOBILE_DATA="$(${ADB} settings get global mobile_data 2>/dev/null | tr -d '\r')"
    [ "$WIFI_ON" = "1" ] && ok "wifi_on=1" || bad "wifi_on='$WIFI_ON' (expected 1)"
    [ "$MOBILE_DATA" = "0" ] && ok "mobile_data=0" || bad "mobile_data='$MOBILE_DATA' (expected 0)"
else
    info "cellular mode — transport check is the rmnet0 compare in section 3"
fi

# ------------------------------------------------------------------------------
# 6. Exit IP sanity (public, i.e. proxy actually routing)
# ------------------------------------------------------------------------------
hdr "6. Exit IP sanity"
if [ -n "$EXIT_IP" ]; then
    case "$EXIT_IP" in
        10.*|192.168.*|172.1[6-9].*|172.2[0-9].*|172.3[0-1].*|127.*)
            bad "exit IP '$EXIT_IP' is private — proxy NOT routing (traffic going direct)" ;;
        *)
            ok "exit IP '$EXIT_IP' is public (proxy routing)" ;;
    esac
else
    bad "no exit IP to evaluate"
fi

# ------------------------------------------------------------------------------
# Summary
# ------------------------------------------------------------------------------
echo ""
echo "=================================================="
echo "  RESULT: ${PASS} PASS, ${FAIL} FAIL  (serial=${SERIAL}, mode=${MODE})"
echo "=================================================="
[ "$FAIL" -eq 0 ] && exit 0 || exit 1
