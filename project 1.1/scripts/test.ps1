# test.ps1 - Runs automated C unit tests and returns appropriate exit code

Write-Host "=======================================================" -ForegroundColor Cyan
Write-Host " Executing Smart Parking C Unit Test Suite" -ForegroundColor Cyan
Write-Host "=======================================================" -ForegroundColor Cyan

# Check if parking_engine.dll exists; if not, build it
if (-not (Test-Path "parking_engine.dll")) {
    Write-Host "parking_engine.dll not found. Running build first..." -ForegroundColor Yellow
    & ".\scripts\build.ps1"
}

# Run test runner via Python runner
python backend/run_tests.py
$testExitCode = $LASTEXITCODE

if ($testExitCode -eq 0) {
    Write-Host "`n[SUCCESS] All 24 Capstone DSA unit tests passed without errors!" -ForegroundColor Green
} else {
    Write-Host "`n[FAILURE] One or more unit tests failed! Exit code: $testExitCode" -ForegroundColor Red
}

exit $testExitCode
