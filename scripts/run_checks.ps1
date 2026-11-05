# Core regression set run before each release candidate.
# Run scripts\build.bat first (the suites link against its object files).
# Results go to build\checks\<name>.txt; the script stops at the first failure.
# 'Continue': vcvars prints harmless stderr, which Windows PowerShell would otherwise treat as fatal.
# Failures are detected from exit codes below.
$ErrorActionPreference = 'Continue'
$root = Split-Path $PSScriptRoot -Parent
Set-Location $root
New-Item -ItemType Directory -Force build\checks | Out-Null
Get-ChildItem Env:FC3_*_ONLY -ErrorAction SilentlyContinue | Remove-Item
$env:FC3_STEREO_NO_XR = '1'   # offline tests must never open a headset session

# The handle/vehicle render suite: built once, run in several focused modes.
& cmd.exe /d /c ('"' + (Join-Path $PSScriptRoot 'tests\build_handle333_render_test.bat') + '"') *> build\checks\render_build.txt
if ($LASTEXITCODE) { Write-Output "render build: $LASTEXITCODE"; exit $LASTEXITCODE }

$focused = @(
    @('car',     'FC3_CAR354_ONLY',     'PASS:'),
    @('bars',    'FC3_BARS355_ONLY',    'PASS:355'),
    @('pouch',   'FC3_POUCH361_ONLY',   'PASS:361'),
    @('render',  '',                    'PASS:350 bow'),
    @('vehicle', 'FC3_VEHICLE363_ONLY', 'PASS:363')
)
foreach ($e in $focused) {
    if ($e[1]) { Set-Item -Path ('Env:' + $e[1]) -Value '1' }
    & build\stereo\handle333_render_test.exe *> ('build\checks\' + $e[0] + '.txt'); $code = $LASTEXITCODE
    if ($e[1]) { Remove-Item -Path ('Env:' + $e[1]) }
    if ($code -eq 0 -and -not (Select-String -LiteralPath ('build\checks\' + $e[0] + '.txt') -SimpleMatch $e[2])) { throw "Wrong focused branch: $($e[0])" }
    Write-Output ($e[0] + ': ' + $code); if ($code) { exit $code }
}

$suites = @(
    @('controls',  'build_weapons356_xr_test.bat'),
    @('wrist',     'build_wrist360_xr_test.bat'),
    @('locker',    'build_locker351_xr_test.bat'),
    @('shotguns',  'build_shotguns318_native_test.bat'),
    @('sidearms',  'build_sidearms315_native_test.bat')
)
foreach ($e in $suites) {
    & cmd.exe /d /c ('"' + (Join-Path $PSScriptRoot ('tests\' + $e[1])) + '"') *> ('build\checks\' + $e[0] + '.txt'); $code = $LASTEXITCODE
    Write-Output ($e[0] + ': ' + $code); if ($code) { exit $code }
}
Write-Output 'All core checks passed.'
