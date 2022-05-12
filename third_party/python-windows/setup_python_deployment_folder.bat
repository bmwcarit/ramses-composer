@echo off
SET TARGET_FOLDER=%1%
REM Setup embeddable Python with pip and virtualenv using the instructions here: https://dev.to/fpim/setting-up-python-s-windows-embeddable-distribution-properly-1081
REM This uses the embeddable Python package for Windows and adds pip and virtualenv to it.
SET SCRIPT_FOLDER=%~d0%~p0
SET SOURCE_FOLDER=%SCRIPT_FOLDER%python-3.8.10-embed-amd64
ECHO Preparing Python deployment folder %TARGET_FOLDER% based on %SOURCE_FOLDER%
rd /s /q %TARGET_FOLDER%
XCOPY /S /E /I %SOURCE_FOLDER% %TARGET_FOLDER%
SET PYTHON_EXE=%TARGET_FOLDER%\python.exe
%PYTHON_EXE% "%SCRIPT_FOLDER%\get-pip.py"
copy /Y "%SCRIPT_FOLDER%\python38._pth_patched" "%TARGET_FOLDER%\python38._pth"
copy /Y "%SCRIPT_FOLDER%\sitecustomize.py" "%TARGET_FOLDER%\sitecustomize.py"
powershell expand-archive -Path '%TARGET_FOLDER%\python38.zip' -DestinationPath '%TARGET_FOLDER%\python38dir'
del "%TARGET_FOLDER%\python38.zip"
move "%TARGET_FOLDER%\python38dir" "%TARGET_FOLDER%\python38.zip"
%PYTHON_EXE% -m pip install virtualenv
REM We also need the DLLs folder - otherwise the creation of a virtualenv fails
mkdir "%TARGET_FOLDER%\DLLs"
REM Add a file indicating the build completed successfully
ECHO >"%TARGET_FOLDER%\python_setup_completed.txt" Python folder setup via %~nx0% completed
