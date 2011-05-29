@echo off
del ..\releases\current.zip
copy obj\win32\release\sleepy.exe
..\tools\7za a ..\releases\current.zip license.rtf sleepy.exe keywords.txt osfunctions.txt osmodules.txt
del sleepy.exe

