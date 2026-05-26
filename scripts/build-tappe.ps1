param(
    [string]$BuildRoot = "build/tappe",
    [string]$SourceRoot = "build/tappe-source",
    [string]$Config = "Debug",
    [string]$SfmlDir = "",
    [string]$SfmlRoot = ""
)

$ErrorActionPreference = "Stop"

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$buildRootPath = Join-Path $repoRoot $BuildRoot
$sourceRootPath = Join-Path $repoRoot $SourceRoot

New-Item -ItemType Directory -Force -Path $buildRootPath | Out-Null
New-Item -ItemType Directory -Force -Path $sourceRootPath | Out-Null

$tags = git -C $repoRoot tag --list "tappa-*" --sort=version:refname
if (-not $tags) {
    throw "Nessun tag tappa-* trovato nel repository."
}

foreach ($tag in $tags) {
    Write-Host "==> Build $tag"

    $tagSource = Join-Path $sourceRootPath $tag
    $tagBuild = Join-Path $buildRootPath $tag

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

    $configureArgs = @("-S", $tagSource, "-B", $tagBuild)
    if ($SfmlDir) {
        $configureArgs += "-DSFML_DIR=$SfmlDir"
    }
    if ($SfmlRoot) {
        $configureArgs += "-DSFML_ROOT=$SfmlRoot"
    }

    cmake @configureArgs
    cmake --build $tagBuild --config $Config
}

Write-Host "Build completata per $($tags.Count) tappe."
