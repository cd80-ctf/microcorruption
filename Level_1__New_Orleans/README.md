# Microcorruption CTF — New Orleans (10 points)

## Problem

We are given decompiled assembly of a program and a short description. Our goal is to find an input that causes the lock to open.

![manual](https://raw.githubusercontent.com/cd80-ctf/microcorruption/main/Level_1__New_Orleans/manual.PNG)

## Reasoning

### Password Validation

The `main` function is identical to the `main` from the tutorial. However, `check_password` is slightly different:

![check_password](https://raw.githubusercontent.com/cd80-ctf/microcorruption/main/Level_1__New_Orleans/check_password.PNG)

A pointer to the user-provided password is stored in `r15`, which is copied to `r13`. The copied password is then compared byte-by-byte to a value at address `0x2400`
in memory. Every correct byte increments a counter in `r14`. If this counter reaches eight (including the null byte at the end) and no incorrect bytes have been provided,
the function returns `1` and the lock opens.

Simply setting a breakpoint in `check_password` and looking at the memory at `0x2400` reveals the password in memory:

![password in memory](https://raw.githubusercontent.com/cd80-ctf/microcorruption/main/Level_1__New_Orleans/password_in_memory.PNG)

Running the program multiple times reveals that the password at `0x2400` is the same every time. Thus simply providing these bytes in hex will open the lock.

## Solution

`']K}(w` in ASCII

`275d4b7d522877` in hex
