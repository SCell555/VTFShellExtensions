@echo off
cd /d %~dp0
regsvr32 /s VTFThumbnailProvider64.dll
regsvr32 /s VTFShellInfo64.dll
C:\Windows\SysWOW64\regsvr32 /s VTFThumbnailProvider32.dll
C:\Windows\SysWOW64\regsvr32 /s VTFShellInfo32.dll