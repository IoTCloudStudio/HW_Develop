#!/bin/sh
# ============================================================================
# install-offline.sh - Migracion ZTE MF193 a QMI (Claro / Movistar / etc.)
#
# 100% offline. No necesita internet en el router ni en la PC.
#
# Uso:
#   1) Copiar esta carpeta completa (mf193-offline-pkgs) al router:
#        scp -O -r mf193-offline-pkgs root@192.168.1.1:/tmp/
#   2) Entrar por SSH y ejecutar:
#        sh /tmp/mf193-offline-pkgs/install-offline.sh
#
#   Parametros opcionales por variable de entorno (si no se pasan, usa Claro):
#        APN, PPP_USER, PPP_PASS, CARRIER_HINT
#
#   Ejemplo Movistar:
#        APN='wap.gprs.unifon.com.ar' PPP_USER='wap' PPP_PASS='wap' \
#        CARRIER_HINT='MOVISTAR' sh /tmp/mf193-offline-pkgs/install-offline.sh
# ============================================================================
set -e

PKGDIR="$(cd "$(dirname "$0")" && pwd)"

# ---------------------------------------------------------------------------
# Datos del operador (con default Claro si no se pasan por variable de entorno)
# ---------------------------------------------------------------------------
APN="${APN:-igprs.claro.com.ar}"
PPP_USER="${PPP_USER:-}"
PPP_PASS="${PPP_PASS:-}"
CARRIER_HINT="${CARRIER_HINT:-CLARO}"

echo "== Operador esperado: $CARRIER_HINT  (APN=$APN, usuario=${PPP_USER:-ninguno}) =="

# ---------------------------------------------------------------------------
# Verificar que estan todos los paquetes antes de tocar nada
# ---------------------------------------------------------------------------
need() {
    if [ ! -e "$PKGDIR"/$1 ]; then
        echo "FALTA el archivo: $1 (esperado en $PKGDIR)"
        echo "No se modifico nada en el router. Revisar la carpeta copiada."
        exit 1
    fi
}
need "kmod-mii_*.ipk"
need "kmod-usb-wdm_*.ipk"
need "kmod-usb-net_*.ipk"
need "kmod-usb-net-qmi-wwan_*.ipk"
need "usb-modeswitch_*.ipk"
if [ ! -s "$PKGDIR/uqmi" ]; then
    echo "FALTA el archivo: uqmi (binario, esperado en $PKGDIR)"
    echo "No se modifico nada en el router. Revisar la carpeta copiada."
    exit 1
fi

echo "== Todos los paquetes estan presentes =="
df -h /overlay

# ---------------------------------------------------------------------------
# Liberar espacio en flash (paquetes que no se usan en este router)
# ---------------------------------------------------------------------------
echo "== Liberando espacio en flash =="
for pkg in ppp-mod-pppoe kmod-pppoe kmod-pppox luci-ssl px5g-wolfssl kmod-usb-ledtrig-usbport; do
    opkg remove "$pkg" >/dev/null 2>&1 && echo "  saque $pkg" || true
done
opkg remove --force-depends luci-proto-ipv6 >/dev/null 2>&1 && echo "  saque luci-proto-ipv6" || true

# ---------------------------------------------------------------------------
# Instalar soporte QMI (orden de dependencias: mii -> wdm -> net -> qmi-wwan)
# ---------------------------------------------------------------------------
echo "== Instalando modulos QMI =="
opkg install "$PKGDIR"/kmod-mii_*.ipk "$PKGDIR"/kmod-usb-wdm_*.ipk "$PKGDIR"/kmod-usb-net_*.ipk "$PKGDIR"/kmod-usb-net-qmi-wwan_*.ipk

echo "== Instalando uqmi (binario suelto, ya no esta en el feed de OpenWrt) =="
cp "$PKGDIR/uqmi" /sbin/uqmi
chmod 755 /sbin/uqmi

echo "== Instalando usb-modeswitch =="
opkg install "$PKGDIR"/usb-modeswitch_*.ipk

# ---------------------------------------------------------------------------
# Hotplug: destrabar el modem (factory_test) cada vez que se conecta
# ---------------------------------------------------------------------------
echo "== Instalando script de auto-online del modem =="
cat > /etc/hotplug.d/usb/01-zte-qmi-online << 'EOF'
#!/bin/sh
# Destraba el ZTE MF193 (factory_test) via QMI cada vez que aparece por USB.
[ "$DEVICENAME" = "cdc-wdm0" ] || exit 0
[ "$ACTION" = "add" ] || exit 0

exec 9>/var/run/zte-qmi-online.lock
flock -n 9 || exit 0

sleep 15

i=0
while [ $i -lt 15 ]; do
    uqmi -d /dev/cdc-wdm0 --set-device-operating-mode online >/tmp/qmi-online.log 2>&1
    mode=$(uqmi -d /dev/cdc-wdm0 --get-device-operating-mode 2>/dev/null)
    case "$mode" in
        *online*) exit 0 ;;
    esac
    i=$((i+1))
    sleep 2
done
EOF
chmod +x /etc/hotplug.d/usb/01-zte-qmi-online

# ---------------------------------------------------------------------------
# Esperar a que el modem quede en modo modem (idProduct 0124, no 0149 storage)
# ---------------------------------------------------------------------------
echo "== Esperando a que el modem cambie a modo modem (hasta 90s) =="
found=""
i=0
while [ $i -lt 30 ]; do
    if grep -qx 0124 /sys/bus/usb/devices/*/idProduct 2>/dev/null; then
        found=1
        break
    fi
    i=$((i+1))
    sleep 3
done
if [ -z "$found" ]; then
    echo "ERROR: el modem nunca aparecio en modo modem (idProduct 0124)."
    echo "Revisar que el modem este bien conectado. No se sigue con la configuracion de red."
    exit 1
fi

DEVICE=$(ls /dev/ttyUSB* 2>/dev/null | tail -1)
echo "== Modem detectado, puerto de marcado: $DEVICE =="

# ---------------------------------------------------------------------------
# Liberar el puerto por si ya habia una conexion previa usandolo
# ---------------------------------------------------------------------------
ifdown 3GWAN >/dev/null 2>&1 || true
killall pppd chat 2>/dev/null || true
sleep 2

if [ -n "$DEVICE" ]; then
    if gcom -d "$DEVICE" -s /etc/gcom/getcardinfo.gcom 2>/tmp/gcom.log | grep -qi "ZTE"; then
        echo "  Modem responde correctamente a comandos AT."
    else
        echo "  (Info: el modem no respondio a la consulta AT de prueba - no es un problema,"
        echo "   puede pasar si el puerto estaba ocupado. Se continua con la configuracion.)"
    fi
fi

# ---------------------------------------------------------------------------
# Backup de la config original (una sola vez, no se acumulan copias)
# ---------------------------------------------------------------------------
BACKUP="/etc/config/network.original-antes-de-instalar"
if [ -e "$BACKUP" ]; then
    echo "== Ya existe un backup de la config original ($BACKUP) - no lo piso =="
else
    cp /etc/config/network "$BACKUP" 2>/dev/null && echo "== Backup de la config original: $BACKUP =="
fi

# ---------------------------------------------------------------------------
# Configurar la interfaz 3G
# ---------------------------------------------------------------------------
echo "== Configurando interfaz 3GWAN =="
uci set network.3GWAN="interface"
uci set network.3GWAN.proto='3g'
uci set network.3GWAN.ipv6='auto'
uci set network.3GWAN.service='umts_only'
uci set network.3GWAN.dialnumber='*99#'
uci set network.3GWAN.device="$DEVICE"
uci set network.3GWAN.pppd_options='nocrtscts'
uci set network.3GWAN.delay='45'
uci set network.3GWAN.apn="$APN"
if [ -n "$PPP_USER" ]; then
    uci set network.3GWAN.username="$PPP_USER"
    uci set network.3GWAN.password="$PPP_PASS"
fi
uci commit network

# ---------------------------------------------------------------------------
# Asegurar que 3GWAN este en la zona wan del firewall
# ---------------------------------------------------------------------------
echo "== Verificando firewall =="
i=0
WANIDX=""
while : ; do
    zname=$(uci -q get firewall.@zone[$i].name 2>/dev/null) || break
    if [ "$zname" = "wan" ]; then
        WANIDX=$i
        break
    fi
    i=$((i+1))
done
if [ -n "$WANIDX" ]; then
    if uci -q get firewall.@zone[$WANIDX].network 2>/dev/null | grep -qw "3GWAN"; then
        echo "  3GWAN ya esta en la zona wan."
    else
        uci add_list firewall.@zone[$WANIDX].network='3GWAN'
        uci commit firewall
        /etc/init.d/firewall reload
        echo "  Agregue 3GWAN a la zona wan del firewall."
    fi
else
    echo "  (Aviso: no encontre una zona 'wan' en el firewall - revisar manualmente.)"
fi

# ---------------------------------------------------------------------------
# Reiniciar red y verificar navegacion real (no solo que no haya errores)
# ---------------------------------------------------------------------------
echo "== Reiniciando red =="
/etc/init.d/network restart

echo "== Verificando navegacion por 3G (hasta 2 minutos) =="
ok=""
for i in $(seq 1 24); do
    sleep 5
    up=$(ifstatus 3GWAN 2>/dev/null | grep -c '"up": true' || true)
    if [ "$up" = "1" ]; then
        l3=$(ifstatus 3GWAN 2>/dev/null | sed -n 's/.*"l3_device": "\([^"]*\)".*/\1/p')
        if ping -I "${l3:-3g-3GWAN}" -c3 -W5 8.8.8.8 >/dev/null 2>&1; then
            ok=1
            break
        fi
    fi
done

if [ -n "$ok" ]; then
    echo
    echo "=================================================================="
    echo " RESULTADO: OK - EL ROUTER YA NAVEGA POR 3G"
    echo "=================================================================="

    echo "== Verificando operador registrado =="
    reg=$(uqmi -d /dev/cdc-wdm0 --get-serving-system 2>/dev/null || true)
    echo "$reg"
    case "$reg" in
        *"$CARRIER_HINT"*) echo "  Operador OK: coincide con $CARRIER_HINT" ;;
        *) echo "  (Aviso: no se pudo confirmar que el operador sea $CARRIER_HINT. No bloquea el resultado.)" ;;
    esac

    # -----------------------------------------------------------------
    # Auto-reparacion de VPN (OpenVPN puede perder su IP al reiniciar red)
    # -----------------------------------------------------------------
    if [ -x /etc/init.d/openvpn ] && pgrep -f openvpn >/dev/null 2>&1; then
        echo
        echo "== Revisando la VPN (OpenVPN) tras reiniciar la red =="
        sleep 3
        vpn_rota=""
        for tun in /sys/class/net/tun*; do
            [ -e "$tun" ] || continue
            ifname=$(basename "$tun")
            if ip addr show "$ifname" 2>/dev/null | grep -q "state UP\|UP,LOWER_UP" \
                && ! ip addr show "$ifname" 2>/dev/null | grep -q "inet "; then
                vpn_rota="$ifname"
            fi
        done
        if [ -n "$vpn_rota" ]; then
            echo "  $vpn_rota quedo sin IP - reiniciando OpenVPN para restablecerla"
            /etc/init.d/openvpn restart >/dev/null 2>&1
            vpn_ok=""
            for vi in $(seq 1 8); do
                sleep 5
                if ip addr show "$vpn_rota" 2>/dev/null | grep -q "inet "; then
                    vpn_ok=1
                    break
                fi
            done
            if [ -n "$vpn_ok" ]; then
                vpnip=$(ip -4 addr show "$vpn_rota" 2>/dev/null | sed -n 's/.*inet \([0-9.]*\).*/\1/p' || true)
                echo "  Listo - $vpn_rota tiene IP $vpnip"
            else
                echo "  !! Sigue sin IP despues de 40s. La VPN puede necesitar"
                echo "  !! mas tiempo o revision aparte - no bloquea el resultado del 3G."
            fi
        else
            echo "  OK - la VPN ya tenia IP asignada, no hizo falta tocar nada."
        fi
    fi

    exit 0
else
    echo
    echo "=================================================================="
    echo " RESULTADO: FALLO - NO SE PUDO CONFIRMAR NAVEGACION"
    echo "=================================================================="
    echo "Estado actual de la interfaz 3GWAN:"
    ifstatus 3GWAN 2>&1 || true
    echo
    echo "La configuracion original quedo respaldada en: $BACKUP"
    echo "Para volver atras: cp $BACKUP /etc/config/network && /etc/init.d/network restart"
    exit 1
fi
