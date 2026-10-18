# run.ps1 - Starts the native C Smart Parking HTTP server

param (
    [int]$Port = 8080
)

Write-Host "=======================================================" -ForegroundColor Cyan
Write-Host " Launching Smart Parking Management System (C Server)" -ForegroundColor Cyan
Write-Host "=======================================================" -ForegroundColor Cyan

# Check if parking_backend.dll exists; if not, build it
if (-not (Test-Path "parking_backend.dll")) {
    Write-Host "parking_backend.dll not found. Running build first..." -ForegroundColor Yellow
    & ".\scripts\build.ps1"
}

Write-Host "Server running at: http://localhost:$Port" -ForegroundColor Green
Write-Host "Web Dashboard:    http://localhost:$Port/" -ForegroundColor Green
Write-Host "API Health Check: http://localhost:$Port/api/health" -ForegroundColor Green
Write-Host "Press Ctrl+C to stop the server gracefully." -ForegroundColor Yellow
Write-Host ""

python backend/run_server.py $Port
