param([string]$Version)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$package = Get-Content -Raw -LiteralPath (Join-Path $root 'wcs-web\package.json') | ConvertFrom-Json
$lock = Get-Content -Raw -LiteralPath (Join-Path $root 'wcs-web\package-lock.json') | ConvertFrom-Json -AsHashtable
$toc = Get-Content -Raw -LiteralPath (Join-Path $root 'addon\WoWCompanionScreen\WoWCompanionScreen.toc')
$protocol = Get-Content -Raw -LiteralPath (Join-Path $root 'extensions\wcs-bridge\Protocol.hpp')
$bridgeModule = Get-Content -Raw -LiteralPath (Join-Path $root 'extensions\wcs-bridge\Module.cpp')
$gamepadModule = Get-Content -Raw -LiteralPath (Join-Path $root 'extensions\wcs-gamepad\Module.cpp')

$tocMatch = [regex]::Match($toc, '(?m)^## Version:\s*(\d+\.\d+\.\d+)\s*$')
$bridgeMatch = [regex]::Match($protocol, 'kBridgeVersion\s*=\s*"(\d+\.\d+\.\d+)"')
$bridgeLogMatch = [regex]::Match($bridgeModule, 'bridge (\d+\.\d+\.\d+) for WoW')
$gamepadLogMatch = [regex]::Match($gamepadModule, 'gamepad (\d+\.\d+\.\d+) for WoW')
$bridgeAbiMatch = [regex]::Match($bridgeModule, '"wcs-bridge",\s*(0x[0-9A-Fa-f]+)')
$gamepadAbiMatch = [regex]::Match($gamepadModule, '"wcs-gamepad",\s*(0x[0-9A-Fa-f]+)')
if (-not $tocMatch.Success -or -not $bridgeMatch.Success -or -not $bridgeLogMatch.Success -or -not $gamepadLogMatch.Success -or -not $bridgeAbiMatch.Success -or -not $gamepadAbiMatch.Success) {
    throw 'Could not read committed component versions.'
}

$expected = if ($Version) { $Version } else { [string]$package.version }
$parts = $expected.Split('.')
if ($parts.Count -ne 3) { throw "Version '$expected' is not semantic X.Y.Z." }
$packed = ([int]$parts[0] -shl 16) -bor ([int]$parts[1] -shl 8) -bor [int]$parts[2]
$expectedAbi = '0x{0:X6}' -f $packed
$versions = [ordered]@{
    'PWA package' = [string]$package.version
    'PWA lockfile' = [string]$lock['version']
    'PWA lockfile root package' = [string]$lock['packages']['']['version']
    'Addon manifest' = $tocMatch.Groups[1].Value
    'Native bridge' = $bridgeMatch.Groups[1].Value
    'Native bridge log' = $bridgeLogMatch.Groups[1].Value
    'Native gamepad log' = $gamepadLogMatch.Groups[1].Value
}
foreach ($item in $versions.GetEnumerator()) {
    if ($item.Value -ne $expected) { throw "$($item.Key) version '$($item.Value)' does not match '$expected'." }
}
if ($bridgeAbiMatch.Groups[1].Value.ToUpperInvariant() -ne $expectedAbi -or $gamepadAbiMatch.Groups[1].Value.ToUpperInvariant() -ne $expectedAbi) {
    throw "Native component ABI versions must both match '$expectedAbi'."
}
Write-Host "All committed component versions match $expected."
