param(
    [string]$BuildRoot  = "build/tappe",
    [string]$SourceRoot = "build/tappe-source",
    [string]$Config     = "Debug"
)

$ErrorActionPreference = "Stop"

$repoRoot       = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$buildRootPath  = Join-Path $repoRoot $BuildRoot
$sourceRootPath = Join-Path $repoRoot $SourceRoot

New-Item -ItemType Directory -Force -Path $buildRootPath  | Out-Null
New-Item -ItemType Directory -Force -Path $sourceRootPath | Out-Null

$tags = git -C $repoRoot tag --list "tappa-*" --sort=version:refname
if (-not $tags) {
    throw "Nessun tag tappa-* trovato nel repository."
}

foreach ($tag in $tags) {
    Write-Host ""
    Write-Host "==> Build $tag"

    $tagSource = Join-Path $sourceRootPath $tag
    $tagBuild  = Join-Path $buildRootPath  $tag

    if (Test-Path $tagSource) {
        Remove-Item -LiteralPath $tagSource -Recurse -Force
    }
    New-Item -ItemType Directory -Force -Path $tagSource | Out-Null

    $archivePath = Join-Path $sourceRootPath "$tag.tar"
    if (Test-Path $archivePath) {
        Remove-Item -LiteralPath $archivePath -Force
    }

    git -C $repoRoot archive --format=tar --output=$archivePath $tag
    tar -xf $archivePath -C $tagSource
    Remove-Item -LiteralPath $archivePath -Force

    if (-not (Test-Path (Join-Path $tagSource "CMakeLists.txt"))) {
        Write-Host "  Avviso: CMakeLists.txt non trovato per $tag, salto."
        continue
    }

    try {
        cmake -S $tagSource -B $tagBuild
        cmake --build $tagBuild --config $Config
        Write-Host "  OK: $tag compilata con successo."
    } catch {
        Write-Host "  ERRORE: build fallita per $tag." -ForegroundColor Red
    }
}

Write-Host ""
Write-Host "Build completata per $($tags.Count) tappe."
