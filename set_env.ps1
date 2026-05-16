$newPath = Join-Path $PSScriptRoot "cppkg\target\build\Debug\"
echo $newPath
[System.Environment]::SetEnvironmentVariable("PATH", $env:PATH + ";$newPath", "User")
