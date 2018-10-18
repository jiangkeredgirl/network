set sln_name=network
set platform_version=141
cd /d %~dp0
call build_sln.bat "%VS141COMNTOOLS%",%sln_name%,%platform_version%

echo f|xcopy /y /s /i /f .\NetworkServer\OutNetworkServerBin\*               .\OutBinOfSln\
echo f|xcopy /y /s /i /f .\NetworkClient\OutNetworkClientBin\*               .\OutBinOfSln\
echo f|xcopy /y /s /i /f .\KlogSDK\bin\*                                     .\OutBinOfSln\
pause