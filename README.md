# TennaAsking

TennaAsking is a native C++ Valentine app for Windows. Hover over **NO** and it jumps away. Click **YES** to reveal the Valentine's Day message and play the celebration animation and music.

## Download

Download [TennaAsking.exe](https://github.com/Ralvi07/TennaAsking/releases/latest/download/TennaAsking.exe) and run it on 64-bit Windows. The executable includes its GIFs and MP3s.

## Build

Clone this repository and open its folder in an MSYS2 UCRT64 shell with GCC and binutils installed. All six media files used by the app are included in `resources/`.

```sh
windres TennaAsking.rc -O coff -o TennaAsking.res.o
g++ -std=c++17 -O2 -static -DUNICODE -D_UNICODE -municode -mwindows TennaAsking.cpp TennaAsking.res.o -o TennaAsking.exe -lgdiplus -lwinmm -lcomctl32 -lgdi32 -lshlwapi
```

## Run

Double-click `TennaAsking.exe` on Windows 11. The EXE embeds the GIFs and MP3s and runs without Java, the original installation, or a separate media folder.

The bundled media includes DELTARUNE material and music by Toby Fox.
