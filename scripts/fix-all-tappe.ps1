#!/usr/bin/env pwsh
#
# fix-all-tappe.ps1
# Aggiorna CMakeLists.txt e main.cpp di tutte le tappe 01-23:
#   - FetchContent per SFML (elimina path hardcoded)
#   - Profilo OpenGL Core
#   - sf::Window invece di sf::RenderWindow
#   - Rimozione pannello UI SFML 2D (tappe 10-23)
# Poi esegue le operazioni git richieste dal professore:
#   branch, commit, elimina tag, ricrea tag.
#
param(
    [switch]$DryRun
)

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$transformScript = Join-Path $PSScriptRoot "transform_tappa.py"

# ── CMakeLists template per gruppi ────────────────────────────────────────────

# Gruppo A: tappa-01-02  (CXX only, senza glad, senza SFML::Graphics)
$cmakeGroupA = @'
cmake_minimum_required(VERSION 3.21)

project(SistemaSolare3D LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG        3.0.0
    GIT_SHALLOW    TRUE
    SYSTEM
)

FetchContent_MakeAvailable(SFML)

add_executable(SistemaSolare3D
    src/main.cpp
)

target_link_libraries(SistemaSolare3D PRIVATE
    SFML::Window
    SFML::System
    opengl32
)
'@

# Gruppo B: tappa-03-14  (C CXX, glad, senza SFML::Graphics)
$cmakeGroupB = @'
cmake_minimum_required(VERSION 3.21)

project(SistemaSolare3D LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG        3.0.0
    GIT_SHALLOW    TRUE
    SYSTEM
)

FetchContent_MakeAvailable(SFML)

add_executable(SistemaSolare3D
    src/main.cpp
    src/glad.c
)

target_include_directories(SistemaSolare3D PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(SistemaSolare3D PRIVATE
    SFML::Window
    SFML::System
    opengl32
)
'@

# Gruppo C: tappa-15-23  (C CXX, glad, con SFML::Graphics per sf::Image)
$cmakeGroupC = @'
cmake_minimum_required(VERSION 3.21)

project(SistemaSolare3D LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG        3.0.0
    GIT_SHALLOW    TRUE
    SYSTEM
)

FetchContent_MakeAvailable(SFML)

add_executable(SistemaSolare3D
    src/main.cpp
    src/glad.c
)

target_include_directories(SistemaSolare3D PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(SistemaSolare3D PRIVATE
    SFML::Window
    SFML::Graphics
    SFML::System
    opengl32
)
'@

# ── Configurazione per ogni tappa ─────────────────────────────────────────────

# campoMain: se modificare main.cpp e come
#   none          = nessuna modifica a main.cpp
#   add-core      = aggiunge solo la riga attributeFlags Core
#   remove-panel  = rimuovi pannello UI, senza sf::Image (--keep-graphics NO)
#   remove-panel-keep-gfx = rimuovi pannello, mantieni sf::Image (--keep-graphics)
#   remove-panel-legend   = rimuovi pannello + drawControlsLegend

$tappaConfigs = [ordered]@{
    "tappa-01" = @{ cmake = "A"; mainAction = "add-core" }
    "tappa-02" = @{ cmake = "A"; mainAction = "add-core" }
    "tappa-03" = @{ cmake = "B"; mainAction = "none" }
    "tappa-04" = @{ cmake = "B"; mainAction = "none" }
    "tappa-05" = @{ cmake = "B"; mainAction = "none" }
    "tappa-06" = @{ cmake = "B"; mainAction = "none" }
    "tappa-07" = @{ cmake = "B"; mainAction = "none" }
    "tappa-08" = @{ cmake = "B"; mainAction = "none" }
    "tappa-09" = @{ cmake = "B"; mainAction = "none" }
    "tappa-10" = @{ cmake = "B"; mainAction = "remove-panel" }
    "tappa-11" = @{ cmake = "B"; mainAction = "remove-panel-legend" }
    "tappa-12" = @{ cmake = "B"; mainAction = "remove-panel-legend" }
    "tappa-13" = @{ cmake = "B"; mainAction = "remove-panel-legend" }
    "tappa-14" = @{ cmake = "B"; mainAction = "remove-panel-legend" }
    "tappa-15" = @{ cmake = "C"; mainAction = "remove-panel-keep-gfx-legend" }
    "tappa-16" = @{ cmake = "C"; mainAction = "remove-panel-keep-gfx-legend" }
    "tappa-17" = @{ cmake = "C"; mainAction = "remove-panel-keep-gfx-legend" }
    "tappa-18" = @{ cmake = "C"; mainAction = "remove-panel-keep-gfx-legend" }
    "tappa-19" = @{ cmake = "C"; mainAction = "remove-panel-keep-gfx-legend" }
    "tappa-20" = @{ cmake = "C"; mainAction = "remove-panel-keep-gfx-legend" }
    "tappa-21" = @{ cmake = "C"; mainAction = "remove-panel-keep-gfx-legend" }
    "tappa-22" = @{ cmake = "C"; mainAction = "remove-panel-keep-gfx-legend" }
    "tappa-23" = @{ cmake = "C"; mainAction = "remove-panel-keep-gfx-legend" }
}

# ── Verifica presenza delle tappe ─────────────────────────────────────────────

$existingTags = git -C $repoRoot tag --list "tappa-*"

foreach ($tag in $tappaConfigs.Keys) {
    if ($tag -notin $existingTags) {
        Write-Warning "Tag $tag non trovato nel repository, ignorato."
    }
}

# ── Funzione di aggiunta Core flag a main.cpp (tappe 01-02) ───────────────────

function Add-CoreFlag {
    param([string]$mainCpp)

    $src = Get-Content $mainCpp -Raw
    if ($src -match 'attributeFlags') {
        Write-Host "  Core flag gia presente, skip."
        return
    }
    # Inserisce dopo la riga che imposta minorVersion
    $src = $src -replace '(settings\.minorVersion\s*=\s*\d+;\s*\n)', "`$1    settings.attributeFlags = sf::ContextSettings::Attribute::Core;`n"
    Set-Content $mainCpp $src -NoNewline -Encoding utf8
}

# ── Loop principale ───────────────────────────────────────────────────────────

foreach ($tag in $tappaConfigs.Keys) {
    if ($tag -notin $existingTags) { continue }

    $cfg = $tappaConfigs[$tag]

    Write-Host ""
    Write-Host "══════════════════════════════════════════"
    Write-Host "  Elaborazione: $tag"
    Write-Host "══════════════════════════════════════════"

    # Commit puntato dal tag
    $tagCommit = (git -C $repoRoot rev-list -n 1 $tag).Trim()
    Write-Host "  Commit tag: $tagCommit"

    $branchName = "fix/$tag"

    # Elimina branch precedente se esiste
    $branchExists = git -C $repoRoot branch --list $branchName
    if ($branchExists) {
        Write-Host "  Branch $branchName gia esiste, eliminato."
        if (-not $DryRun) {
            git -C $repoRoot branch -D $branchName
        }
    }

    # Crea e checkout branch dal commit del tag
    Write-Host "  Creazione branch $branchName da $tagCommit"
    if (-not $DryRun) {
        git -C $repoRoot checkout -b $branchName $tagCommit
    }

    # ── Modifica CMakeLists.txt ────────────────────────────────────────────────
    $cmakeFile = Join-Path $repoRoot "CMakeLists.txt"
    $cmakeContent = switch ($cfg.cmake) {
        "A" { $cmakeGroupA }
        "B" { $cmakeGroupB }
        "C" { $cmakeGroupC }
    }
    Write-Host "  Scrittura CMakeLists.txt (gruppo $($cfg.cmake))"
    if (-not $DryRun) {
        Set-Content $cmakeFile $cmakeContent -NoNewline -Encoding utf8
    }

    # ── Modifica main.cpp ──────────────────────────────────────────────────────
    $mainCpp = Join-Path (Join-Path $repoRoot "src") "main.cpp"
    if (Test-Path $mainCpp) {
        $action = $cfg.mainAction
        Write-Host "  Azione main.cpp: $action"

        if (-not $DryRun) {
            switch -Wildcard ($action) {
                "add-core" {
                    Add-CoreFlag $mainCpp
                }
                "remove-panel*" {
                    $pythonArgs = @($transformScript, $mainCpp, $mainCpp)
                    if ($action -like "*keep-gfx*") { $pythonArgs += "--keep-graphics" }
                    if ($action -like "*legend*")   { $pythonArgs += "--has-legend" }
                    python @pythonArgs
                }
            }
        }
    }

    # ── Git: add, commit ──────────────────────────────────────────────────────
    Write-Host "  Git add + commit"
    if (-not $DryRun) {
        git -C $repoRoot add CMakeLists.txt
        if (Test-Path $mainCpp) {
            git -C $repoRoot add src/main.cpp
        }
        $commitMsg = "Usa FetchContent per SFML e profilo OpenGL Core"
        git -C $repoRoot commit -m $commitMsg
    }

    # ── Git: elimina e ricrea tag ─────────────────────────────────────────────
    Write-Host "  Ricrea tag $tag"
    if (-not $DryRun) {
        git -C $repoRoot tag -d $tag
        git -C $repoRoot tag $tag
    }

    # ── Torna a master ────────────────────────────────────────────────────────
    Write-Host "  Checkout master"
    if (-not $DryRun) {
        git -C $repoRoot checkout master
    }

    Write-Host "  $tag completato."
}

Write-Host ""
Write-Host "══════════════════════════════════════════"
Write-Host "  Elaborazione: tappa-24 (master branch)"
Write-Host "══════════════════════════════════════════"

if (-not $DryRun) {
    git -C $repoRoot checkout master
}

# CMakeLists con FetchContent SFML + FreeType
$cmakeTappa24 = @'
cmake_minimum_required(VERSION 3.21)

project(SistemaSolare3D LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG        3.0.0
    GIT_SHALLOW    TRUE
    SYSTEM
)
FetchContent_MakeAvailable(SFML)

FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://github.com/freetype/freetype.git
    GIT_TAG        VER-2-13-3
    GIT_SHALLOW    TRUE
    SYSTEM
)
set(FT_DISABLE_ZLIB     ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2    ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG      ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI   ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(freetype)

add_executable(SistemaSolare3D
    src/main.cpp
    src/glad.c
)

target_include_directories(SistemaSolare3D PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(SistemaSolare3D PRIVATE
    SFML::Window
    SFML::Graphics
    SFML::System
    freetype
    opengl32
)
'@

Write-Host "  Scrittura CMakeLists.txt (tappa-24 + FreeType)"
if (-not $DryRun) {
    Set-Content (Join-Path $repoRoot "CMakeLists.txt") $cmakeTappa24 -NoNewline -Encoding utf8
}

Write-Host "  Trasformazione main.cpp con fix_tappa24.py"
$mainCpp24 = Join-Path (Join-Path $repoRoot "src") "main.cpp"
if (-not $DryRun) {
    python (Join-Path $PSScriptRoot "fix_tappa24.py") $mainCpp24 $mainCpp24
}

Write-Host "  Git add + commit su master"
if (-not $DryRun) {
    git -C $repoRoot add CMakeLists.txt src/main.cpp
    git -C $repoRoot commit -m "Usa FetchContent per SFML+FreeType, profilo Core, pannello UI con OpenGL"
}

Write-Host "  Ricrea tag tappa-24"
if (-not $DryRun) {
    git -C $repoRoot tag -d tappa-24 2>$null
    git -C $repoRoot tag tappa-24
}

Write-Host "  tappa-24 completata."
Write-Host ""
Write-Host "Tutte le tappe elaborate. Verifica con: git tag -l tappa-*"
