param(
    [string]$Sdk = 'C:/Users/cabre/.codex/toolchains/ps5-v0.45/ps5-payload-sdk',
    [string]$Llvm = 'C:/Users/cabre/.codex/toolchains/ps5-v0.45/clang+llvm-18.1.8-x86_64-pc-windows-msvc/bin'
)
$ErrorActionPreference='Stop'
Push-Location (Split-Path $PSScriptRoot -Parent)
try {
    foreach($required in @("$Sdk/target/lib/crt1.o","$Llvm/clang.exe","$Llvm/ld.lld.exe")) {
        if(!(Test-Path -LiteralPath $required)){throw "Missing toolchain file: $required"}
    }
    New-Item -ItemType Directory -Force build/generated,dist | Out-Null
    $html=[IO.File]::ReadAllText((Join-Path (Get-Location) 'web/index.html'))
    # ASCII C source with UTF-8 octal bytes, no runtime filesystem assets.
    $encoded=([Text.Encoding]::UTF8.GetBytes($html) | ForEach-Object { '\'+[Convert]::ToString($_,8).PadLeft(3,'0') }) -join ''
    [IO.File]::WriteAllText((Join-Path (Get-Location) 'build/generated/ui.h'), "static const char GH_UI[] = `"$encoded`";`n",[Text.Encoding]::ASCII)
    $sources=@('guitar','usb_guard','usb_ps5','http','main')
    $objects=@()
    foreach($source in $sources) {
        $obj="build/$source.o";$objects+=$obj
        & "$Llvm/clang.exe" -target x86_64-sie-ps5 -std=c11 -D_BSD_SOURCE -O2 -g -Wall -Wextra -Werror -fPIC -fno-stack-protector -fno-plt -femulated-tls -ffunction-sections -fdata-sections -isystem "$Sdk/target/include" -Isrc -Ibuild/generated -c "src/$source.c" -o $obj
        if($LASTEXITCODE -ne 0){throw "PS5 compilation failed: $source"}
    }
    & "$Llvm/ld.lld.exe" -m elf_x86_64 -pie --eh-frame-hdr -z max-page-size=0x4000 -T "$Sdk/ldscripts/elf_x86_64.x" --hash-style=gnu --gc-sections --no-undefined --strip-debug '-Map=build/diagnostic.map' -o dist/ps5-guitar-diagnostic.elf @objects "$Sdk/target/lib/crt1.o" -L "$Sdk/target/lib" -lc -lkernel_sys -lSceLibcInternal -lSceNet -lSceSystemService -lSceUserService
    if($LASTEXITCODE -ne 0){throw 'PS5 link failed'}
    & "$Llvm/llvm-readelf.exe" -h -l -d dist/ps5-guitar-diagnostic.elf | Out-File build/elf-inspection.txt -Encoding utf8
    if($LASTEXITCODE -ne 0){throw 'ELF inspection failed'}
    $hash=Get-FileHash -Algorithm SHA256 dist/ps5-guitar-diagnostic.elf
    "$($hash.Hash.ToLower())  ps5-guitar-diagnostic.elf" | Set-Content dist/SHA256SUMS -Encoding ascii
    Get-Item dist/ps5-guitar-diagnostic.elf | Select-Object Name,Length
    Get-Content dist/SHA256SUMS
} finally {Pop-Location}
