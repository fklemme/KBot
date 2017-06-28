:: Build BWAPILIB
pushd %CD%
cd bwapi\bwapi
msbuild /p:Configuration=Debug   /p:Platform=Win32 /target:BWAPILIB bwapi.sln
msbuild /p:Configuration=Release /p:Platform=Win32 /target:BWAPILIB bwapi.sln
popd

:: Build BWAPIClient
pushd %CD%
cd bwapi\bwapi
msbuild /p:Configuration=Debug   /p:Platform=Win32 /target:BWAPIClient bwapi.sln
msbuild /p:Configuration=Release /p:Platform=Win32 /target:BWAPIClient bwapi.sln
popd

:: Build KBot
msbuild /p:Configuration=Debug   /p:Platform=x86 KBot.sln
msbuild /p:Configuration=Release /p:Platform=x86 KBot.sln
