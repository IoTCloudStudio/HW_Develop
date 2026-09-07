param([switch]$Check)   # -Check: solo reconectar y leer el resultado (util si el
                        # WiFi se corto y hay que ver como salio la migracion).

$ErrorActionPreference = "Stop"
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$PkgDir    = Join-Path $ScriptDir "mf193-offline-pkgs"
$Plink     = Join-Path $ScriptDir "tools\plink.exe"
$Pscp      = Join-Path $ScriptDir "tools\pscp.exe"
$LogFile   = Join-Path $ScriptDir ("instalacion-" + (Get-Date -Format "yyyyMMdd-HHmmss") + ".log")

Start-Transcript -Path $LogFile -Append | Out-Null

# Contrasena SSH del router. Es la misma de fabrica en todos los equipos: el
# tecnico normalmente NO tiene que escribir nada. Solo se le pide si esta no anda.
$PW = "3gC0nn3ct!"
$Ip = $null

# Ejecuta un comando en el router por plink (con la contrasena ya puesta) y
# devuelve codigo + salida. $ErrorActionPreference se baja a Continue para que
# PowerShell 5.1 no convierta el stderr en excepcion al capturarlo con 2>&1.
function Invoke-Plink {
    param([string]$Cmd)
    $old = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        $texto = & $Plink -ssh -batch -pw $PW "root@$Ip" $Cmd 2>&1 | Out-String
        $codigo = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $old
    }
    [pscustomobject]@{ Code = $codigo; Out = $texto }
}

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

if (-not (Test-Path $Plink) -or -not (Test-Path $Pscp)) {
    Salir-Con-Error "Falta la carpeta 'tools' con plink.exe / pscp.exe junto a este instalador. No se copio la carpeta completa - vuelva a copiarla entera desde el origen."
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
$Ip = $ip

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

if (-not $Check) {
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
}  # fin  if (-not $Check)  del Paso 2

# ---------------------------------------------------------------------------
# Paso 2b: acceso al router (contrasena de fabrica; solo se pide si no anda)
# ---------------------------------------------------------------------------
Write-Host "Paso 2b: probando acceso al router..."

# Primera conexion: aceptar y cachear la clave del host respondiendo 'y'.
# (sin -batch para poder contestar esa unica pregunta)
$probe = (& cmd /c "echo y| `"$Plink`" -ssh -pw `"$PW`" root@$ip `"echo AUTH_OK`" 2>&1") | Out-String

if ($probe -notmatch "AUTH_OK") {
    Write-Host "  La contrasena de fabrica no funciono en este router." -ForegroundColor Yellow
    $PW = Read-Host "  Escriba la contrasena SSH del router"
    $probe2 = Invoke-Plink "echo AUTH_OK"
    if ($probe2.Out -notmatch "AUTH_OK") {
        Salir-Con-Error "No se pudo entrar al router. Revise la contrasena y que sea el equipo correcto.`n(detalle: $($probe2.Out.Trim()))"
    }
}
Write-Host "  OK - acceso listo." -ForegroundColor Green
Write-Host ""

if (-not $Check) {
# ---------------------------------------------------------------------------
# Paso 3: copiar los paquetes al router
# ---------------------------------------------------------------------------
Write-Host "Paso 3: copiando archivos al router..."
& $Pscp -scp -batch -pw $PW -r $PkgDir "root@${ip}:/tmp/"
if ($LASTEXITCODE -ne 0) {
    Salir-Con-Error "Fallo la copia de archivos al router (pscp, codigo $LASTEXITCODE). Reintente ejecutando INSTALAR.bat."
}
Write-Host "  OK - archivos copiados." -ForegroundColor Green
Write-Host ""

# ---------------------------------------------------------------------------
# Paso 4: instalacion en el router (fase 1: NO toca la red todavia)
# ---------------------------------------------------------------------------
Write-Host "Paso 4: instalando en el router (puede tardar unos minutos, no desconecte nada)..."
$remoteCmd = "APN='$apn' PPP_USER='$user' PPP_PASS='$pass' CARRIER_HINT='$carrierHint' sh /tmp/mf193-offline-pkgs/install-offline.sh"
& $Plink -ssh -batch -pw $PW "root@$ip" $remoteCmd
$fase1Rc = $LASTEXITCODE
Write-Host ""

if ($fase1Rc -ne 0) {
    Salir-Con-Error "Fallo la instalacion en el router (codigo $fase1Rc) antes de tocar la red. La configuracion original NO fue modificada. Guarde el registro y avise a soporte."
}
}  # fin  if (-not $Check)  de Pasos 3 y 4

# ---------------------------------------------------------------------------
# Paso 5: la fase de red corre sola en el router (desacoplada del SSH). Nos
#         reconectamos cada pocos segundos a leer el resultado.
#         Solo se usa 'network reload' (no 'restart'), asi que la LAN y el
#         WiFi del router NO se caen. Aun asi, si estas por WiFi y por lo que
#         sea la PC se desconecta: volve a conectar la PC a la red del router
#         y esta ventana sigue esperando (o corre VER-RESULTADO.bat).
# ---------------------------------------------------------------------------
if ($Check) {
    Write-Host "Modo verificacion: leyendo el resultado de la ultima migracion..."
} else {
    Write-Host "Paso 5: el router esta configurando el modem por QMI y probando el 3G."
    Write-Host "        Puede tardar 3-6 minutos." -ForegroundColor Yellow
}
Write-Host "        NO cierre esta ventana. Si usas WiFi y se corta, reconecta la" -ForegroundColor Yellow
Write-Host "        PC a la red del router; la ventana sigue reintentando." -ForegroundColor Yellow
Write-Host -NoNewline "        Esperando"

$estado   = $null
$vueltas  = 0
$deadline = (Get-Date).AddMinutes(18)
while ((Get-Date) -lt $deadline) {
    Start-Sleep -Seconds 6
    $vueltas++
    Write-Host -NoNewline "."
    if ($vueltas % 25 -eq 0) {
        Write-Host ""
        Write-Host "        (sigo esperando el resultado del router...)" -ForegroundColor DarkGray
        Write-Host -NoNewline "        "
    }
    $r = Invoke-Plink "cat /tmp/migracion-3g.resultado 2>/dev/null"
    if ($r.Code -eq 0 -and $r.Out.Trim()) { $estado = $r.Out.Trim(); break }
}
Write-Host ""
Write-Host ""

# Traer el log detallado de la fase de red (best-effort)
$logRemoto = (Invoke-Plink "cat /tmp/migracion-3g.log 2>/dev/null").Out

# ---------------------------------------------------------------------------
# Paso 6: resultado
# ---------------------------------------------------------------------------
if ($estado -match '^OK') {
    Write-Host "=================================================================="  -ForegroundColor Green
    Write-Host " LISTO - EL ROUTER QUEDO NAVEGANDO POR 3G"                           -ForegroundColor Green
    Write-Host "=================================================================="  -ForegroundColor Green
    Write-Host "Puede desconectar el cable / apagar la PC. El router se reconecta"
    Write-Host "solo si se corta la luz o se reinicia."
    $rc = 0
}
elseif ($estado -match '^FAIL') {
    Write-Host "=================================================================="  -ForegroundColor Red
    Write-Host " LA MIGRACION NO PUDO CONFIRMAR NAVEGACION POR 3G"                   -ForegroundColor Red
    Write-Host "=================================================================="  -ForegroundColor Red
    Write-Host "NO retire el router todavia. La configuracion original quedo"
    Write-Host "respaldada dentro del router en:"
    Write-Host "  /etc/config/network.original-antes-de-instalar"
    if ($logRemoto -and $logRemoto.Trim()) {
        Write-Host ""
        Write-Host "--- detalle del router ---" -ForegroundColor Yellow
        Write-Host $logRemoto
    }
    Write-Host ""
    Write-Host "Guarde y envie este archivo de registro a soporte:" -ForegroundColor Yellow
    Write-Host "  $LogFile"
    $rc = 1
}
else {
    Write-Host "=================================================================="  -ForegroundColor Yellow
    Write-Host " TODAVIA NO HAY RESULTADO"                                           -ForegroundColor Yellow
    Write-Host "=================================================================="  -ForegroundColor Yellow
    Write-Host "El router sigue probando (o la PC no pudo reconectarse a tiempo)."
    Write-Host ""
    Write-Host "  1) Si estabas por WiFi: volve a conectar la PC a la red del router."
    Write-Host "  2) Espera 1-2 minutos."
    Write-Host "  3) Doble clic en  VER-RESULTADO.bat  (en esta misma carpeta)."
    Write-Host "     Eso se reconecta y te dice como salio, sin volver a instalar nada."
    if ($logRemoto -and $logRemoto.Trim()) {
        Write-Host ""
        Write-Host "--- ultimo detalle leido del router ---" -ForegroundColor Yellow
        Write-Host $logRemoto
    }
    Write-Host ""
    Write-Host "El detalle completo queda en el router en /tmp/migracion-3g.log"
    Write-Host "Guarde y envie este archivo de registro a soporte:" -ForegroundColor Yellow
    Write-Host "  $LogFile"
    $rc = 2
}

Write-Host ""
Read-Host "Presione ENTER para cerrar"
Stop-Transcript | Out-Null
exit $rc
