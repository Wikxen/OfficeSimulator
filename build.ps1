param([switch]$Test)

$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    $gcc = (Get-Command gcc -CommandType Application | Select-Object -First 1).Source
    $target = & $gcc -dumpmachine
    if ($LASTEXITCODE -ne 0 -or $target -notlike 'x86_64-*mingw*') {
        throw 'Use an x64 MinGW-w64 GCC toolchain (for example, w64devkit).'
    }

    $flags = @('-std=c17', '-O2', '-Wall', '-Wextra', '-Werror', '-static')
    New-Item -ItemType Directory -Force .\dist | Out-Null
    & $gcc @flags -s -mwindows -o .\dist\OfficeSimulator.exe .\office.c .\sim.c .\save.c -lgdi32 -lshell32
    if ($LASTEXITCODE -ne 0) { throw 'Game build failed.' }

    if ($Test) {
        New-Item -ItemType Directory -Force .\build | Out-Null
        & $gcc @flags -I. -o .\build\sim-tests.exe .\tests\sim_tests.c .\sim.c
        if ($LASTEXITCODE -ne 0) { throw 'Test build failed.' }
        & .\build\sim-tests.exe
        if ($LASTEXITCODE -ne 0) { throw 'Simulation tests failed.' }
    }
}
finally {
    Pop-Location
}
