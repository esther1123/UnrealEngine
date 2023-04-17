@echo off

echo Copy GameLift's prerequisite files to WindowsServer folder...
copy /y Prerequisites\install.bat WindowsServer\install.bat
if not exist WindowsServer\install.bat (
    echo "install.bat does not exist in WindowsServer folder."
    timeout 10
    exit /b -1
)
copy /y Prerequisites\VC_Redist_2022_x64.exe.bak WindowsServer\VC_Redist_2022_x64.exe
if not exist WindowsServer\VC_Redist_2022_x64.exe (
    echo "VC_Redist_2022_x64.exe does not exist in WindowsServer folder."
    timeout 10
    exit /b -1
)
:: GameLift Server SDK 5.0.0 need libcrypto and libssl. You don't need there files if you are using 4.x SDK.
copy /y Prerequisites\libcrypto-3-x64.dll.bak WindowsServer\DayOne\Binaries\Win64\libcrypto-3-x64.dll
if not exist WindowsServer\DayOne\Binaries\Win64\libcrypto-3-x64.dll (
    echo "libcrypto-3-x64.dll does not exist in WindowsServer\DayOne\Binaries\Win64 folder."
    timeout 10
    exit /b -1
)
copy /y Prerequisites\libssl-3-x64.dll.bak WindowsServer\DayOne\Binaries\Win64\libssl-3-x64.dll
if not exist WindowsServer\DayOne\Binaries\Win64\libssl-3-x64.dll (
    echo "libssl-3-x64.dll does not exist in WindowsServer\DayOne\Binaries\Win64 folder."
    timeout 10
    exit /b -1
)
echo.

echo Fetch DayOne's latest Build Version in GameLift...
SET CURR_BUILD_VERSION=1.0.0
FOR /F "tokens=* USEBACKQ" %%F IN (`aws gamelift list-builds --query "reverse(sort_by(Builds[?Name==`DayOne`], &CreationTime))[0].Version" --output text`) DO (
    SET CURR_BUILD_VERSION=%%F
)
if "%CURR_BUILD_VERSION%"=="None" (
    SET NEW_PATCH_VERSION=0
) else (
    SET /a NEW_PATCH_VERSION=%CURR_BUILD_VERSION:~4,3% + 1
)
SET NEW_BUILD_VERSION=1.0.%NEW_PATCH_VERSION%
ECHO Current build version is %CURR_BUILD_VERSION%, raise the build version to %NEW_BUILD_VERSION%
echo.

echo Please wait while DayOneServer is being uploaded to GameLift...
aws gamelift upload-build --name DayOne --build-version %NEW_BUILD_VERSION% --build-root "WindowsServer" --operating-system WINDOWS_2012
echo.

echo Fetch DayOne's latest Build Id in GameLift...
SET BUILD_ID=build-
FOR /F "tokens=* USEBACKQ" %%F IN (`aws gamelift list-builds --query "reverse(sort_by(Builds[?Name==`DayOne`], &CreationTime))[0].BuildId" --output text`) DO (
    SET BUILD_ID=%%F
)
ECHO New Build Id is %BUILD_ID%
echo.

timeout /T 5 /NOBREAK > nul

echo Create fleet based on new Build Id...
aws gamelift create-fleet --name DayOne-%NEW_BUILD_VERSION% --build-id %BUILD_ID% --ec2-instance-type "c6i.8xlarge" --fleet-type SPOT --ec2-inbound-permissions "FromPort=7777,ToPort=7777,IpRange=0.0.0.0/0,Protocol=UDP" "FromPort=3389,ToPort=3389,IpRange=0.0.0.0/0,Protocol=TCP" --runtime-configuration "ServerProcesses=[{LaunchPath=C:\game\DayOne\Binaries\Win64\DayOneServer.exe,Parameters=-log=..\..\Binaries\Win64\DayOneGameLift.log -port=7777,ConcurrentExecutions=2}]"
echo.

pause
