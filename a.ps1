Invoke-WebRequest -UseBasicParsing -Uri
"https://github.com/Kitware/CMake/releases/download/v3.30.2/cmake-3.30.2-windows-x86_64.zip" -OutFile
"$env:TEMP\cmake.zip"