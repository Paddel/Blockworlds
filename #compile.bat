@echo off
color 0b

:: Check if we already have the tools in the environment
if exist "%VCINSTALLDIR%" (
	goto compile
)

:: Check for Visual Studio
if exist "%VS140COMNTOOLS%" (
	set VSPATH="%VS140COMNTOOLS%"
	goto set_env
)
if exist "%VS100COMNTOOLS%" (
	set VSPATH="%VS100COMNTOOLS%"
	goto set_env
)
if exist "%VS90COMNTOOLS%" (
	set VSPATH="%VS90COMNTOOLS%"
	goto set_env
)
if exist "%VS80COMNTOOLS%" (
	set VSPATH="%VS80COMNTOOLS%"
	goto set_env
)

echo You need Microsoft Visual Studio 8, 9 or 10 installed
pause

exit

:: Setup the environment
:set_env
if not exist %VSPATH%vsvars32.bat (
	color 0a
	cls
	echo.
	echo === An error occured! ===
	echo.
	pause >nul
	exit
) else call %VSPATH%vsvars32.bat

:type
@echo ..
set /p answer=compiling d/c/o/n/od:
cls
if "%answer%"=="d" (GOTO debug)
if "%answer%"=="c" (GOTO clear)
if "%answer%"=="o" (GOTO output)
if "%answer%"=="od" (GOTO outputdebug)
if "%answer%"=="n" (GOTO compile)
goto compile

:: Compile
:compile
echo.
@echo "<<<-START        Compiling by Paddel->>>"
@call bam.exe server_release -j 8
@call bam.exe client_release -j 8
@call bam.exe release -j 8
@echo "<<<- FINISHED    Compiling by Paddel->>>"
echo.
goto type

:debug
@echo DEBUG
echo.
@echo "<<<-START        Compiling by Paddel->>>"
@call bam.exe server_debug -j 8
@call bam.exe client_debug -j 8
@echo "<<<- FINISHED    Compiling by Paddel->>>"
echo.
goto type


:clear
@echo CLEAR
echo.
@echo "<<<-START        Compiling by Paddel->>>"
@call bam.exe debug -c all
@echo "<<<- FINISHED    Compiling by Paddel->>>"
echo.
goto type

:output
echo.
@echo "<<<-START        Compiling by Paddel->>>"
@call bam.exe release -j 8 > compile_out.txt
@echo "<<<- FINISHED    Compiling by Paddel->>>"
echo.
goto type

:outputdebug
echo.
@echo "<<<-START        Compiling by Paddel->>>"
@call bam.exe debug -j 8 > compile_out.txt
@echo "<<<- FINISHED    Compiling by Paddel->>>"
echo.
goto type
