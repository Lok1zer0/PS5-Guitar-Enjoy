$ErrorActionPreference='Stop'
Push-Location (Split-Path $PSScriptRoot -Parent)
try {
    $required=@('dist/ps5-guitar-diagnostic.elf','dist/SHA256SUMS','dist/ps5-payload-sdk-v0.45-source.zip')
    foreach($file in $required){if(!(Test-Path -LiteralPath $file)){throw "Missing: $file"}}
    $root='dist/package-v0.1.0/PS5-Guitar-Enjoy'
    New-Item -ItemType Directory -Force "$root/dist" | Out-Null
    foreach($dir in @('src','web','tests','tools','docs')){Copy-Item -LiteralPath $dir -Destination $root -Recurse -Force}
    foreach($file in @('README.md','TESTE_NO_PS5.md','RELATORIO_VIABILIDADE.md','THIRD_PARTY.md','LICENSE')) {
        Copy-Item -LiteralPath $file -Destination $root -Force
    }
    foreach($file in $required){Copy-Item -LiteralPath $file -Destination "$root/dist" -Force}
    $package='dist/ps5-guitar-diagnostic-v0.1.0.zip'
    Compress-Archive -Path $root -DestinationPath $package -Force
    $hash=Get-FileHash -Algorithm SHA256 $package
    "$($hash.Hash.ToLower())  ps5-guitar-diagnostic-v0.1.0.zip" | Set-Content dist/PACKAGE_SHA256SUMS -Encoding ascii
    Get-Item $package | Select-Object Name,Length
    Get-Content dist/PACKAGE_SHA256SUMS
} finally {Pop-Location}
