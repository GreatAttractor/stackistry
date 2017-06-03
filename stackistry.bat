set PATH=%PATH%;.\bin

REM Workaround for bug in GTK 3.22.4.1
set GDK_WIN32_DISABLE_HIDPI=1

start "" ".\bin\stackistry.exe"