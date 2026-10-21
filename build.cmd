@echo off
setlocal
rem All entry points use the same PowerShell build, including exit codes.
rem Default EXE is release-only; explicit -Test uses build/tests separately.
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0build.ps1" %*
set "buildResult=%errorlevel%"
endlocal & exit /b %buildResult%
