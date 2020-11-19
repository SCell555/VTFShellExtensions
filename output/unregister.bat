@echo off
cd /d %~dp0
regsvr32 /s /u VTFThumbnailProvider64.dll
regsvr32 /s /u VTFShellInfo64.dll
C:\Windows\SysWOW64\regsvr32 /s /u VTFThumbnailProvider32.dll
C:\Windows\SysWOW64\regsvr32 /s /u VTFShellInfo32.dll