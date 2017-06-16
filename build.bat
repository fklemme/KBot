:: Build BWAPI
pushd %CD%
cd bwapi\bwapi
msbuild /p:Configuration=Debug   /p:Platform=Win32 /target:BWAPI bwapi.sln
msbuild /p:Configuration=Release /p:Platform=Win32 /target:BWAPI bwapi.sln
popd

:: Build KBot
msbuild /p:Configuration=Debug   /p:Platform=x86 KBot.sln
msbuild /p:Configuration=Release /p:Platform=x86 KBot.sln
