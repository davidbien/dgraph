# Microsoft Developer Studio Project File - Name="gr_test" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=gr_test - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "gr_test.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "gr_test.mak" CFG="gr_test - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "gr_test - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "gr_test - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "gr_test - Win32 Release debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 1
# PROP Scc_ProjName ""$/dv/dslib", FSAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "gr_test - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /Ob2 /D "NDEBUG" /D "__DEBUG_THROW" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "__HAVE_ALLOC_TRAITS" /D "__GR_DEFINEOLEIO" /D "__DBIEN_LARGEINTEGER" /D "__DBIEN_ULARGEINTEGER" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "gr_test - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "c:\dvnew\stl32" /D "_DEBUG" /D "__DEBUG_THROW" /D "__DBIEN_ULARGEINTEGER" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "__HAVE_ALLOC_TRAITS" /D "__GR_DEFINEOLEIO" /D "__DBIEN_LARGEINTEGER" /YX /FD /GZ /Qwd985 /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ELSEIF  "$(CFG)" == "gr_test - Win32 Release debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "gr_test___Win32_Release_debug"
# PROP BASE Intermediate_Dir "gr_test___Win32_Release_debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "gr_test___Win32_Release_debug"
# PROP Intermediate_Dir "gr_test___Win32_Release_debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /Ob2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "__HAVE_ALLOC_TRAITS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /Zi /O2 /Ob2 /D "NDEBUG" /D "__NDEBUG_THROW" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "__HAVE_ALLOC_TRAITS" /D "__GR_DEFINEOLEIO" /D "__DBIEN_LARGEINTEGER" /D "__DBIEN_ULARGEINTEGER" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386

!ENDIF 

# Begin Target

# Name "gr_test - Win32 Release"
# Name "gr_test - Win32 Debug"
# Name "gr_test - Win32 Release debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\_gr_outp.cpp
# End Source File
# Begin Source File

SOURCE=.\_gr_test.cpp
# End Source File
# Begin Source File

SOURCE=.\_gr_todo.cpp

!IF  "$(CFG)" == "gr_test - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "gr_test - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "gr_test - Win32 Release debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\dbgthrw.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\_allbase.h
# End Source File
# Begin Source File

SOURCE=..\memlib\_aloctrt.h
# End Source File
# Begin Source File

SOURCE=..\stdlib\_basemap.h
# End Source File
# Begin Source File

SOURCE=..\_bitutil.h
# End Source File
# Begin Source File

SOURCE=..\_booltyp.h
# End Source File
# Begin Source File

SOURCE=..\stdlib\_booltyp.h
# End Source File
# Begin Source File

SOURCE=..\_dbgthrw.h
# End Source File
# Begin Source File

SOURCE=..\stdlib\_debug.h
# End Source File
# Begin Source File

SOURCE=..\stdlib\_dtorp.h
# End Source File
# Begin Source File

SOURCE=..\stdlib\_fcdtor.h
# End Source File
# Begin Source File

SOURCE=..\_garcoll.h
# End Source File
# Begin Source File

SOURCE=.\_gr_alst.h
# End Source File
# Begin Source File

SOURCE=.\_gr_copy.h
# End Source File
# Begin Source File

SOURCE=.\_gr_cpwt.h
# End Source File
# Begin Source File

SOURCE=.\_gr_def.h
# End Source File
# Begin Source File

SOURCE=.\_gr_disn.h
# End Source File
# Begin Source File

SOURCE=.\_gr_dtor.h
# End Source File
# Begin Source File

SOURCE=.\_gr_dump.h
# End Source File
# Begin Source File

SOURCE=.\_gr_gitr.h
# End Source File
# Begin Source File

SOURCE=.\_gr_inc.h
# End Source File
# Begin Source File

SOURCE=.\_gr_inpt.h
# End Source File
# Begin Source File

SOURCE=.\_gr_inpw.h
# End Source File
# Begin Source File

SOURCE=.\_gr_iter.h
# End Source File
# Begin Source File

SOURCE=.\_gr_ole.h
# End Source File
# Begin Source File

SOURCE=.\_gr_olio.h
# End Source File
# Begin Source File

SOURCE=.\_gr_outp.h
# End Source File
# Begin Source File

SOURCE=.\_gr_pitr.h
# End Source File
# Begin Source File

SOURCE=.\_gr_rndm.h
# End Source File
# Begin Source File

SOURCE=.\_gr_safe.h
# End Source File
# Begin Source File

SOURCE=.\_gr_shdo.h
# End Source File
# Begin Source File

SOURCE=.\_gr_shwr.h
# End Source File
# Begin Source File

SOURCE=.\_gr_sitr.h
# End Source File
# Begin Source File

SOURCE=.\_gr_sril.h
# End Source File
# Begin Source File

SOURCE=.\_gr_stin.h
# End Source File
# Begin Source File

SOURCE=.\_gr_stio.h
# End Source File
# Begin Source File

SOURCE=.\_gr_titr.h
# End Source File
# Begin Source File

SOURCE=.\_gr_trt.h
# End Source File
# Begin Source File

SOURCE=.\_gr_tst0.h
# End Source File
# Begin Source File

SOURCE=.\_gr_type.h
# End Source File
# Begin Source File

SOURCE=.\_graph.h
# End Source File
# Begin Source File

SOURCE=..\_oleutil.h
# End Source File
# Begin Source File

SOURCE=..\stdlib\_ref.h
# End Source File
# Begin Source File

SOURCE=..\_sdp.h
# End Source File
# Begin Source File

SOURCE=..\_sdpd.h
# End Source File
# Begin Source File

SOURCE=..\_sdpn.h
# End Source File
# Begin Source File

SOURCE=..\_sdpv.h
# End Source File
# Begin Source File

SOURCE=..\stdlib\_simpbv.h
# End Source File
# Begin Source File

SOURCE=..\stdlib\_swapobj.h
# End Source File
# Begin Source File

SOURCE=..\stdlib\_ticont.h
# End Source File
# Begin Source File

SOURCE=..\stdlib\_typemap.h
# End Source File
# Begin Source File

SOURCE=..\bienutil.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=.\makefile

!IF  "$(CFG)" == "gr_test - Win32 Release"

!ELSEIF  "$(CFG)" == "gr_test - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "gr_test - Win32 Release debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\makendbg

!IF  "$(CFG)" == "gr_test - Win32 Release"

!ELSEIF  "$(CFG)" == "gr_test - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "gr_test - Win32 Release debug"

!ENDIF 

# End Source File
# End Target
# End Project