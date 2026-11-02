param([string]$CC = 'C:/Strawberry/c/bin/gcc.exe')
$ErrorActionPreference = 'Stop'
Push-Location (Split-Path $PSScriptRoot -Parent)
try {
    New-Item -ItemType Directory -Force build | Out-Null
    & $CC -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/guitar.c tests/test_core.c -o build/test_core.exe
    if ($LASTEXITCODE -ne 0) { throw 'Host compilation failed' }
    & ./build/test_core.exe
    if ($LASTEXITCODE -ne 0) { throw 'Core tests failed' }
    & $CC -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/guitar.c src/usb_guard.c tests/test_usb.c -o build/test_usb.exe
    if ($LASTEXITCODE -ne 0) { throw 'USB test compilation failed' }
    & ./build/test_usb.exe
    if ($LASTEXITCODE -ne 0) { throw 'USB guard tests failed' }
    & $CC -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/http.c tests/test_http.c -o build/test_http.exe
    if ($LASTEXITCODE -ne 0) { throw 'HTTP test compilation failed' }
    & ./build/test_http.exe
    if ($LASTEXITCODE -ne 0) { throw 'HTTP tests failed' }
    & $CC -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/guitar.c tests/replay.c -o build/replay.exe
    if ($LASTEXITCODE -ne 0) { throw 'Replay compilation failed' }
    foreach ($capture in @(@('buttons',1),@('axes',2),@('chords',3))) {
        & ./build/replay.exe "tests/fixtures/$($capture[0]).bin" $capture[1]
        if ($LASTEXITCODE -ne 0) { throw "Replay failed: $($capture[0])" }
    }
} finally { Pop-Location }
