# build.ps1 - Compiles Smart Parking Management System C backend and test suites

$ErrorActionPreference = "Stop"

Write-Host "=======================================================" -ForegroundColor Cyan
Write-Host " Building Smart Parking Management System (C Backend)" -ForegroundColor Cyan
Write-Host "=======================================================" -ForegroundColor Cyan

# Locate C compiler
$zigPath = "C:\Users\KING\AppData\Local\Microsoft\WinGet\Packages\zig.zig_Microsoft.Winget.Source_8wekyb3d8bbwe\zig-x86_64-windows-0.17.0\zig.exe"
$compiler = $null
$isZig = $false

if (Test-Path $zigPath) {
    $compiler = $zigPath
    $isZig = $true
} else {
    $gcc = Get-Command gcc -ErrorAction SilentlyContinue
    if ($gcc) {
        $compiler = "gcc"
    } else {
        Write-Error "No C compiler found! Run .\scripts\setup.ps1 first."
        exit 1
    }
}

Write-Host "Using C compiler: $compiler" -ForegroundColor Green

# Collect source files
$backendSrcs = Get-ChildItem -Path "backend/src/*.c" | ForEach-Object { $_.FullName }
$testSrcs = @(
    (Resolve-Path "backend/tests/test_main.c").Path,
    (Resolve-Path "backend/src/linked_list.c").Path,
    (Resolve-Path "backend/src/queue.c").Path,
    (Resolve-Path "backend/src/hash_table.c").Path,
    (Resolve-Path "backend/src/sorting.c").Path,
    (Resolve-Path "backend/src/searching.c").Path,
    (Resolve-Path "backend/src/graph.c").Path,
    (Resolve-Path "backend/src/parking.c").Path,
    (Resolve-Path "backend/src/vehicle.c").Path,
    (Resolve-Path "backend/src/billing.c").Path,
    (Resolve-Path "backend/src/reservation.c").Path,
    (Resolve-Path "backend/src/storage.c").Path,
    (Resolve-Path "backend/src/cJSON.c").Path
)

# 1. Compile backend dynamic library (parking_backend.dll)
Write-Host "Compiling parking_backend.dll..." -ForegroundColor Yellow
if ($isZig) {
    & $compiler cc -shared -o "parking_backend.dll" -Ibackend/include $backendSrcs -lws2_32 -O2
} else {
    & $compiler -shared -o "parking_backend.dll" -Ibackend/include $backendSrcs -lws2_32 -O2
}
if ($LASTEXITCODE -ne 0) {
    Write-Error "Failed to compile parking_backend.dll"
    exit 1
}
Write-Host "[OK] parking_backend.dll compiled successfully." -ForegroundColor Green

# 2. Compile test engine dynamic library (parking_engine.dll)
Write-Host "Compiling parking_engine.dll..." -ForegroundColor Yellow
if ($isZig) {
    & $compiler cc -shared -o "parking_engine.dll" -Ibackend/include $testSrcs -lws2_32 -O2
} else {
    & $compiler -shared -o "parking_engine.dll" -Ibackend/include $testSrcs -lws2_32 -O2
}
if ($LASTEXITCODE -ne 0) {
    Write-Error "Failed to compile parking_engine.dll"
    exit 1
}
Write-Host "[OK] parking_engine.dll compiled successfully." -ForegroundColor Green

# 3. Compile standalone parking_server.exe binary
Write-Host "Compiling backend/parking_server.exe..." -ForegroundColor Yellow
if ($isZig) {
    & $compiler cc -o "backend/parking_server.exe" -Ibackend/include $backendSrcs -lws2_32 -O2
} else {
    & $compiler -o "backend/parking_server.exe" -Ibackend/include $backendSrcs -lws2_32 -O2
}
Write-Host "[OK] Standalone backend/parking_server.exe compiled successfully." -ForegroundColor Green

Write-Host ""
Write-Host "=======================================================" -ForegroundColor Cyan
Write-Host " BUILD SUCCESSFUL!" -ForegroundColor Green
Write-Host " Next steps:" -ForegroundColor Cyan
Write-Host "   Run tests:  .\scripts\test.ps1" -ForegroundColor White
Write-Host "   Run server: .\scripts\run.ps1" -ForegroundColor White
Write-Host "=======================================================" -ForegroundColor Cyan
