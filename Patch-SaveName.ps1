# Патч Fallout4.exe: имя сейва остаётся слотом, без хвоста _00000000SM...
# Ищет формат в файле, не по адресу. Если обновление сменило строку — файл не трогает.
# Проверено на 1.11.240.0.

param(
    [string]$ExePath
)

$ErrorActionPreference = "Stop"
$signature = [System.Text.Encoding]::ASCII.GetBytes('%s%u_%s%s%s_%s_%s_%s_%u_%u')
$already = [System.Text.Encoding]::ASCII.GetBytes('%s%u')

function Find-GameExe {
    $candidates = @()
    foreach ($key in @(
        'HKLM:\SOFTWARE\WOW6432Node\Bethesda Softworks\Fallout4',
        'HKLM:\SOFTWARE\Bethesda Softworks\Fallout4'
    )) {
        $prop = Get-ItemProperty -Path $key -ErrorAction SilentlyContinue
        if ($prop.installed) { $candidates += (Join-Path $prop.installed 'Fallout4.exe') }
        if ($prop.'Installed Path') { $candidates += (Join-Path $prop.'Installed Path' 'Fallout4.exe') }
    }
    $candidates += @(
        'C:\Program Files (x86)\Steam\steamapps\common\Fallout 4\Fallout4.exe',
        'C:\Program Files\Steam\steamapps\common\Fallout 4\Fallout4.exe',
        'D:\Steam\steamapps\common\Fallout 4\Fallout4.exe',
        'D:\SteamLibrary\steamapps\common\Fallout 4\Fallout4.exe'
    )
    foreach ($path in $candidates) {
        if ($path -and (Test-Path -LiteralPath $path)) { return $path }
    }
    return $null
}

if (-not $ExePath) { $ExePath = Find-GameExe }
if (-not $ExePath -or -not (Test-Path -LiteralPath $ExePath)) {
    Write-Host "Не нашёл Fallout4.exe. Перетащи файл на Patch-SaveName.bat"
    exit 1
}

if (Get-Process -Name "Fallout4" -ErrorAction SilentlyContinue) {
    Write-Host "Сначала закрой игру."
    exit 1
}

$bytes = [System.IO.File]::ReadAllBytes($ExePath)

function IndexOf([byte[]]$hay, [byte[]]$needle, [int]$start) {
    $last = $hay.Length - $needle.Length
    for ($i = $start; $i -le $last; $i++) {
        $ok = $true
        for ($j = 0; $j -lt $needle.Length; $j++) {
            if ($hay[$i + $j] -ne $needle[$j]) { $ok = $false; break }
        }
        if ($ok) { return $i }
    }
    return -1
}

$at = IndexOf $bytes $signature 0
if ($at -ge 0) {
    $second = IndexOf $bytes $signature ($at + 1)
    if ($second -ge 0) {
        Write-Host "В файле две одинаковые строки формата. Не патчу, чтобы не задеть чужое."
        exit 1
    }
    $backup = "$ExePath.bak"
    if (-not (Test-Path -LiteralPath $backup)) {
        Copy-Item -LiteralPath $ExePath -Destination $backup
        Write-Host "Копия: $backup"
    }
    $bytes[$at + 4] = 0
    [System.IO.File]::WriteAllBytes($ExePath, $bytes)
    Write-Host "Готово: $ExePath"
    Write-Host "Новые сейвы: Exitsave0.fos, Autosave1.fos, Save1.fos"
    exit 0
}

$patched = IndexOf $bytes $already 0
$looksPatched = $false
if ($patched -ge 0 -and ($patched + 5) -lt $bytes.Length -and $bytes[$patched + 4] -eq 0) {
    $tail = [System.Text.Encoding]::ASCII.GetString($bytes, $patched + 5, [Math]::Min(21, $bytes.Length - ($patched + 5)))
    if ($tail.StartsWith('%s%s%s_%s_%s_%s_%u_%u')) { $looksPatched = $true }
}
if ($looksPatched) {
    Write-Host "Уже пропатчен: $ExePath"
    exit 0
}

Write-Host "Эта версия Fallout4.exe не содержит старый формат имени."
Write-Host "Файл не изменён. Пришли новый exe, если Bethesda опять сменила строку."
exit 2
