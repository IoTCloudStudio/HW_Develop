@echo off
chcp 65001 >nul
title Migracion de router - ZTE MF193
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0instalar.ps1"
