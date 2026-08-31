$ErrorActionPreference = "Stop"
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$PkgDir    = Join-Path $ScriptDir "mf193-offline-pkgs"
$LogFile   = Join-Path $ScriptDir ("instalacion-" + (Get-Date -Format "yyyyMMdd-HHmmss") + ".log")

Start-Transcript -Path $LogFile -Append | Out-Null

function Salir-Con-Error {
    param([string]$Mensaje)
    Write-Host ""
    Write-Host "=================================================================="  -ForegroundColor Red
    Write-Host " NO SE PUDO COMPLETAR LA INSTALACION"                                -ForegroundColor Red
    Write-Host "=================================================================="  -ForegroundColor Red
    Write-Host $Mensaje -ForegroundColor Red
    Write-Host ""
    Write-Host "El router NO fue modificado (o quedo con su configuracion de respaldo)." -ForegroundColor Yellow
    Write-Host "Guarde este mensaje y el archivo de registro para enviarlo a soporte:"   -ForegroundColor Yellow
    Write-Host "  $LogFile"
    Write-Host ""
    Read-Host "Presione ENTER para cerrar"
    Stop-Transcript | Out-Null
    exit 1
}

Write-Host "=================================================================="
Write-Host " Migracion de router - ZTE MF193"
Write-Host "=================================================================="
Write-Host ""

# ---------------------------------------------------------------------------
# Paso 0: verificar herramientas y archivos necesarios
# ---------------------------------------------------------------------------
Write-Host "Paso 0: verificando que este todo lo necesario..."

if (-not (Get-Command ssh.exe -ErrorAction SilentlyContinue)) {
    Salir-Con-Error "No se encontro 'ssh.exe' en esta PC. Windows 10/11 lo trae de fabrica (Configuracion > Aplicaciones opcionales > Cliente OpenSSH). Instalelo y vuelva a intentar."
}
if (-not (Get-Command scp.exe -ErrorAction SilentlyContinue)) {
    Salir-Con-Error "No se encontro 'scp.exe' en esta PC. Windows 10/11 lo trae de fabrica junto con el Cliente OpenSSH. Instalelo y vuelva a intentar."
}

if (-not (Test-Path $PkgDir)) {
    Salir-Con-Error "No se encontro la carpeta 'mf193-offline-pkgs' junto a este instalador. No se copio bien la carpeta completa."
}

$archivosNecesarios = @(
    "install-offline.sh",
    "uqmi",
    "kmod-mii_*.ipk",
    "kmod-usb-wdm_*.ipk",
    "kmod-usb-net_*.ipk",
    "kmod-usb-net-qmi-wwan_*.ipk",
    "usb-modeswitch_*.ipk"
)
foreach ($patron in $archivosNecesarios) {
    if (-not (Get-ChildItem -Path $PkgDir -Filter $patron -ErrorAction SilentlyContinue)) {
        Salir-Con-Error "Falta el archivo '$patron' dentro de 'mf193-offline-pkgs'. La carpeta esta incompleta - vuelva a copiarla entera desde el origen."
    }
}
Write-Host "  OK - estan todos los archivos." -ForegroundColor Green
Write-Host ""

# ---------------------------------------------------------------------------
# Paso 1: conexion al router
# ---------------------------------------------------------------------------
Write-Host "Paso 1: conexion al router"
$ip = Read-Host "IP del router [ENTER = 192.168.1.1]"
if ([string]::IsNullOrWhiteSpace($ip)) { $ip = "192.168.1.1" }

Write-Host "  Probando conexion a $ip ..."
$pingOk = $false
try {
    if (Test-Connection -ComputerName $ip -Count 2 -Quiet -ErrorAction Stop) { $pingOk = $true }
} catch { $pingOk = $false }

if (-not $pingOk) {
    Salir-Con-Error "No se pudo hacer ping a $ip. Verifique que la PC este conectada al WiFi del router (no a otra red) y que el router este encendido."
}

$sshPortOk = $false
try {
    $tcp = New-Object System.Net.Sockets.TcpClient
    $iar = $tcp.BeginConnect($ip, 22, $null, $null)
    if ($iar.AsyncWaitHandle.WaitOne(3000)) {
        $tcp.EndConnect($iar)
        $sshPortOk = $true
    }
    $tcp.Close()
} catch { $sshPortOk = $false }

if (-not $sshPortOk) {
    Salir-Con-Error "El router responde al ping pero no al puerto SSH (22). Puede estar recien reiniciando - espere un minuto y reintente."
}
Write-Host "  OK - el router responde." -ForegroundColor Green
Write-Host ""

# ---------------------------------------------------------------------------
# Paso 2: operador (carrier)
# ---------------------------------------------------------------------------
Write-Host "Paso 2: elija el operador del SIM que tiene el router"
Write-Host "  1) Claro     (default)"
Write-Host "  2) Movistar"
Write-Host "  3) Otro (ingresar datos manualmente)"
$opcion = Read-Host "Opcion [ENTER = 1]"

switch ($opcion) {
    "2" {
        $apn = "wap.gprs.unifon.com.ar"
        $user = "wap"
        $pass = "wap"
        $carrierHint = "MOVISTAR"
    }
    "3" {
        $apn = Read-Host "APN"
        $user = Read-Host "Usuario (ENTER si no tiene)"
        $pass = Read-Host "Contrasena (ENTER si no tiene)"
        $carrierHint = Read-Host "Nombre del operador (para verificar al final, ej: CLARO)"
    }
    default {
        $apn = "igprs.claro.com.ar"
        $user = ""
        $pass = ""
        $carrierHint = "CLARO"
    }
}
Write-Host "  Operador seleccionado: $carrierHint (APN=$apn)" -ForegroundColor Green
Write-Host ""
Write-Host "  Recordatorio: la contrasena de acceso SSH del router es 3gC0nn3ct!" -ForegroundColor Yellow
Write-Host ""

$sshOpts = @("-o", "StrictHostKeyChecking=no", "-o", "UserKnownHostsFile=NUL", "-o", "ConnectTimeout=10")

# ---------------------------------------------------------------------------
# Paso 3: copiar los paquetes al router
# ---------------------------------------------------------------------------
Write-Host "Paso 3: copiando archivos al router (va a pedir la contrasena)..."
# -O fuerza el protocolo SCP clasico: el dropbear del router no tiene sftp-server,
# y el scp moderno de Windows usa SFTP por defecto salvo que se le indique lo contrario.
& scp.exe -O @sshOpts -r $PkgDir "root@${ip}:/tmp/"
if ($LASTEXITCODE -ne 0) {
    Salir-Con-Error "Fallo la copia de archivos al router (scp, codigo $LASTEXITCODE). Si escribio mal la contrasena, vuelva a ejecutar el instalador. Si el error menciona 'sftp-server', puede ser un cliente SSH de Windows viejo - avisar a soporte."
}
Write-Host "  OK - archivos copiados." -ForegroundColor Green
Write-Host ""

# ---------------------------------------------------------------------------
# Paso 4: ejecutar la instalacion en el router
# ---------------------------------------------------------------------------
Write-Host "Paso 4: instalando en el router (puede tardar unos minutos, no desconecte nada)..."
$remoteCmd = "APN='$apn' PPP_USER='$user' PPP_PASS='$pass' CARRIER_HINT='$carrierHint' sh /tmp/mf193-offline-pkgs/install-offline.sh"
& ssh.exe @sshOpts "root@$ip" $remoteCmd
$instalarRc = $LASTEXITCODE
Write-Host ""

# ---------------------------------------------------------------------------
# Paso 5: resultado
# ---------------------------------------------------------------------------
if ($instalarRc -eq 0) {
    Write-Host "=================================================================="  -ForegroundColor Green
    Write-Host " LISTO - EL ROUTER QUEDO NAVEGANDO POR 3G"                           -ForegroundColor Green
    Write-Host "=================================================================="  -ForegroundColor Green
    Write-Host "Puede desconectar el cable/apagar la PC. El router va a reconectarse solo"
    Write-Host "si se corta la luz o se reinicia."
} else {
    Write-Host "=================================================================="  -ForegroundColor Red
    Write-Host " LA INSTALACION TERMINO CON UN PROBLEMA (codigo $instalarRc)"        -ForegroundColor Red
    Write-Host "=================================================================="  -ForegroundColor Red
    Write-Host "NO retire el router todavia. La configuracion original quedo respaldada"
    Write-Host "dentro del router en /etc/config/network.original-antes-de-instalar"
    Write-Host ""
    Write-Host "Guarde y envie este archivo de registro a soporte:" -ForegroundColor Yellow
    Write-Host "  $LogFile"
}

Write-Host ""
Read-Host "Presione ENTER para cerrar"
Stop-Transcript | Out-Null
exit $instalarRc
