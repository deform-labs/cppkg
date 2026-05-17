$buildPath = if (Test-Path "$PSScpkriptRoot\cppkg\target\build\Debug") {
    "$PSScriptRoot\cppkg\target\build\Debug"
} else {
    "$PSScriptRoot\cppkg\target\build"
}
[System.Environment]::SetEnvironmentVariable("PATH", $env:PATH + ";$buildPath", "User")
