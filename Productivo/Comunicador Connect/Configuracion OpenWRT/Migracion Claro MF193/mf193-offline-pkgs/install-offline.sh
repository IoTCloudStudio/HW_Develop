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
# ============================================================================
set -e
PKGDIR="$(cd "$(dirname "$0")" && pwd)"
APN="${APN:-igprs.claro.com.ar}"
CARRIER_HINT="${CARRIER_HINT:-CLARO}"
echo "== Operador esperado: $CARRIER_HINT  (APN=$APN) =="

# --- verificar archivos ---
need() { [ -e "$PKGDIR"/$1 ] || { echo "FALTA $1 en $PKGDIR - carpeta incompleta"; exit 1; }; }
need "kmod-mii_*.ipk"; need "kmod-usb-wdm_*.ipk"; need "kmod-usb-net_*.ipk"
need "kmod-usb-net-qmi-wwan_*.ipk"; need "usb-modeswitch_*.ipk"
for f in uqmi qmi.sh mf193-qmi; do
    [ -s "$PKGDIR/$f" ] || { echo "FALTA $f en $PKGDIR - carpeta incompleta"; exit 1; }
done
echo "== Todos los archivos presentes =="
df -h /overlay | tail -1

# --- liberar flash ---
echo "== Liberando espacio en flash =="
for p in ppp-mod-pppoe kmod-pppoe kmod-pppox luci-ssl px5g-wolfssl kmod-usb-ledtrig-usbport; do
    opkg remove "$p" >/dev/null 2>&1 && echo "  saque $p" || true
done
opkg remove --force-depends luci-proto-ipv6 >/dev/null 2>&1 && echo "  saque luci-proto-ipv6" || true

# --- instalar QMI ---
echo "== Instalando modulos QMI =="
opkg install "$PKGDIR"/kmod-mii_*.ipk "$PKGDIR"/kmod-usb-wdm_*.ipk "$PKGDIR"/kmod-usb-net_*.ipk "$PKGDIR"/kmod-usb-net-qmi-wwan_*.ipk
cp "$PKGDIR/uqmi" /sbin/uqmi; chmod 755 /sbin/uqmi
mkdir -p /lib/netifd/proto; cp "$PKGDIR/qmi.sh" /lib/netifd/proto/qmi.sh; chmod 755 /lib/netifd/proto/qmi.sh
cp "$PKGDIR/mf193-qmi" /usr/sbin/mf193-qmi; chmod 755 /usr/sbin/mf193-qmi
opkg install "$PKGDIR"/usb-modeswitch_*.ipk

# --- hotplug: onlinear el modem al aparecer ---
cat > /etc/hotplug.d/usb/01-zte-qmi-online << 'EOF'
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
EOF
chmod +x /etc/hotplug.d/usb/01-zte-qmi-online

# --- autostart en boot ---
grep -q 'mf193-qmi connect' /etc/rc.local 2>/dev/null || {
    sed -i '/^exit 0/d' /etc/rc.local 2>/dev/null
    printf '( sleep 30; for i in 1 2 3; do /usr/sbin/mf193-qmi connect && break; sleep 20; done ) &\nexit 0\n' >> /etc/rc.local
}
# --- watchdog por cron ---
( crontab -l 2>/dev/null | grep -v mf193-qmi; echo '*/4 * * * * /usr/sbin/mf193-qmi watch' ) | crontab -
/etc/init.d/cron enable >/dev/null 2>&1; /etc/init.d/cron restart >/dev/null 2>&1

# --- esperar modo modem ---
echo "== Esperando modo modem (idProduct 0124, hasta 90s) =="
i=0; while [ $i -lt 30 ]; do
    grep -qx 0124 /sys/bus/usb/devices/*/idProduct 2>/dev/null && break
    i=$((i+1)); sleep 3
done

# --- backup config ---
BACKUP="/etc/config/network.original-antes-de-instalar"
[ -e "$BACKUP" ] || cp /etc/config/network "$BACKUP" 2>/dev/null
echo "== Backup config: $BACKUP =="

# --- fase de red DESACOPLADA del SSH (sobrevive al corte) ---
RES=/tmp/migracion-3g.resultado
LOG=/tmp/migracion-3g.log
P2=/tmp/migracion-3g-fase2.sh
rm -f "$RES" "$LOG"
{
    echo "#!/bin/sh"
    echo "set +e"
    echo "APN='$APN'"
    echo "CARRIER_HINT='$CARRIER_HINT'"
    echo "BACKUP='$BACKUP'"
    echo "RES='$RES'"
    echo "LOG='$LOG'"
} > "$P2"
cat >> "$P2" << 'FASE2'
exec >> "$LOG" 2>&1
echo "== $(date) :: fase de red (QMI) =="
D=/dev/cdc-wdm0

uq(){ s="$1"; shift; uqmi -d "$D" "$@" >/tmp/uq.$$ 2>&1 & p=$!; ( sleep "$s"; kill -9 $p 2>/dev/null )& w=$!; wait $p 2>/dev/null; kill $w 2>/dev/null; cat /tmp/uq.$$ 2>/dev/null; rm -f /tmp/uq.$$; }
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

# esperar cdc-wdm0
if [ ! -c "$D" ]; then
    echo "== esperando /dev/cdc-wdm0 (hasta 60s) =="
    n=0; while [ $n -lt 20 ] && [ ! -c "$D" ]; do n=$((n+1)); sleep 3; done
fi
if [ ! -c "$D" ]; then
    echo "=================================================================="
    echo " RESULTADO: FALLO - NO APARECE /dev/cdc-wdm0"
    echo "=================================================================="
    echo ">> El driver qmi_wwan no bindeo al modem. Revisar conexion USB / cable."
    dump; echo "FAIL" > "$RES"; exit 0
fi

# interfaz wan3g = dhcp sobre wwan0
echo "== configurando wan3g (proto dhcp / wwan0) =="
ifdown 3GWAN >/dev/null 2>&1; killall pppd chat comgt 2>/dev/null
uci -q delete network.3GWAN
uci -q delete network.wan3g
uci set network.wan3g=interface
uci set network.wan3g.proto=dhcp
uci set network.wan3g.device=wwan0
uci set network.wan3g.apn="$APN"
uci commit network
z=$(uci show firewall | sed -n "s/^firewall\.\(@zone\[[0-9]*\]\)\.name='wan'/\1/p")
if [ -n "$z" ]; then
    uci -q get firewall.$z.network | grep -q wan3g || uci add_list firewall.$z.network=wan3g
    uci -q del_list firewall.$z.network=3GWAN 2>/dev/null
    uci commit firewall
fi
/etc/init.d/network reload; sleep 2

# conectar
echo "== mf193-qmi connect =="
mf193-qmi connect; rc=$?
echo "  connect rc=$rc"
mf193-qmi status

if [ "$rc" = 0 ] && ping -I wwan0 -c 3 -W 5 8.8.8.8 >/dev/null 2>&1; then
    echo "=================================================================="
    echo " RESULTADO: OK - EL ROUTER NAVEGA POR 3G (QMI, apn $APN)"
    echo "=================================================================="
    # OpenVPN puede haber perdido la IP al aparecer la ruta 3G
    if [ -x /etc/init.d/openvpn ] && pgrep -f openvpn >/dev/null 2>&1; then
        sleep 3
        for tun in /sys/class/net/tun*; do
            [ -e "$tun" ] || continue
            ifn=$(basename "$tun")
            ip addr show "$ifn" 2>/dev/null | grep -q 'inet ' || {
                echo "  $ifn sin IP - reiniciando OpenVPN"; /etc/init.d/openvpn restart >/dev/null 2>&1
            }
        done
    fi
    echo "OK" > "$RES"
else
    echo "=================================================================="
    echo " RESULTADO: FALLO - NO SE PUDO LEVANTAR LA CONEXION QMI"
    echo "=================================================================="
    echo ">> El modem responde por QMI pero no completo la sesion de datos."
    echo "   Confirmar el APN con Claro y que la SIM tenga datos habilitados."
    dump
    echo "FAIL" > "$RES"
fi
FASE2
chmod +x "$P2"

if command -v setsid >/dev/null 2>&1; then setsid sh "$P2" >/dev/null 2>&1 </dev/null &
elif command -v nohup >/dev/null 2>&1; then nohup sh "$P2" >/dev/null 2>&1 </dev/null &
else sh "$P2" >/dev/null 2>&1 </dev/null & fi

echo "== Fase de red lanzada en segundo plano. El instalador lee el resultado. =="
exit 0
