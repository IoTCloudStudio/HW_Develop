#!/bin/sh
# ============================================================================
# install-offline.sh - Migracion ZTE MF193 a 3G por QMI (Claro / otro operador)
#
# 100% offline. Corre en el router (OpenWrt 22.03, ramips/mt76x8).
# Lo lanza instalar.ps1 desde la PC del tecnico. Parametros por env:
#   APN (default igprs.claro.com.ar), CARRIER_HINT
#
# El MF193 arranca en modo 'factory_test' y NO conecta por PPP. El unico
# camino confiable (probado en vivo: IP de Claro + ping OK) es QMI:
#   uqmi --set-device-operating-mode online  -> registra en Claro
#   uqmi --start-network ...                 -> sesion de datos + wwan0
#   interfaz 'wan3g' proto dhcp device wwan0 -> netifd hace DHCP/ruta/DNS
# Persistencia: /usr/sbin/mf193-qmi (connect en boot via rc.local + watchdog
# por cron cada 4 min; si no hay internet 3 veces seguidas -> reboot).
#
# --------------------------------------------------------------------------
# DISEÑO POR ETAPAS (idempotente / resumible)
# --------------------------------------------------------------------------
# Cada etapa tiene un chequeo (etapa_X_check) que mira el ESTADO REAL del
# router, no una bandera. Si el chequeo ya pasa, la etapa se marca OK y NO
# se repite el trabajo. Si no pasa, se corre el arreglo (etapa_X_fix) y se
# vuelve a chequear. Por esto, correr este script 2 veces (router reiniciado
# a mitad de camino, tecnico que ejecuta INSTALAR.bat de nuevo, etc.) nunca
# rehace lo que ya quedo bien - solo retoma desde la primera etapa que
# todavia no este validada.
#
# Un flock global (fd 8) evita que dos instalaciones corran en paralelo en el
# mismo router (dos ventanas del instalador, o el tecnico que lo vuelve a
# ejecutar mientras la fase de red anterior todavia esta terminando en
# segundo plano). El lock se mantiene tomado durante toda la fase 2 porque el
# proceso en background hereda el descriptor abierto.
#
# Al final, finalizar_validacion() vuelve a chequear TODAS las etapas de
# cero (sin asumir nada de lo que paso durante el "fix") y escribe en
# /tmp/migracion-3g.resultado:
#   linea 1: OK | FAIL | YA_CORRIENDO
#   lineas siguientes: "<etapa>=<estado>" para que el instalador le diga al
#   tecnico exactamente que falta, en vez de un OK/FAIL generico.
# ============================================================================
PKGDIR="$(cd "$(dirname "$0")" && pwd)"
APN="${APN:-igprs.claro.com.ar}"
CARRIER_HINT="${CARRIER_HINT:-CLARO}"

LOCKFILE=/var/run/migracion-mf193.lock
RES=/tmp/migracion-3g.resultado
LOG=/tmp/migracion-3g.log

# --- evitar dos instalaciones/migraciones en paralelo en el mismo router ---
exec 8>"$LOCKFILE"
if ! flock -n 8; then
    echo "YA_CORRIENDO" > "$RES"
    echo "Ya hay una instalacion en curso en este router (otra ventana del instalador, u otro tecnico). Espere unos minutos y vuelva a intentar (VER-RESULTADO.bat)."
    exit 3
fi
# A partir de aca el lock (fd 8) queda tomado por este proceso Y por el hijo
# en background que lanzamos mas abajo para la fase de red: hereda el fd
# abierto, asi que el lock sigue vivo hasta que la fase de red termine de
# verdad, no solo hasta que este script "sh install-offline.sh" retorne.

rm -f "$RES"
: > "$LOG"
exec >> "$LOG" 2>&1
echo "== $(date) :: inicio instalador (staged) - operador esperado $CARRIER_HINT (APN=$APN) =="

ETAPAS=""
marcar() { ETAPAS="$ETAPAS
$1=$2"; }

# Corre una etapa: si el chequeo ya pasa no hace nada (resume-from-there).
# Si no pasa, aplica el fix y vuelve a chequear. Devuelve 0/1 y deja marcado
# el resultado via marcar().
run_etapa() {
    nombre="$1"; label="$2"
    if eval "etapa_${nombre}_check"; then
        echo "[$label] ya estaba OK - no se repite"
        marcar "$nombre" "OK"
        return 0
    fi
    echo "[$label] aplicando..."
    eval "etapa_${nombre}_fix"
    if eval "etapa_${nombre}_check"; then
        echo "[$label] OK"
        marcar "$nombre" "OK"
        return 0
    fi
    echo "[$label] FALLA"
    marcar "$nombre" "FALLA"
    return 1
}

# ----------------------------------------------------------------------
# Etapa: paquetes QMI instalados (kmods + uqmi + proto qmi.sh)
# ----------------------------------------------------------------------
etapa_paquetes_check() {
    for f in kmod-mii kmod-usb-wdm kmod-usb-net kmod-usb-net-qmi-wwan librt libusb-1.0-0 usb-modeswitch; do
        opkg list-installed 2>/dev/null | grep -q "^$f " || return 1
    done
    [ -x /sbin/uqmi ] || return 1
    [ -s /lib/netifd/proto/qmi.sh ] || return 1
    return 0
}
etapa_paquetes_fix() {
    need() { [ -e "$PKGDIR"/$1 ] || { echo "FALTA $1 en $PKGDIR - carpeta incompleta"; return 1; }; }
    need "kmod-mii_*.ipk" || return 1
    need "kmod-usb-wdm_*.ipk" || return 1
    need "kmod-usb-net_*.ipk" || return 1
    need "kmod-usb-net-qmi-wwan_*.ipk" || return 1
    need "librt_*.ipk" || return 1
    need "libusb-1.0-0_*.ipk" || return 1
    need "usb-modeswitch_*.ipk" || return 1
    for f in uqmi qmi.sh mf193-qmi; do
        [ -s "$PKGDIR/$f" ] || { echo "FALTA $f en $PKGDIR - carpeta incompleta"; return 1; }
    done

    echo "== liberando espacio en flash =="
    for p in ppp-mod-pppoe kmod-pppoe kmod-pppox luci-ssl px5g-wolfssl kmod-usb-ledtrig-usbport; do
        opkg remove "$p" >/dev/null 2>&1 && echo "  saque $p" || true
    done
    opkg remove --force-depends luci-proto-ipv6 >/dev/null 2>&1 && echo "  saque luci-proto-ipv6" || true

    echo "== instalando modulos QMI =="
    opkg install "$PKGDIR"/kmod-mii_*.ipk "$PKGDIR"/kmod-usb-wdm_*.ipk "$PKGDIR"/kmod-usb-net_*.ipk "$PKGDIR"/kmod-usb-net-qmi-wwan_*.ipk
    mkdir -p /lib/netifd/proto
    cp "$PKGDIR/uqmi" /sbin/uqmi; chmod 755 /sbin/uqmi
    cp "$PKGDIR/qmi.sh" /lib/netifd/proto/qmi.sh; chmod 755 /lib/netifd/proto/qmi.sh
    # libusb-1.0-0 es dependencia de usb-modeswitch, y a su vez depende de librt.
    # Algunas imagenes de fabrica (o un router despues de un reset de fabrica)
    # no las traen, y opkg no puede resolverlas solo porque estamos offline -
    # por eso van empaquetadas aparte, en el orden correcto de dependencias.
    opkg install "$PKGDIR"/librt_*.ipk
    opkg install "$PKGDIR"/libusb-1.0-0_*.ipk
    opkg install "$PKGDIR"/usb-modeswitch_*.ipk
}

# ----------------------------------------------------------------------
# Etapa: script mf193-qmi instalado, version con lock (evita 2 instancias
# concurrentes peleando por el mismo modem - el bug que causaba cuelgues).
# ----------------------------------------------------------------------
etapa_script_check() {
    [ -x /usr/sbin/mf193-qmi ] && grep -q flock /usr/sbin/mf193-qmi
}
etapa_script_fix() {
    [ -s "$PKGDIR/mf193-qmi" ] || { echo "FALTA mf193-qmi en $PKGDIR"; return 1; }
    sh -n "$PKGDIR/mf193-qmi" || { echo "mf193-qmi tiene error de sintaxis, no se instala"; return 1; }
    cp "$PKGDIR/mf193-qmi" /usr/sbin/mf193-qmi
    chmod 755 /usr/sbin/mf193-qmi

    cat > /etc/hotplug.d/usb/01-zte-qmi-online << 'HOTPLUG'
#!/bin/sh
[ "$DEVICENAME" = "cdc-wdm0" ] || exit 0
[ "$ACTION" = "add" ] || exit 0
exec 9>/var/run/zte-qmi-online.lock; flock -n 9 || exit 0
sleep 12
i=0; while [ $i -lt 15 ]; do
    uqmi -d /dev/cdc-wdm0 --set-device-operating-mode online >/dev/null 2>&1
    case "$(uqmi -d /dev/cdc-wdm0 --get-device-operating-mode 2>/dev/null)" in
        *online*) exit 0 ;;
    esac
    i=$((i+1)); sleep 2
done
HOTPLUG
    chmod +x /etc/hotplug.d/usb/01-zte-qmi-online
}

# ----------------------------------------------------------------------
# Etapa: autostart (rc.local + cron) presente exactamente una vez, sin
# duplicar lineas si el instalador corrio antes en este mismo router.
# ----------------------------------------------------------------------
etapa_autostart_check() {
    rc_n=$(grep -c 'mf193-qmi connect' /etc/rc.local 2>/dev/null)
    cron_n=$(crontab -l 2>/dev/null | grep -c 'mf193-qmi watch')
    [ "${rc_n:-0}" = "1" ] && [ "${cron_n:-0}" = "1" ]
}
etapa_autostart_fix() {
    if [ -e /etc/rc.local ]; then
        grep -v 'mf193-qmi connect' /etc/rc.local > /tmp/rc.local.nuevo 2>/dev/null
    else
        : > /tmp/rc.local.nuevo
    fi
    sed -i '/^exit 0/d' /tmp/rc.local.nuevo 2>/dev/null
    printf '( sleep 30; for i in 1 2 3; do /usr/sbin/mf193-qmi connect && break; sleep 20; done ) &\nexit 0\n' >> /tmp/rc.local.nuevo
    cat /tmp/rc.local.nuevo > /etc/rc.local
    rm -f /tmp/rc.local.nuevo

    ( crontab -l 2>/dev/null | grep -v 'mf193-qmi watch'; echo '*/4 * * * * /usr/sbin/mf193-qmi watch' ) | crontab -
    /etc/init.d/cron enable >/dev/null 2>&1
    /etc/init.d/cron restart >/dev/null 2>&1
}

# ----------------------------------------------------------------------
# Etapa: SSH accesible desde wan/VPN (soporte remoto). Por defecto OpenWrt
# solo acepta SSH desde lan - sin esta regla, un router solo se puede
# administrar estando fisicamente ahi. La web (uhttpd) NO se toca aca: debe
# seguir sin responder desde wan salvo que alguien la habilite a mano.
# ----------------------------------------------------------------------
etapa_sshwan_check() {
    uci show firewall 2>/dev/null | grep -q "\.dest_port='22'" && \
    uci show firewall 2>/dev/null | grep -B4 "\.dest_port='22'" | grep -q "\.src='wan'"
}
etapa_sshwan_fix() {
    uci add firewall rule
    uci set firewall.@rule[-1].name='Allow-SSH-WAN'
    uci set firewall.@rule[-1].src='wan'
    uci set firewall.@rule[-1].proto='tcp'
    uci set firewall.@rule[-1].dest_port='22'
    uci set firewall.@rule[-1].target='ACCEPT'
    uci commit firewall
    /etc/init.d/firewall reload
}

echo "== Fase 1: paquetes / script / autostart / ssh-wan (no toca la conexion) =="
df -h /overlay | tail -1

fase1_ok=1
run_etapa paquetes  "paquetes QMI"      || fase1_ok=0
if [ "$fase1_ok" = 1 ]; then
    run_etapa script     "script mf193-qmi" || true
    run_etapa autostart  "autostart"        || true
    run_etapa sshwan     "SSH desde wan"    || true
fi

if [ "$fase1_ok" = 0 ]; then
    echo "=================================================================="
    echo " RESULTADO: FALLO - no se pudieron instalar los paquetes QMI"
    echo "=================================================================="
    { echo "FAIL"; echo "$ETAPAS"; } > "$RES"
    exit 1
fi

# --- backup config (una sola vez, nunca se pisa) ---
BACKUP="/etc/config/network.original-antes-de-instalar"
[ -e "$BACKUP" ] || cp /etc/config/network "$BACKUP" 2>/dev/null
echo "== Backup config: $BACKUP =="

echo "== Esperando modo modem (idProduct 0124, hasta 90s) =="
i=0; while [ $i -lt 30 ]; do
    grep -qx 0124 /sys/bus/usb/devices/*/idProduct 2>/dev/null && break
    i=$((i+1)); sleep 3
done

# ----------------------------------------------------------------------
# Fase 2: DESACOPLADA del SSH (sobrevive al corte). Hereda el flock de fd 8,
# asi que mientras siga corriendo, una segunda ejecucion del instalador se
# encuentra con "YA_CORRIENDO" en vez de arrancar una migracion en paralelo.
# ----------------------------------------------------------------------
P2=/tmp/migracion-3g-fase2.sh
{
    echo "#!/bin/sh"
    echo "APN='$APN'"
    echo "CARRIER_HINT='$CARRIER_HINT'"
    echo "BACKUP='$BACKUP'"
    echo "RES='$RES'"
    echo "LOG='$LOG'"
    echo "ETAPAS_FASE1='$ETAPAS'"
} > "$P2"
cat >> "$P2" << 'FASE2'
exec >> "$LOG" 2>&1
echo "== $(date) :: fase 2 - red / conexion / vpn =="

ETAPAS="$ETAPAS_FASE1"
marcar() { ETAPAS="$ETAPAS
$1=$2"; }

D=/dev/cdc-wdm0
uq(){ s="$1"; shift; uqmi -d "$D" "$@" >/tmp/uq.$$ 2>&1 & p=$!; ( sleep "$s"; kill -9 $p 2>/dev/null )& w=$!; wait $p 2>/dev/null; kill $w 2>/dev/null; cat /tmp/uq.$$ 2>/dev/null; rm -f /tmp/uq.$$; }
online_wwan() { ping -I wwan0 -c 2 -W 4 8.8.8.8 >/dev/null 2>&1; }

# Esta fase corre como proceso aparte (no hereda las funciones de la fase 1),
# asi que para que la validacion final sea un chequeo de verdad independiente
# -y no solo "confiar" en lo que dijo la fase 1- repetimos aca los mismos
# chequeos (sin los fix, esos ya corrieron en la fase 1).
etapa_paquetes_check() {
    for f in kmod-mii kmod-usb-wdm kmod-usb-net kmod-usb-net-qmi-wwan librt libusb-1.0-0 usb-modeswitch; do
        opkg list-installed 2>/dev/null | grep -q "^$f " || return 1
    done
    [ -x /sbin/uqmi ] || return 1
    [ -s /lib/netifd/proto/qmi.sh ] || return 1
    return 0
}
etapa_script_check() {
    [ -x /usr/sbin/mf193-qmi ] && grep -q flock /usr/sbin/mf193-qmi
}
etapa_autostart_check() {
    rc_n=$(grep -c 'mf193-qmi connect' /etc/rc.local 2>/dev/null)
    cron_n=$(crontab -l 2>/dev/null | grep -c 'mf193-qmi watch')
    [ "${rc_n:-0}" = "1" ] && [ "${cron_n:-0}" = "1" ]
}
etapa_sshwan_check() {
    uci show firewall 2>/dev/null | grep -q "\.dest_port='22'" && \
    uci show firewall 2>/dev/null | grep -B4 "\.dest_port='22'" | grep -q "\.src='wan'"
}

dump(){
    echo "--- op-mode  : $(uq 8 --get-device-operating-mode | tr -d '\n\t ')"
    echo "--- serving  : $(uq 8 --get-serving-system | tr -d '\n\t ')"
    echo "--- data     : $(uq 8 --get-data-status | tr -d '\n\t ')"
    echo "--- pin      : $(uq 8 --get-pin-status | tr -d '\n\t ')"
    echo "--- wan3g    :"; ifstatus wan3g 2>&1
    echo "--- wwan0    :"; ifconfig wwan0 2>&1 | grep -E 'inet|UP|RX pack'
    echo "--- ruta     :"; ip route 2>&1
    echo "--- usb      :"; for x in /sys/bus/usb/devices/*/idVendor; do [ -e "$x" ] && printf '  %s:%s\n' "$(cat "$x")" "$(cat "${x%idVendor}idProduct")"; done
    echo "--- logread  :"; logread 2>/dev/null | grep -Ei 'mf193|qmi|wwan|wan3g|usb|option' | tail -70
    echo "--- dmesg    :"; dmesg 2>/dev/null | tail -40
    echo "Volver atras: cp $BACKUP /etc/config/network && reboot"
}

run_etapa() {
    nombre="$1"; label="$2"
    if eval "etapa_${nombre}_check"; then
        echo "[$label] ya estaba OK - no se repite"
        marcar "$nombre" "OK"
        return 0
    fi
    echo "[$label] aplicando..."
    eval "etapa_${nombre}_fix"
    if eval "etapa_${nombre}_check"; then
        echo "[$label] OK"
        marcar "$nombre" "OK"
        return 0
    fi
    echo "[$label] FALLA"
    marcar "$nombre" "FALLA"
    return 1
}

# esperar cdc-wdm0 antes de evaluar nada de red
if [ ! -c "$D" ]; then
    echo "== esperando /dev/cdc-wdm0 (hasta 60s) =="
    n=0; while [ $n -lt 20 ] && [ ! -c "$D" ]; do n=$((n+1)); sleep 3; done
fi
if [ ! -c "$D" ]; then
    echo "=================================================================="
    echo " RESULTADO: FALLO - NO APARECE /dev/cdc-wdm0"
    echo "=================================================================="
    echo ">> El driver qmi_wwan no bindeo al modem. Revisar conexion USB / cable."
    dump
    marcar "red" "FALLA"
    { echo "FAIL"; echo "$ETAPAS"; } > "$RES"
    exit 0
fi

# ----------------------------------------------------------------------
# Etapa: interfaz de red migrada. Detecta CUALQUIER interfaz legacy con
# proto=3g por su VALOR, no por nombre fijo (en la practica aparecio como
# "3GWAN", "3gWAN", "WAN3g" segun el equipo) - asi no hace falta tocar este
# script cada vez que aparece una convencion de nombre distinta.
# ----------------------------------------------------------------------
legacy_3g_ifaces() {
    uci show network 2>/dev/null | sed -n "s/^network\.\([^.]*\)\.proto='3g'$/\1/p"
}
etapa_red_check() {
    [ -z "$(legacy_3g_ifaces)" ] || return 1
    [ "$(uci -q get network.wan3g.proto 2>/dev/null)" = "dhcp" ] || return 1
    [ "$(uci -q get network.wan3g.device 2>/dev/null)" = "wwan0" ] || return 1
    return 0
}
etapa_red_fix() {
    legacy="$(legacy_3g_ifaces)"
    for ifc in $legacy; do ifdown "$ifc" >/dev/null 2>&1; done
    killall pppd chat comgt 2>/dev/null

    z=$(uci show firewall | sed -n "s/^firewall\.\(@zone\[[0-9]*\]\)\.name='wan'/\1/p")
    for ifc in $legacy; do
        uci -q delete network.$ifc
        [ -n "$z" ] && uci -q del_list firewall.$z.network=$ifc
    done

    uci -q delete network.wan3g
    uci set network.wan3g=interface
    uci set network.wan3g.proto=dhcp
    uci set network.wan3g.device=wwan0
    uci set network.wan3g.apn="$APN"
    uci commit network

    if [ -n "$z" ]; then
        uci -q get firewall.$z.network | grep -q wan3g || uci add_list firewall.$z.network=wan3g
        uci commit firewall
    fi
    /etc/init.d/network reload
    sleep 2
}

# ----------------------------------------------------------------------
# Etapa: conectado por QMI con internet real.
# mf193-qmi ya tiene su propio flock interno (etapa "script"), asi que si
# justo hay un "watch" de cron corriendo, connect() de ahi sale enseguida
# en vez de pelear por el modem - por eso ademas reintentamos un par de
# veces en vez de darnos por vencidos al primer choque de lock.
# ----------------------------------------------------------------------
etapa_conexion_check() { online_wwan; }
etapa_conexion_fix() {
    i=0
    while [ $i -lt 3 ]; do
        mf193-qmi connect && break
        i=$((i+1))
        sleep 15
    done
    mf193-qmi status
}

# ----------------------------------------------------------------------
# Etapa: VPN con tunel real (no solo el proceso corriendo). Si OpenVPN
# esta configurado pero tun0 no existe o no tiene IP (le paso al perder la
# ruta vieja y ganar la nueva por 3G), se reinicia. Si no hay ningun
# .ovpn instalado, la VPN no es parte de esta migracion (se carga aparte
# por LuCI) y se marca como "SIN_CONFIGURAR", no como falla.
# ----------------------------------------------------------------------
etapa_vpn_check() {
    ls /etc/openvpn/*.ovpn >/dev/null 2>&1 || return 2
    ip addr show tun0 2>/dev/null | grep -q 'inet '
}
etapa_vpn_fix() {
    [ -x /etc/init.d/openvpn ] && pgrep -f openvpn >/dev/null 2>&1 && {
        echo "  tun0 sin IP (o inexistente) - reiniciando OpenVPN"
        /etc/init.d/openvpn restart >/dev/null 2>&1
    }
    sleep 6
}

run_etapa red       "interfaz 3G (QMI)"
run_etapa conexion  "conexion / internet"

# VPN: distinguimos "sin configurar" (no es un fallo) de "falla real"
if ls /etc/openvpn/*.ovpn >/dev/null 2>&1; then
    run_etapa vpn "VPN" || true
else
    echo "[VPN] no hay ningun .ovpn instalado en este router - no aplica, se omite"
    marcar "vpn" "SIN_CONFIGURAR"
fi

# ----------------------------------------------------------------------
# Validacion final: se vuelve a chequear TODO de cero (no lo que paso
# durante los fix de arriba), y esto es lo que decide el veredicto.
# ----------------------------------------------------------------------
echo "== Validacion final =="
TODO_OK=1
etapa_paquetes_check  || TODO_OK=0
etapa_script_check    || TODO_OK=0
etapa_autostart_check || TODO_OK=0
etapa_sshwan_check    || TODO_OK=0
etapa_red_check       || TODO_OK=0
etapa_conexion_check  || TODO_OK=0
if ls /etc/openvpn/*.ovpn >/dev/null 2>&1; then
    etapa_vpn_check || TODO_OK=0
fi

if [ "$TODO_OK" = 1 ]; then
    echo "=================================================================="
    echo " RESULTADO: OK - EL ROUTER NAVEGA POR 3G (QMI, apn $APN) - TODO VALIDADO"
    echo "=================================================================="
    { echo "OK"; echo "$ETAPAS"; } > "$RES"
else
    echo "=================================================================="
    echo " RESULTADO: FALLO - alguna etapa no quedo validada, ver detalle"
    echo "=================================================================="
    echo ">> Confirmar el APN con el operador y que la SIM tenga datos habilitados."
    dump
    { echo "FAIL"; echo "$ETAPAS"; } > "$RES"
fi
FASE2
chmod +x "$P2"

if command -v setsid >/dev/null 2>&1; then setsid sh "$P2" >/dev/null 2>&1 </dev/null &
elif command -v nohup >/dev/null 2>&1; then nohup sh "$P2" >/dev/null 2>&1 </dev/null &
else sh "$P2" >/dev/null 2>&1 </dev/null & fi

echo "== Fase de red lanzada en segundo plano (hereda el lock). El instalador lee el resultado. =="
exit 0
