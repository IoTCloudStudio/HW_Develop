@echo off
chcp 65001 >nul
title Ver resultado - Migracion ZTE MF193
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0instalar.ps1" -Check
