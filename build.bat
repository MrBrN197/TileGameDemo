@echo off

SET CommonCompilerFlags= /EHa /MTd /WX- /W1 /Od /Oi /FC /Fm /Gm /Gr /Zi -wd4208 -wd4189
SET CommonLinkerFlags=/INCREMENTAL:off /OPT:REF user32.lib gdi32.lib winmm.lib


CALL "C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Auxiliary\Build\vcvars64.bat"

rem return to current addres
CD %~dp0
pwd 

IF NOT EXIST build mkdir build

rem (x86 Build) cl %commonCompilerFlags% .\src\Win32.cpp /link -subsystem:windows,5.1 %commonLinkerFlags 


PUSHD ".\build\"
rm -f *.pdb
rm -f *.obj
echo WAITING FOR PDB > lock.tmp
cl %CommonCompilerFlags% /LDd ..\src\Game.cpp  /link /PDB:game%random%.pdb /EXPORT:GameUpdateAndRender /EXPORT:GameGetSoundSamples user32.lib
del lock.tmp
cl %CommonCompilerFlags% ..\src\Win32.cpp /link -OPT:REF user32.lib gdi32.lib winmm.lib
POPD

rem cl %CommonCompilerFlags Win32.cpp Game.cpp /link -subsystem:windows,5.1 %CommonLinkerFlags


rem /Gr = Enable Run Time Type Information
rem -MT = static runtime library
rem /WX[-] = [don't] treat warnings as errors
rem -W{n} = set warning level n
rem /Od = Disable Optimizations
rem -Oi = generate instrinsic functions
rem /FC = Displays the full path of source code files passed to cl.exe in diagnostic text.
rem -Fm = creates map file
rem /Gm = Minimal build(Deprecated)
rem /Zi = output debug info

rem (other options visual studio uses)
rem /Yu "engine_pch.h" = use precompiled header
rem /Fp"..\bin\Debug - windows - x86_64\PhysicsEngine\PhysicsEngine.pch " = creates a precompiled header file name
rem /Fo"..\bin\Debug - windows - x86_64\PhysicsEngine\" = creates an object file.
rem /Fa"..\bin\Debug-windows-x86_64\PhysicsEngine\" = Creates an assembly listing file.
rem /GS = Controls stack probesl
rem /W3 = Warning Level 3
rem /Zc:wchar_t = 
rem /I"src" = Searches a directory for include files.
rem /ZI = Includes debug information in a program database compatible with Edit and Continue. (x86 only)
rem /Fd"..\bin\Debug-windows-x86_64\PhysicsEngine\vc141.pdb" = Renames program database file.
rem /Zc:inline = Specifies standard behavior under /Ze.
rem /fp:precise = Specifies floating-point behavior.
rem /errorReport : prompt = Enables you to provide internal compiler error (ICE) information directly to the Microsoft C++ team.
rem /WX- = don't disable warnings
rem /Zc:forScope
rem /RTC1 = (Deprecated)
rem /Gd = Uses the __cdecl calling convention. (x86 only)
rem /std:c++ 14 = C++ standard version compatibility selector.
rem /EHsc = enable exceptions
rem /nologo = Suppresses display of sign-on banner.
rem /diagnostics:classic 

rem /LD[d] = create a [debug] dll. (sets /DLL in linker)
rem /MD[d] = Compiles to create a [debug] multithreaded DLL            , by using MSVCRTD.lib.
rem /MT[d] = Compiles to create a [debug] multithreaded executable file, by using LIBCMTD.lib.

rem /c = compile without linking
rem /link  = pass linker options
rem /E, /EP, /P = Preprocess without compiling or linking
