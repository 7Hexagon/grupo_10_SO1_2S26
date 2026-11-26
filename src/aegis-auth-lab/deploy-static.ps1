param()
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$source = $PSScriptRoot
$key = Join-Path $env:USERPROFILE '.ssh\id_ed25519_lfs'
$ssh = (Get-Command ssh.exe -CommandType Application -ErrorAction Stop).Source
$target = 'admin_aegis@192.168.56.111'
$options = @('-i', $key, '-o', 'IdentitiesOnly=yes', '-o', 'BatchMode=yes', '-o', 'ConnectTimeout=5', '-o', 'HostKeyAlias=192.168.56.110', '-o', 'StrictHostKeyChecking=yes')
if (-not (Test-Path -LiteralPath $key -PathType Leaf)) { throw 'No se encuentra la clave SSH configurada.' }
$files = @('index.html', 'style.css')
$data = @{}
$hash = @{}
foreach ($name in $files) {
    $path = Join-Path $source $name
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Falta $path" }
    $bytes = [IO.File]::ReadAllBytes($path)
    $data[$name] = [Convert]::ToBase64String($bytes)
    $sha = [Security.Cryptography.SHA256]::Create()
    try { $hash[$name] = ([BitConverter]::ToString($sha.ComputeHash($bytes))).Replace('-', '').ToLowerInvariant() } finally { $sha.Dispose() }
    Write-Host "$name SHA256 $($hash[$name])"
}
$id = [guid]::NewGuid().ToString('N')
$stage = "/home/admin_aegis/aegis-static-stage-$id"
$backup = "/root/aegis-static-backup-$id"
Write-Host "Preparando archivos en $stage"
& $ssh @options $target "umask 077; mkdir -m 700 $stage"
if ($LASTEXITCODE -ne 0) { throw 'No se pudo crear el directorio temporal; no se inicio la instalacion.' }
foreach ($name in $files) {
    $encodedFile = $data[$name]
    & $ssh @options $target "umask 077; printf %s $encodedFile | base64 -d > $stage/$name"
    if ($LASTEXITCODE -ne 0) { throw "Fallo la transferencia de $name; no se inicio la instalacion." }
}
$script = @"
#!/bin/sh
set -eu
umask 077
stage='$stage'
backup='$backup'
dest=/opt/aegis-auth-lab
[ "`$(id -u)" = 0 ] || { echo 'Se requiere root'; exit 1; }
[ -d "`$stage" ] && [ ! -L "`$stage" ] || exit 1
for f in index.html style.css; do
  [ -f "`$stage/`$f" ] && [ ! -L "`$stage/`$f" ] && [ -f "`$dest/`$f" ] && [ ! -L "`$dest/`$f" ] || exit 1
done
mkdir -m 700 "`$backup"
mkdir -m 700 "`$backup/candidate"
cp "`$stage/index.html" "`$stage/style.css" "`$backup/candidate/"
printf '%s  %s\n' '$($hash['index.html'])' "`$backup/candidate/index.html" '$($hash['style.css'])' "`$backup/candidate/style.css" | sha256sum -c -
cp -p "`$dest/index.html" "`$dest/style.css" "`$backup/"
echo "BACKUP=`$backup"
changed=0
rollback() {
  rc=`$?
  if [ "`$rc" -ne 0 ] && [ "`$changed" -eq 1 ]; then
    echo "ERROR: restaurando desde `$backup" >&2
    cp -p "`$backup/index.html" "`$dest/index.html" || echo 'ERROR restaurando index.html' >&2
    cp -p "`$backup/style.css" "`$dest/style.css" || echo 'ERROR restaurando style.css' >&2
  fi
}
trap rollback EXIT
changed=1
for f in index.html style.css; do
  cp "`$backup/candidate/`$f" "`$dest/`$f"
done
printf '%s  %s\n' '$($hash['index.html'])' "`$dest/index.html" '$($hash['style.css'])' "`$dest/style.css" | sha256sum -c -
for f in index.html style.css; do
  [ "`$(stat -c '%u:%g:%a' "`$backup/`$f")" = "`$(stat -c '%u:%g:%a' "`$dest/`$f")" ] || { echo "Permisos alterados: `$f" >&2; exit 1; }
done
changed=0
echo 'INSTALACION_STATIC_PASS'
"@
$script64 = [Convert]::ToBase64String([Text.Encoding]::UTF8.GetBytes(($script -replace "`r`n", "`n")))
$scriptBytes = [Convert]::FromBase64String($script64)
$sha = [Security.Cryptography.SHA256]::Create()
try { $scriptHash = ([BitConverter]::ToString($sha.ComputeHash($scriptBytes))).Replace('-', '').ToLowerInvariant() } finally { $sha.Dispose() }
& $ssh @options $target "umask 077; printf %s $script64 | base64 -d > $stage/install.sh && echo $scriptHash  $stage/install.sh | sha256sum -c - && chmod 700 $stage/install.sh"
if ($LASTEXITCODE -ne 0) { throw 'Fallo la transferencia de instrucciones; no se inicio la instalacion.' }
Write-Host 'Se solicitara la contrasena de root mediante su, si corresponde.'
& $ssh @options -t $target "su -c $stage/install.sh"
if ($LASTEXITCODE -ne 0) { throw "Fallo la instalacion. Revise la salida y el respaldo $backup antes de reintentar." }
Write-Host "Respaldo: $backup"
Write-Host "Temporal conservado: $stage"
