@echo off
echo ================================================
echo  WetReverb VST3 Plugin - Installation
echo ================================================
echo.

set PLUGIN_SOURCE=WetReverb\build\VST3\Release\WetReverb.vst3
set PLUGIN_DEST=%COMMONPROGRAMFILES%\VST3

if not exist "%PLUGIN_SOURCE%" (
    echo ERROR: Plugin not found at %PLUGIN_SOURCE%
    echo.
    echo Please run build.bat first to build the plugin.
    echo.
    pause
    exit /b 1
)

if not exist "%PLUGIN_DEST%" (
    echo Creating VST3 directory: %PLUGIN_DEST%
    mkdir "%PLUGIN_DEST%"
)

echo Installing WetReverb.vst3 to %PLUGIN_DEST%...
xcopy /E /I /Y "%PLUGIN_SOURCE%" "%PLUGIN_DEST%\WetReverb.vst3"

if errorlevel 1 (
    echo.
    echo ERROR: Installation failed
    echo You may need to run this script as Administrator.
    echo.
    pause
    exit /b 1
)

echo.
echo ================================================
echo  Installation successful!
echo ================================================
echo.
echo Plugin installed to: %PLUGIN_DEST%\WetReverb.vst3
echo.
echo Next steps:
echo 1. Restart your DAW if it's running
echo 2. Scan for new plugins (most DAWs do this automatically)
echo 3. Look for WetReverb in your plugin list
echo.

pause