@call "%VS141COMNTOOLS%VsDevCmd.bat"
cd /d %~dp0
MSBuild network.sln /t:rebuild /p:platform=win32 /p:configuration=debug   /p:PlatformToolset=v141
MSBuild network.sln /t:rebuild /p:platform=win32 /p:Configuration=release /p:PlatformToolset=v141
MSBuild network.sln /t:rebuild /p:platform=x64   /p:configuration=debug   /p:PlatformToolset=v141
MSBuild network.sln /t:rebuild /p:platform=x64   /p:Configuration=release /p:PlatformToolset=v141
echo f|xcopy /y /s /i /f .\NetworkServer\OutNetworkServerBin\*               .\OutBinOfSln\
echo f|xcopy /y /s /i /f .\NetworkClient\OutNetworkClientBin\*               .\OutBinOfSln\
pause