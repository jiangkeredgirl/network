echo copy to out sdk package
echo f|xcopy /y  .\tcpclient.h                     .\OutNetworkSDK\include\
echo f|xcopy /y  .\tcpclienthandler.h              .\OutNetworkSDK\include\
echo f|xcopy /y  .\tcpserver.h                     .\OutNetworkSDK\include\
echo f|xcopy /y  .\tcpserverhandler.h              .\OutNetworkSDK\include\
echo f|xcopy /y  .\TcpPackage.h                    .\OutNetworkSDK\include\
echo f|xcopy /y  .\bin\LibNetwork*.lib             .\OutNetworkSDK\lib\
echo f|xcopy /y  .\bin\LibNetwork*.dll             .\OutNetworkSDK\bin\
echo f|xcopy /y  .\bin\LibNetwork*.pdb             .\OutNetworkSDK\bin\