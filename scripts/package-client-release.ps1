#requires -Version 5.1
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^\d+\.\d+\.\d+$')]
    [string]$Version,

    [string]$BuildPath = "build\Release",
    [string]$OutputDirectory = "dist"
)

$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent $PSScriptRoot
$resolvedBuildPath = if ([IO.Path]::IsPathRooted($BuildPath)) { $BuildPath } else { Join-Path $projectRoot $BuildPath }
$resolvedOutputDirectory = if ([IO.Path]::IsPathRooted($OutputDirectory)) { $OutputDirectory } else { Join-Path $projectRoot $OutputDirectory }
$releaseName = "wow-companion-screen-client-$Version"
$stagingDirectory = Join-Path $resolvedOutputDirectory $releaseName
$archivePath = Join-Path $resolvedOutputDirectory "$releaseName.zip"
$checksumPath = "$archivePath.sha256"

function Assert-File([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Required release input is missing: $Path"
    }
}

function Copy-RequiredFile([string]$Source, [string]$Destination) {
    Assert-File $Source
    Copy-Item -LiteralPath $Source -Destination $Destination
}

$requiredBuildFiles = @(
    "wcs-core.dll",
    "wcs-patcher.exe",
    "wcs-gamepad.dll",
    "wcs-bridge.dll"
)
foreach ($file in $requiredBuildFiles) {
    Assert-File (Join-Path $resolvedBuildPath $file)
}

New-Item -ItemType Directory -Force -Path $resolvedOutputDirectory | Out-Null
if (Test-Path -LiteralPath $stagingDirectory) { Remove-Item -LiteralPath $stagingDirectory -Recurse -Force }
if (Test-Path -LiteralPath $archivePath) { Remove-Item -LiteralPath $archivePath -Force }
if (Test-Path -LiteralPath $checksumPath) { Remove-Item -LiteralPath $checksumPath -Force }

$gamepadDirectory = Join-Path $stagingDirectory "Extensions\wcs-gamepad"
$bridgeDirectory = Join-Path $stagingDirectory "Extensions\wcs-bridge"
$addonDirectory = Join-Path $stagingDirectory "Interface\AddOns"
$docsDirectory = Join-Path $stagingDirectory "docs"
New-Item -ItemType Directory -Force -Path $gamepadDirectory, $bridgeDirectory, $addonDirectory, $docsDirectory | Out-Null

Copy-RequiredFile (Join-Path $resolvedBuildPath "wcs-core.dll") $stagingDirectory
Copy-RequiredFile (Join-Path $resolvedBuildPath "wcs-patcher.exe") $stagingDirectory
Copy-RequiredFile (Join-Path $resolvedBuildPath "wcs-gamepad.dll") $gamepadDirectory
Copy-RequiredFile (Join-Path $projectRoot "extensions\wcs-gamepad\wcs-gamepad.cfg.example") $gamepadDirectory
Copy-RequiredFile (Join-Path $projectRoot "extensions\wcs-gamepad\gamecontrollerdb.txt") $gamepadDirectory
Copy-RequiredFile (Join-Path $projectRoot "extensions\wcs-gamepad\gamecontrollerdb.LICENSE.txt") $gamepadDirectory
Copy-RequiredFile (Join-Path $resolvedBuildPath "wcs-bridge.dll") $bridgeDirectory
Copy-RequiredFile (Join-Path $projectRoot "extensions\wcs-bridge\wcs-bridge.cfg.example") $bridgeDirectory

$sourceAddon = Join-Path $projectRoot "addon\WoWCompanionScreen"
if (-not (Test-Path -LiteralPath $sourceAddon -PathType Container)) { throw "Required addon directory is missing: $sourceAddon" }
Copy-Item -LiteralPath $sourceAddon -Destination $addonDirectory -Recurse

$tocPath = Join-Path $addonDirectory "WoWCompanionScreen\WoWCompanionScreen.toc"
$toc = Get-Content -LiteralPath $tocPath -Raw
$stampedToc = [regex]::Replace($toc, '(?m)^## Version:.*$', "## Version: $Version")
if ($stampedToc -eq $toc -and $toc -notmatch "(?m)^## Version: $([regex]::Escape($Version))$") {
    throw "Could not stamp the addon TOC version."
}
[IO.File]::WriteAllText($tocPath, $stampedToc, [Text.UTF8Encoding]::new($false))

Copy-RequiredFile (Join-Path $projectRoot "README.md") $stagingDirectory
Copy-RequiredFile (Join-Path $projectRoot "NOTICE.md") $stagingDirectory
Copy-RequiredFile (Join-Path $projectRoot "LICENSE") $stagingDirectory
Copy-RequiredFile (Join-Path $projectRoot "docs\CLIENT_INSTALL.md") (Join-Path $stagingDirectory "INSTALL.md")
Copy-RequiredFile (Join-Path $projectRoot "docs\CONTROLLER.md") (Join-Path $stagingDirectory "CONTROLLER.md")
Copy-RequiredFile (Join-Path $projectRoot "docs\CLIENT_INSTALL.md") $docsDirectory
Copy-RequiredFile (Join-Path $projectRoot "docs\CONTROLLER.md") $docsDirectory
Copy-RequiredFile (Join-Path $projectRoot "docs\wcs-core.cfg.example") $docsDirectory

Compress-Archive -Path (Join-Path $stagingDirectory "*") -DestinationPath $archivePath -CompressionLevel Optimal

Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive = [IO.Compression.ZipFile]::OpenRead($archivePath)
try {
    $entries = @($archive.Entries | ForEach-Object { $_.FullName.Replace('\', '/') })
    $requiredEntries = @(
        "wcs-core.dll",
        "wcs-patcher.exe",
        "Extensions/wcs-gamepad/wcs-gamepad.dll",
        "Extensions/wcs-bridge/wcs-bridge.dll",
        "Interface/AddOns/WoWCompanionScreen/WoWCompanionScreen.toc",
        "INSTALL.md",
        "CONTROLLER.md",
        "docs/CONTROLLER.md"
    )
    foreach ($entry in $requiredEntries) {
        if ($entries -notcontains $entry) { throw "Release archive is missing required entry: $entry" }
    }
    $forbidden = @($entries | Where-Object { $_ -match '(^|/)(Wow(?:\.exe|\.exe\.orig)|d3d9\.dll)$' -or $_ -like 'web/*' -or $_ -like 'wcs-web/*' })
    if ($forbidden.Count) { throw "Release archive contains forbidden entries: $($forbidden -join ', ')" }
} finally {
    $archive.Dispose()
}

$hash = (Get-FileHash -LiteralPath $archivePath -Algorithm SHA256).Hash.ToLowerInvariant()
Set-Content -LiteralPath $checksumPath -Value "$hash  $releaseName.zip" -Encoding ASCII

Write-Host "Created $archivePath"
Write-Host "Created $checksumPath"
