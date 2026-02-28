@echo off

rem This .bat doesn't work on Proton 8.0, because its tasklist and findstr are stubs.
rem On Proton 8.0 use a custom exe program compiled from GGXrdLaunchWaiter.cpp instead.
rem Also, on Proton 8.0, && and || pipes work incorrectly. For example, the following code:
rem GGXrdLaunchWaiter && echo SUCCESS || echo FAIL
rem would output both SUCCESS and FAIL, which shouldn't be possible.
rem They seems to both work like the & pipe, which just chains commands.
rem So, instead use IF NOT ERRORLEVEL 1 DO (echo SUCCESS) ELSE (echo FAIL)
rem IF NOT ERRORLEVEL 1 checks if the last command succeeeded.

SET "CHECK_XRD=(tasklist /FI "IMAGENAME eq GuiltyGearXrd.exe" /FI "WINDOWTITLE eq Guilty Gear Xrd -REVELATOR-" | findstr GuiltyGearXrd.exe > NUL)"

%CHECK_XRD% && goto :finish
echo Waiting for Xrd to launch...
FOR /L %%I IN (1,1,30) DO (
  %CHECK_XRD% && goto :finish || (timeout /t 1 > NUL)
)
:finish
%CHECK_XRD% && echo "Xrd is running! (Do whatever you wanted to do.)" || echo Xrd didn't launch...