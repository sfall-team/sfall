@ECHO OFF

rem debug, release, etc.
SET type=%1
rem full path to the compiled DLL
SET target=%2

SET destination=C:\Games\Fallout2\ddraw.dll

SET pdb="%~dpn2.pdb"

IF EXIST ducible.exe (
    IF EXIST %pdb% (
        ducible %target% %pdb%
    ) ELSE (
        ducible %target%
    )
)

rem echo Copying %target% to %destination% ...
rem copy %target% %destination%