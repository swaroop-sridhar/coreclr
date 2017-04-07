@echo off
dumpbin /headers %1 | findstr /C:"Certificates Directory
