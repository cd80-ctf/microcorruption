# Microcorruption CTF - Addis Ababa (50 points)

## Problem

Same deal as always -- find an input that opens the door. This time, we can only input a password (which is helpfully `printf`'d back to us), but Lockitall has
gotten serious about length checks.

![manual](https://user-images.githubusercontent.com/86139991/125538370-f55782bf-e804-442f-8a06-cb95016070e8.PNG)

## Reasoning

## A new vulnerability

This time, the length check finally seems to be sufficient -- `0x16` bytes are allocated on the stack, and we are only allowed to copy `0x13` to `sp + 2`:

![no_overflow](https://user-images.githubusercontent.com/86139991/125538476-2725b8e5-6364-4ce2-8afd-5dd0eb24cfce.PNG)

The system checks if our password, and sets a bit at `sp` if it is valid. Note that this is above our password on the stack, so we cannot overwrite it with the
password itself. If this were the entire program, it would be pretty secure. However, another vulnerability is introduced when our password (stored at the pointer in
`r11`) is `printf`'d back at us:

![printf](https://user-images.githubusercontent.com/86139991/125538742-168fd5db-f675-4c56-8594-b390c28aa0e6.PNG)

Since this happens after the "password valid" bit is set at `sp` but before that bit is checked, our intuition is to use a `printf` vulnerability to overwrite
this bit with a nonzero value. The program will then believe this nonzero bit was returned from the `check_password` function and open the door for us.

## The `printf` exploit.

In many programs compiled with older C compilers, it is possible to use a `printf` of a user-provided string to overwrite a value at any location in memory. This is done
courtesy of the very strange `%n` format value, which sets the value at the pointer at the top of the stack to the number of bytes written so far:

![%n](https://user-images.githubusercontent.com/86139991/125539076-d1cc3da7-e34d-48f7-a0d0-8fa288d098cd.PNG)

Normally, the values to be formatted with `%` format values would be in a null-terminated list on top of the stack. If there are more `%` format values than elements
of this null-terminated list, however, the system will start using values below that list on the stack. Conveniently, in this version of `printf`, the string to be
printed is directly below this list.

Our strategy will be as follows:

- First, print the address of the "password valid" bit in hex. This will serve two purposes. First, it will cause the number of bytes written to be `0x02`. Second, it
will place this address right below the null-terminated list. We will use this later to write values into it.
- Second, provide a useless format code. This will pop the null-terminator of the list off the stack, leaving our string (and thus the address of the "password valid"
byte) on top.
- Finally, we will put the `%n` format code. This will put the number of bytes written so far (`0x02`) into the address at the top of the stack (the address of the
"password valid" byte.)

Once we have placed `0x02` in the "password valid" byte, we can sit back and watch the program open the door for us.

## Solution

`d83e2578256e` in hex. The first two bytes are the address of the "password valid" byte, while `2578` and `256e` are hex for "%x" and "%n" respectively.
