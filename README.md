# TennaAsking

TennaAsking is a native C++ Valentine app for Windows. Hover over **NO** and it jumps away. Click **YES** to play the celebration animation and music and reveal the Valentine's Day message.

## Run on Windows

Double-click `TennaAsking.exe` after building it. The standalone EXE includes all three animations and music tracks. You can copy it to another Windows 11 computer and run it without Java or a separate media folder.

## Build from source

1. From the project folder, extract the media from the original installation using the JDK's `jar` command in PowerShell.

   ```powershell
   jar xf 'C:/Program Files/TennaAsking/app/Project_Test.jar' resources
   ```

2. Open an MSYS2 UCRT64 shell with GCC and binutils installed. From the project folder, compile the resource file and build the EXE.

   ```sh
   windres TennaAsking.rc -O coff -o TennaAsking.res.o
   g++ -std=c++17 -O2 -static -DUNICODE -D_UNICODE -municode -mwindows TennaAsking.cpp TennaAsking.res.o -o TennaAsking.exe -lgdiplus -lwinmm -lcomctl32 -lgdi32 -lshlwapi
   ```

The original JavaFX app and its media are available in the separate [TennaAsking-Java repository](https://github.com/Ralvi07/TennaAsking-Java).
