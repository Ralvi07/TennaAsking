# TennaAsking

A small Windows Valentine app. The No button moves away when hovered. Choosing Yes displays the Valentine's Day message and disables both buttons.

## Build the native app

Extract the original media into the project folder with the JDK's `jar` command.

```powershell
jar xf 'C:/Program Files/TennaAsking/app/Project_Test.jar' resources
```

Then run these commands from the project folder in an MSYS2 UCRT64 shell with GCC and binutils installed.

```sh
windres TennaAsking.rc -O coff -o TennaAsking.res.o
g++ -std=c++17 -O2 -static -DUNICODE -D_UNICODE -municode -mwindows TennaAsking.cpp TennaAsking.res.o -o TennaAsking.exe -lgdiplus -lwinmm -lcomctl32 -lgdi32 -lshlwapi
```

The EXE embeds all six media files and runs on Windows 11 without Java or a separate media folder. Git excludes the original media and the built EXE.

## Recovered Java

`recovered/ValentineApp.java` was decompiled with CFR 0.152 from the single application class in `Project_Test.jar`. The original comments, formatting, and some local names cannot be recovered from the bytecode. The Java version requires JavaFX controls and media and the original resources on its classpath.
