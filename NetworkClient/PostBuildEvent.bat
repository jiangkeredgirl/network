echo copy to out bin package
echo f|xcopy /y  .\bin\NetworkClient*.exe                  .\OutNetworkClientBin\
echo f|xcopy /y  .\bin\NetworkClient*.pdb                  .\OutNetworkClientBin\
echo f|xcopy /y /s /i /f .\NetworkSDK\bin\*                .\OutNetworkClientBin\
echo f|xcopy /y /s /i /f .\NetworkSDK\bin\*                .\bin\