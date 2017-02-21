@echo off
if exist C:\Windows\Sysnative\bash.exe (
	C:\Windows\Sysnative\bash.exe -c "make %*"
) else (
	C:\Windows\System32\bash.exe -c "make %*"
)
