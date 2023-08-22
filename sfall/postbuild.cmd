@ECHO OFF

rem debug, release, etc.
SET type=%1
rem full path to the compiled DLL
SET target=%2

::SET destination=d:\GAMES\Fallout2\@RP\ddraw.dll

SET pdb="%~dpn2.pdb"

IF EXIST ducible.exe (
    IF EXIST %pdb% (
        ducible %target% %pdb%
    ) ELSE (
        ducible %target%
    )
)

::echo Copying %target% to %destination% ...
::copy %target% %destination%