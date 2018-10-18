rem @call "%VS141COMNTOOLS%VsDevCmd.bat"
rem cd /d %~dp0
rem MSBuild NetworkServer.vcxproj /t:rebuild /p:platform=win32 /p:configuration=debug   /p:PlatformToolset=v141
rem MSBuild NetworkServer.vcxproj /t:rebuild /p:platform=win32 /p:Configuration=release /p:PlatformToolset=v141
rem MSBuild NetworkServer.vcxproj /t:rebuild /p:platform=x64   /p:configuration=debug   /p:PlatformToolset=v141
rem MSBuild NetworkServer.vcxproj /t:rebuild /p:platform=x64   /p:Configuration=release /p:PlatformToolset=v141
rem pause
set project_name=NetworkServer
set platform_version=141
cd /d %~dp0
call build_project.bat "%VS141COMNTOOLS%",%project_name%,%platform_version%
pause