echo copy to out sdk package
echo f|xcopy /y  .\LibNetworkServer.h                         .\OutNetworkServerSDK\include\
echo f|xcopy /y  .\NetworkSDK\include\tcpserverhandler.h      .\OutNetworkServerSDK\include\
echo f|xcopy /y  .\bin\LibNetworkServer*.lib                  .\OutNetworkServerSDK\lib\
echo f|xcopy /y  .\bin\LibNetworkServer*.dll                  .\OutNetworkServerSDK\bin\
echo f|xcopy /y  .\bin\LibNetworkServer*.pdb                  .\OutNetworkServerSDK\bin\
echo f|xcopy /y /s /i /f .\NetworkSDK\bin\*                   .\OutNetworkServerSDK\bin\