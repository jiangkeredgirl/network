echo copy to out sdk package
set toolset=%1
echo f|xcopy /y  .\tcpclient.h                     .\OutNetworkSDK\include\
echo f|xcopy /y  .\tcpclienthandler.h              .\OutNetworkSDK\include\
echo f|xcopy /y  .\tcpserver.h                     .\OutNetworkSDK\include\
echo f|xcopy /y  .\tcpserverhandler.h              .\OutNetworkSDK\include\
echo f|xcopy /y  .\TcpPackage.h                    .\OutNetworkSDK\include\
echo f|xcopy /y  .\bin\LibNetwork*.lib             .\OutNetworkSDK\lib\
echo f|xcopy /y  .\bin\LibNetwork*.dll             .\OutNetworkSDK\bin\
echo f|xcopy /y  .\bin\LibNetwork*.pdb             .\OutNetworkSDK\bin\
echo f|xcopy /y  ..\..\thirdparty\boost\lib\msvc-%toolset%\boost_system*.dll          .\OutNetworkSDK\bin\
echo f|xcopy /y  ..\..\thirdparty\boost\lib\msvc-%toolset%\boost_date_time*.dll       .\OutNetworkSDK\bin\
echo f|xcopy /y  ..\..\thirdparty\boost\lib\msvc-%toolset%\boost_regex*.dll           .\OutNetworkSDK\bin\