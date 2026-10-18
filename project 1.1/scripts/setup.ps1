# setup.ps1 - Environment Verification and Setup for Smart Parking Management System

Write-Host "=======================================================" -ForegroundColor Cyan
Write-Host " Smart Parking Management System - Environment Setup" -ForegroundColor Cyan
Write-Host "=======================================================" -ForegroundColor Cyan

# Check Python
$pythonCmd = Get-Command python -ErrorAction SilentlyContinue
if ($pythonCmd) {
    Write-Host "[OK] Python detected: $($pythonCmd.Source)" -ForegroundColor Green
} else {
    Write-Host "[WARN] Python not found in PATH." -ForegroundColor Yellow
}

# Check Git
$gitCmd = Get-Command git -ErrorAction SilentlyContinue
if ($gitCmd) {
    Write-Host "[OK] Git detected: $($gitCmd.Source)" -ForegroundColor Green
} else {
    Write-Host "[WARN] Git not found in PATH." -ForegroundColor Yellow
}

# Check C Compiler (Zig cc or GCC)
$zigPath = "C:\Users\KING\AppData\Local\Microsoft\WinGet\Packages\zig.zig_Microsoft.Winget.Source_8wekyb3d8bbwe\zig-x86_64-windows-0.17.0\zig.exe"
if (Test-Path $zigPath) {
    Write-Host "[OK] C Compiler detected (Zig Clang C Toolchain): $zigPath" -ForegroundColor Green
} else {
    $gccCmd = Get-Command gcc -ErrorAction SilentlyContinue
    if ($gccCmd) {
        Write-Host "[OK] GCC detected: $($gccCmd.Source)" -ForegroundColor Green
    } else {
        Write-Host "[INFO] Installing Zig C compiler toolchain via winget..." -ForegroundColor Yellow
        winget install --id zig.zig --scope user --accept-package-agreements --accept-source-agreements
    }
}

# Ensure data directory exists
if (-not (Test-Path "data")) {
    New-Item -ItemType Directory -Path "data" | Out-Null
    Write-Host "[OK] Created data directory" -ForegroundColor Green
} else {
    Write-Host "[OK] Data directory already exists" -ForegroundColor Green
}

Write-Host ""
Write-Host "Setup complete! Run '.\scripts\build.ps1' to compile the system." -ForegroundColor Cyan
