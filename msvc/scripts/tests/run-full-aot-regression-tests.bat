@ECHO OFF

SET TEMP_PATH=%PATH%
SET PATH=
SET MONO_RESULT=1

CALL setup-env.bat
IF NOT ERRORLEVEL == 0 (
	ECHO Failed to setup mono paths.
	GOTO ON_ERROR
)

CALL setup-toolchain.bat
IF NOT ERRORLEVEL == 0 (
	ECHO Failed to setup toolchain.
	GOTO ON_ERROR
)

SET FULLAOT_DIR=%MONO_WINAOT_BUILD_DIR%
SET MONO_PATH=%FULLAOT_DIR%

SET RUN_TARGET=%1

REM %FULLAOT_DIR%\basic-simd.exe not in full AOT profile on Windows.

IF "all" == "%RUN_TARGET%" (
	SET RUN_TARGET=%FULLAOT_DIR%\basic.exe ^
	%FULLAOT_DIR%\basic-float.exe ^
	%FULLAOT_DIR%\basic-long.exe ^
	%FULLAOT_DIR%\basic-calls.exe ^
	%FULLAOT_DIR%\objects.exe ^
	%FULLAOT_DIR%\arrays.exe ^
	%FULLAOT_DIR%\basic-math.exe ^
	%FULLAOT_DIR%\exceptions.exe ^
	%FULLAOT_DIR%\iltests.exe ^
	%FULLAOT_DIR%\devirtualization.exe ^
	%FULLAOT_DIR%\generics.exe ^
	%FULLAOT_DIR%\aot-tests.exe ^
	%FULLAOT_DIR%\gshared.exe ^
	%FULLAOT_DIR%\ratests.exe ^
	%FULLAOT_DIR%\unaligned.exe ^
	%FULLAOT_DIR%\builtin-types.exe
) ELSE (
	IF NOT EXIST %RUN_TARGET% (
		SET RUN_TARGET=%FULLAOT_DIR%\%1
	)
)

REM Debug output options.

REM SET MONO_LOG_LEVEL=debug
REM SET MONO_LOG_MASK=asm,aot

SET MONO_LOG_LEVEL=
SET MONO_LOG_MASK=

FOR %%a IN (%RUN_TARGET%) DO (

	ECHO %MONO_AOT_RUNTIME_EXECUTABLE% --full-aot %%a --exclude "!FULLAOT" --exclude "!FULLAOT-AMD64".
	%MONO_AOT_RUNTIME_EXECUTABLE% --full-aot %%a --exclude "!FULLAOT" --exclude "!FULLAOT-AMD64"

	IF NOT ERRORLEVEL == 0 (
		ECHO Failed Full AOT execute of %%a.
		GOTO ON_ERROR
	)

)

GOTO ON_EXIT

:ON_ERROR
	ECHO Failed Full AOT execute.
	SET MONO_RESULT=ERRORLEVEL
	GOTO ON_EXIT

:ON_EXIT
	SET PATH=%TEMP_PATH%
	EXIT /b %MONO_RESULT%

@ECHO ON

