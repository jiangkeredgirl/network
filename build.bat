@call "%VS141COMNTOOLS%VsDevCmd.bat"
cd /d %~dp0
MSBuild network.sln /m /target:Rebuild /p:VisualStudioVersion=14.1 /p:Configuration=Debug   /property:Platform=x86
MSBuild network.sln /m /target:Rebuild /p:VisualStudioVersion=14.1 /p:Configuration=Release /property:Platform=x86
MSBuild network.sln /m /target:Rebuild /p:VisualStudioVersion=14.1 /p:Configuration=Debug   /property:Platform=x64 
MSBuild network.sln /m /target:Rebuild /p:VisualStudioVersion=14.1 /p:Configuration=Release /property:Platform=x64
echo f|xcopy /y /s /i /f .\NetworkServer\OutNetworkServerBin\*               .\OutBinOfSlnBuild\
echo f|xcopy /y /s /i /f .\NetworkClient\OutNetworkClientBin\*               .\OutBinOfSlnBuild\
pause