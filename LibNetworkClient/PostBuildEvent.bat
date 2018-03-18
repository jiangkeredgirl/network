echo copy to out sdk package
echo f|xcopy /y  .\LibNetworkClient.h                         .\OutNetworkClientSDK\include\
echo f|xcopy /y  .\NetworkSDK\include\tcpclienthandler.h      .\OutNetworkClientSDK\include\
echo f|xcopy /y  .\bin\LibNetworkClient*.lib                  .\OutNetworkClientSDK\lib\
echo f|xcopy /y  .\bin\LibNetworkClient*.dll                  .\OutNetworkClientSDK\bin\
echo f|xcopy /y  .\bin\LibNetworkClient*.pdb                  .\OutNetworkClientSDK\bin\
echo f|xcopy /y /s /i /f .\NetworkSDK\bin\*                   .\OutNetworkClientSDK\bin\