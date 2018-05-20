echo copy to out bin package
echo f|xcopy /y  .\bin\NetworkServer*.exe                  .\OutNetworkServerBin\
echo f|xcopy /y  .\bin\NetworkServer*.pdb                  .\OutNetworkServerBin\
echo f|xcopy /y /s /i /f .\NetworkSDK\bin\*                .\OutNetworkServerBin\
echo f|xcopy /y /s /i /f .\NetworkSDK\bin\*                .\bin\
echo f|xcopy /y /s /i /f ..\KlogSDK\bin\*                  .\OutNetworkServerBin\
echo f|xcopy /y /s /i /f ..\KlogSDK\bin\*                  .\bin\