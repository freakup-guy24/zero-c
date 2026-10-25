param(
    [Parameter(Mandatory = $true)]
    [string]$CompilerDirectory,

    [string]$InnoCompiler = ""
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location -LiteralPath $root
$build = Join-Path $root "build"
$bin = Join-Path $CompilerDirectory "bin"
$gcc = Join-Path $bin "g++.exe"
$windres = Join-Path $bin "windres.exe"

foreach ($required in @(
    (Join-Path $root "ZERO-C.cpp"),
    (Join-Path $root "ZERO-C.rc"),
    (Join-Path $root "zero-c.ico"),
    (Join-Path $root "ZERO-C-Setup.iss"),
    (Join-Path $root "C\TC\compat\conio.h"),
    (Join-Path $root "README.txt"),
    (Join-Path $root "THIRD-PARTY-NOTICES.txt"),
    $gcc,
    $windres
)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "Required build file not found: $required"
    }
}

if (-not $InnoCompiler) {
    $command = Get-Command "ISCC.exe" -ErrorAction SilentlyContinue
    if ($command) {
        $InnoCompiler = $command.Source
    }
}
if (-not $InnoCompiler -or -not (Test-Path -LiteralPath $InnoCompiler -PathType Leaf)) {
    throw "Inno Setup 7's ISCC.exe is required. Install it from https://jrsoftware.org/isinfo.php and pass -InnoCompiler with its full path."
}

New-Item -ItemType Directory -Path $build -Force | Out-Null
$resourceObject = Join-Path $build "ZERO-C-res.o"
$appExecutable = Join-Path $build "ZERO-C.exe"
& $windres "-i" "ZERO-C.rc" "-o" $resourceObject
if ($LASTEXITCODE -ne 0) {
    throw "Windows resource compilation failed with exit code $LASTEXITCODE."
}

& $gcc "-std=c++17" "-O2" "-Wall" "-Wextra" "-static" "-static-libgcc" `
    "-static-libstdc++" "-mwindows" "ZERO-C.cpp" $resourceObject `
    "-o" $appExecutable "-lcomctl32" "-lcomdlg32" "-lshell32"
if ($LASTEXITCODE -ne 0) {
    throw "ZERO C application compilation failed with exit code $LASTEXITCODE."
}

Copy-Item -LiteralPath (Join-Path $root "README.txt") -Destination (Join-Path $build "README.txt") -Force
Copy-Item -LiteralPath (Join-Path $root "THIRD-PARTY-NOTICES.txt") -Destination (Join-Path $build "THIRD-PARTY-NOTICES.txt") -Force
New-Item -ItemType Directory -Path (Join-Path $build "compat") -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $root "C\TC\compat\conio.h") -Destination (Join-Path $build "compat\conio.h") -Force

$dist = Join-Path $root "dist"
New-Item -ItemType Directory -Path $dist -Force | Out-Null
& $InnoCompiler "/Qp" "/DCompilerDir=$CompilerDirectory" (Join-Path $root "ZERO-C-Setup.iss")
if ($LASTEXITCODE -ne 0) {
    throw "Inno Setup failed with exit code $LASTEXITCODE."
}

$installer = Join-Path $dist "ZERO-C-Setup-1.0.1.exe"
if (-not (Test-Path -LiteralPath $installer -PathType Leaf)) {
    throw "Installer output was not created: $installer"
}
$checksum = (Get-FileHash -LiteralPath $installer -Algorithm SHA256).Hash.ToLowerInvariant()
Set-Content -LiteralPath "$installer.sha256" -Value "$checksum  ZERO-C-Setup-1.0.1.exe" -Encoding ASCII
Write-Output "Build complete: $installer"
Write-Output "SHA-256: $checksum"
