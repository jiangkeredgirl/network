@call "%VS141COMNTOOLS%VsDevCmd.bat"
cd /d %~dp0
MSBuild NetworkServer.vcxproj /m /target:Rebuild /p:VisualStudioVersion=14.1 /p:Configuration=Debug   /property:Platform=x86
MSBuild NetworkServer.vcxproj /m /target:Rebuild /p:VisualStudioVersion=14.1 /p:Configuration=Release /property:Platform=x86
MSBuild NetworkServer.vcxproj /m /target:Rebuild /p:VisualStudioVersion=14.1 /p:Configuration=Debug   /property:Platform=x64
MSBuild NetworkServer.vcxproj /m /target:Rebuild /p:VisualStudioVersion=14.1 /p:Configuration=Release /property:Platform=x64
pause