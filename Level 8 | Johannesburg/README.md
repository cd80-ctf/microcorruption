# Microcorruption CTF - Johannesburg (20 points)

## Problem

Same deal as always -- find an input that opens the door. This time, the patch notes inform us that "passwords that are too long will be rejected".

![manual]()

## Reasoning

## Stack overflow and cookie

Checking the `login` function reveals a typical stack overflow: `0x12` bytes are allocated on the stack, but a password of length up to  `0x3f` is copied to the
stack pointer.

![overflow]()

This time, however, there's a hardcoded stack cookie. At the start of `login`, the lowest byte on the stack is set to `0x28`. At the end of the program, the lowest byte
is checked against `0x28`; if it does't match, the program exits immediately without returning.

![check_cookie]()

Since this is a hardcoded cookie that is presumably the same in all instances of the lock, it can easily be defeated by entering a password containing `0x11` bytes of
filler, the byte `0x28`, and finally the address to overwrite the return pointer. In this case, the lock's code contains the previously absent `unlock_door` function,
so its address (`4446`) can simply be hardcoded (in reverse order, remembering reverse endianness). Such an input easily bypasses the stack cookie check and opens the lock.

## Solution

Any `0x11` bytes of filler will do, but we used

`aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa284644` in hex.
