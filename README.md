# TennaAsking

A small Windows Valentine app. The No button moves away when hovered. Choosing Yes displays the Valentine's Day message and disables both buttons.

## Build the native app

In an MSYS2 UCRT64 shell with GCC installed, run from the repository root.

```sh
g++ -std=c++17 -O2 -static -DUNICODE -D_UNICODE -municode -mwindows TennaAsking.cpp -o TennaAsking.exe -lgdiplus -lwinmm -lcomctl32 -lgdi32
```

The executable runs on Windows 11 without Java. With the original TennaAsking installation available, extract its local media beside the executable using the JDK's `jar` command from the project folder.

```powershell
jar xf 'C:/Program Files/TennaAsking/app/Project_Test.jar' resources
```

With no local media, the app uses solid backgrounds and runs silently. The GitHub source excludes the bundled animations and music.

## Recovered Java

`recovered/ValentineApp.java` was decompiled with CFR 0.152 from the single application class in `Project_Test.jar`. The original comments, formatting, and some local names cannot be recovered from the bytecode. The Java version requires JavaFX controls and media and the original resources on its classpath.
