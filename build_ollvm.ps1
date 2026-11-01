# Build dll with OLLVM obfuscation.
# Downloads stock LLVM 16 (full toolchain) + patched clang-cl.exe (OLLVM passes).
# One-time download ~400MB. Then incremental builds are instant.
#
# Usage:  .\build_ollvm.ps1          Full build
#         .\build_ollvm.ps1 -NoBuild  Configure only

param([switch]$NoBuild)

$ErrorActionPreference = 'Stop'
$Root        = $PSScriptRoot
$OllvmDir    = Join-Path $Root 'ollvm'
$PatchDir    = Join-Path $Root 'ollvm_patch'
$BuildDir    = Join-Path $Root 'build_ollvm'
$DllOutput   = Join-Path $Root 'src\build\dxilconv.dll'
$StockLLVM   = 'https://github.com/llvm/llvm-project/releases/download/llvmorg-16.0.6/LLVM-16.0.6-win64.exe'
$PatchedClang= 'https://github.com/wwh1004/ollvm-16/releases/download/llvm-16/clang-cl.7z'

Write-Host '=== OLLVM Build Pipeline ===' -ForegroundColor Cyan

# ---- helpers ----
function Test-File($p) { return (Test-Path $p) -and ((Get-Item $p).Length -gt 10000) }
function Download($url, $out) {
    Write-Host ('  downloading ' + (Split-Path $out -Leaf) + ' ...')
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    $ProgressPreference = 'SilentlyContinue'
    try { Invoke-WebRequest -Uri $url -OutFile $out -UseBasicParsing }
    catch {
        Write-Host '  Invoke-WebRequest failed, trying curl...' -ForegroundColor Yellow
        $null = & curl.exe -L -o $out $url 2>&1
        if ($LASTEXITCODE -ne 0) { throw 'Download failed: ' + $url }
    }
}

# ---- Step 1: stock LLVM 16 toolchain (one-time) ----
$ClangCL = Join-Path $OllvmDir 'bin\clang-cl.exe'
$LldLink = Join-Path $OllvmDir 'bin\lld-link.exe'

if (-not (Test-File $ClangCL)) {
    Write-Host 'Installing stock LLVM 16...'
    $Installer = Join-Path $Root 'LLVM-16.0.6-win64.exe'
    if (-not (Test-Path $Installer)) { Download $StockLLVM $Installer }
    Write-Host '  Running LLVM installer (silent)...'
    $installArgs = '/S /D=' + $OllvmDir
    Start-Process -Wait -FilePath $Installer -ArgumentList $installArgs
    Remove-Item $Installer -ErrorAction SilentlyContinue
    if (-not (Test-File $ClangCL)) { throw 'LLVM install failed - ' + $ClangCL + ' not found' }
    Write-Host '  Stock LLVM 16 installed.' -ForegroundColor Green
}

# ---- Step 2: OLLVM-patched clang-cl.exe (one-time, ~4MB) ----
$PatchedExe = Join-Path $PatchDir 'clang-cl.exe'

if (-not (Test-File $PatchedExe)) {
    Write-Host 'Downloading OLLVM-patched clang-cl...'
    New-Item -ItemType Directory -Force $PatchDir | Out-Null
    $SevenZ = Join-Path $PatchDir 'clang-cl.7z'
    Download $PatchedClang $SevenZ

    Write-Host '  extracting with 7z...'
    $SevenZBin = '7z.exe'
    if (-not (Get-Command $SevenZBin -ErrorAction SilentlyContinue)) {
        $SevenZBin = Join-Path $env:ProgramFiles '7-Zip\7z.exe'
        if (-not (Test-Path $SevenZBin)) {
            $SevenZBin = Join-Path ${env:ProgramFiles(x86)} '7-Zip\7z.exe'
        }
    }
    if (-not (Test-Path $SevenZBin)) {
        Write-Host '7-Zip not found. Expand-Archive can not open .7z files.' -ForegroundColor Yellow
        Write-Host 'Please manually extract ' + $SevenZ + ' to ' + $PatchDir
        Write-Host 'Then re-run this script.'
        throw '7z.exe required - install from https://7-zip.org/'
    }
    & $SevenZBin x -o"$PatchDir" "$SevenZ" -y
    if ($LASTEXITCODE -ne 0) { throw '7z extract failed' }
    Remove-Item $SevenZ

    if (-not (Test-File $PatchedExe)) { throw 'clang-cl.exe not found in archive' }
    Write-Host '  OLLVM clang-cl ready.' -ForegroundColor Green
}

# ---- Step 3: overlay OLLVM binary onto stock LLVM ----
$TargetClang = Join-Path $OllvmDir 'bin\clang-cl.exe'
$BackupClang = Join-Path $OllvmDir 'bin\clang-cl.exe.orig'

if (-not (Test-Path $BackupClang)) {
    Copy-Item $TargetClang $BackupClang
}

$needOverlay = $true
if (Test-Path $TargetClang) {
    $stockHash  = (Get-FileHash $BackupClang -Algorithm SHA256).Hash
    $targetHash = (Get-FileHash $TargetClang -Algorithm SHA256).Hash
    if ($targetHash -ne $stockHash) { $needOverlay = $false }
}
if ($needOverlay) {
    Copy-Item -Force $PatchedExe $TargetClang
    Write-Host '  OLLVM clang-cl.exe overlayed onto stock LLVM.' -ForegroundColor Green
}

# ---- Step 4: also need lld-link (linker) for clang-cl ----
# clang-cl + OLLVM uses lld-link, not MSVC link.exe.
# The stock LLVM install already has lld-link.exe.
if (-not (Test-Path $LldLink)) {
    Write-Host 'WARNING: lld-link.exe not found. clang-cl may fail to link.' -ForegroundColor Yellow
}

# ---- Step 5: build ----
Write-Host 'Configuring CMake...'
New-Item -ItemType Directory -Force $BuildDir | Out-Null

$cmakeArgs = @(
    '-B', $BuildDir,
    '-S', (Join-Path $Root 'src'),
    '-DCMAKE_BUILD_TYPE=Release',
    '-DHELLO_USE_OLLVM=ON',
    '-DCMAKE_C_COMPILER='   + $TargetClang,
    '-DCMAKE_CXX_COMPILER=' + $TargetClang,
    '-DCMAKE_LINKER='       + $LldLink
)

$ninja = Get-Command ninja -ErrorAction SilentlyContinue
if ($ninja) {
    $cmakeArgs += '-G','Ninja'
} else {
    Write-Host '  Ninja not found - using Visual Studio generator' -ForegroundColor Yellow
    $cmakeArgs += '-G','Visual Studio 17 2022'
    $cmakeArgs += '-Thost=x64'
}

& cmake @cmakeArgs
if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed' }

if ($NoBuild) {
    Write-Host 'Configured. To build: cmake --build ' + $BuildDir + ' --config Release'
    exit 0
}

Write-Host 'Compiling with OLLVM...'
& cmake --build $BuildDir --config Release
if ($LASTEXITCODE -ne 0) { throw 'Build failed' }

# ---- done ----
if (Test-Path $DllOutput) {
    $sizeKB = [math]::Round((Get-Item $DllOutput).Length / 1KB, 1)
    Write-Host ''
    Write-Host ('DLL: ' + $DllOutput + '  (' + $sizeKB + ' KB)') -ForegroundColor Green
    Write-Host 'OLLVM passes applied: fla split sub bcf sobf mba'
    Write-Host ''
    Write-Host 'Inject: Process Hacker -> GTA5.exe -> Misc -> Inject DLL'
} else {
    Write-Host ''
    Write-Host 'Build completed but DLL not found at:' -ForegroundColor Yellow
    Write-Host $DllOutput
    Write-Host ''
    Write-Host 'Check build output above for errors.'
}
