# Microsoft Developer Studio Project File - Name="GCompDLL" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=GCompDLL - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "GCompDLL.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "GCompDLL.mak" CFG="GCompDLL - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "GCompDLL - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "GCompDLL - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "GCompDLL - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GCOMPDLL_EXPORTS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I ".." /I "..\common" /I "..\storage" /I "..\metadata" /I "..\forms" /I "..\ui" /I "..\DLL" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GCOMPDLL_EXPORTS" /D "_WINDLL" /D "_AFXDLL" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 version.lib /nologo /dll /machine:I386 /out:"../Release/GComp.dll"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "GCompDLL - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GCOMPDLL_EXPORTS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I ".." /I "..\common" /I "..\storage" /I "..\metadata" /I "..\forms" /I "..\ui" /I "..\DLL" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GCOMPDLL_EXPORTS" /D "_WINDLL" /D "_AFXDLL" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 version.lib /nologo /dll /debug /machine:I386 /out:"../Debug/GComp.dll" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "GCompDLL - Win32 Release"
# Name "GCompDLL - Win32 Debug"
# Begin Group "Main"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\GComp.cpp
# End Source File
# Begin Source File

SOURCE=.\GComp.h
# End Source File
# Begin Source File

SOURCE=.\GComp.rc
# End Source File
# End Group
# Begin Group "Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\CommonFunctions.cpp
# End Source File
# Begin Source File

SOURCE=..\common\CommonFunctions.h
# End Source File
# Begin Source File

SOURCE=..\common\Declarations.h
# End Source File
# Begin Source File

SOURCE=..\common\Lexer.cpp
# End Source File
# Begin Source File

SOURCE=..\common\Lexer.h
# End Source File
# Begin Source File

SOURCE=..\storage\Storage.cpp
# End Source File
# Begin Source File

SOURCE=..\storage\Storage.h
# End Source File
# Begin Source File

SOURCE=..\storage\StreamInfo.cpp
# End Source File
# Begin Source File

SOURCE=..\storage\StreamInfo.h
# End Source File
# Begin Source File

SOURCE=..\common\Strings.cpp
# End Source File
# Begin Source File

SOURCE=..\common\Strings.h
# End Source File
# Begin Source File

SOURCE=..\common\Yaml.cpp
# End Source File
# Begin Source File

SOURCE=..\common\Yaml.h
# End Source File
# End Group
# Begin Group "MMS"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\storage\Container.cpp
# End Source File
# Begin Source File

SOURCE=..\storage\Container.h
# End Source File
# Begin Source File

SOURCE=..\metadata\FileParser.cpp
# End Source File
# Begin Source File

SOURCE=..\metadata\MMS.cpp
# End Source File
# Begin Source File

SOURCE=..\metadata\MMS.h
# End Source File
# Begin Source File

SOURCE=..\metadata\MMS_Assemble.cpp
# End Source File
# Begin Source File

SOURCE=..\metadata\MMS_Basic.cpp
# End Source File
# Begin Source File

SOURCE=..\metadata\MMS_Basic.h
# End Source File
# Begin Source File

SOURCE=..\metadata\MMS_BasicTypes.cpp
# End Source File
# Begin Source File

SOURCE=..\metadata\MMS_BasicTypes.h
# End Source File
# Begin Source File

SOURCE=..\metadata\MMS_Buh.cpp
# End Source File
# Begin Source File

SOURCE=..\metadata\MMS_Buh.h
# End Source File
# Begin Source File

SOURCE=..\metadata\MMS_Calculation.cpp
# End Source File
# Begin Source File

SOURCE=..\metadata\MMS_Calculation.h
# End Source File
# Begin Source File

SOURCE=..\metadata\MMS_Compile.cpp
# End Source File
# Begin Source File

SOURCE=..\metadata\MMS_Decompile.cpp
# End Source File
# Begin Source File

SOURCE=..\metadata\MMS_Document.cpp
# End Source File
# Begin Source File

SOURCE=..\metadata\MMS_Document.h
# End Source File
# Begin Source File

SOURCE=..\metadata\MMS_Filter.cpp
# End Source File
# Begin Source File

SOURCE=..\metadata\MMS_Filter.h
# End Source File
# Begin Source File

SOURCE=..\metadata\MMS_Parser.cpp
# End Source File
# Begin Source File

SOURCE=..\metadata\MMS_Subconto.cpp
# End Source File
# Begin Source File

SOURCE=..\metadata\MMS_Subconto.h
# End Source File
# Begin Source File

SOURCE=..\metadata\NameTable.cpp
# End Source File
# Begin Source File

SOURCE=..\metadata\NameTable.h
# End Source File
# Begin Source File

SOURCE=..\storage\Ordering.cpp
# End Source File
# Begin Source File

SOURCE=..\storage\Ordering.h
# End Source File
# Begin Source File

SOURCE=..\storage\PictureGallery.cpp
# End Source File
# End Group
# Begin Group "Dialogs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\forms\DialogForm.cpp
# End Source File
# Begin Source File

SOURCE=..\forms\DialogForm.h
# End Source File
# Begin Source File

SOURCE=..\forms\DlgForm_Tables.cpp
# End Source File
# End Group
# Begin Group "UserInterfaces"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\ui\UI.CPP
# End Source File
# Begin Source File

SOURCE=..\ui\UI.H
# End Source File
# Begin Source File

SOURCE=..\ui\UI_Compile.cpp
# End Source File
# End Group
# Begin Group "UserRights"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\ui\Rights.cpp
# End Source File
# Begin Source File

SOURCE=..\ui\Rights.h
# End Source File
# Begin Source File

SOURCE=..\ui\Rights_Compile.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=..\metadata\1CVS_Tables.cpp
# End Source File
# End Target
# End Project
