# Microcorruption CTF — Tutorial (10 points)

## Problem

We are given the assembly code of a program that runs on a lock. The goal is to determine some input that causes the lock to open.

## Reasoning

### Password Validation

Since the program is short, it is straightforward enough to start at `main`:

![main function](https://raw.githubusercontent.com/cd80-ctf/microcorruption/main/Level%200%20%7C%20Tutorial/main.PNG)

We see that `unlock_door` is called if the return value of `check_password` (stored in `r15`) is nonzero.

The code of `check_password` is as follows:

![check_password](https://raw.githubusercontent.com/cd80-ctf/microcorruption/main/Level%200%20%7C%20Tutorial/check_password.PNG)

The first four lines iterate over what is assumed to be a null-terminated string stored at the address in `r15`. For each character, *including the terminating byte*,
it increments a counter in `r12`. After reaching the null byte, this value (which will be the length of the string plus one) is compared to 9. If the length plus one is
9, `check_password` returns 1; otherwise, it returns 0.

Thus it seems that any password of length 8 will cause `check_password` to return 1, which will in turn cause `unlock_door` to be called.

## Solution

Any eight-character string will open the lock; the word `adequate` was chosen.
