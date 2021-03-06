# Microcorruption CTF — Sydney (15 points)

## Problem

The previous program has been updated to avoid having the password in memory. Our objective is the same: find an input that opens the lock.

![manual](https://raw.githubusercontent.com/cd80-ctf/microcorruption/main/Level%202%20%7C%20Sydney/manual.PNG)

## Reasoning

### Password validation

In this case, the password is checked two bytes at a time against hardcoded values in `check_password`:

![check_password](https://raw.githubusercontent.com/cd80-ctf/microcorruption/main/Level%202%20%7C%20Sydney/check_password.PNG)

The only note of caution is that bytes are read in little-endian order, but compared in big-endian order. Since the word size on the lock is two,
this means every set of two hardcoded bytes must be reversed when they are provided as the password. Providing the hardcoded bytes from the `check_password` function
(with each pair of bytes reversed) successfully opens the lock.

## Solution

`5f4659785c715553` in hex
