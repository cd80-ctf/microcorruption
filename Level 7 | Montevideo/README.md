# Microcorruption CTF - Montevideo (50 points)

## Problem

Same deal as always - find an input that opens the door. This time, the patch notes warn of 'unconfirmed reports' of the lock's vulnerability and ominously mention
that their code has been reimplemented per their "Secure Development Process."

![manual]()

## Reasoning

### Stack overflow

Apparently, this "Secure Development Process" implies replacing a `getsn` overflow with a `strcpy` overflow. The new login function begins as follows:

![new_login]()

There are four parts to this code: 

1. First, `0x10` bytes are allocated on the stack. 
2. Second, up to `0x30` bytes of password are read from the user via `getsn` and stored at `0x2400`.
3. Third, the password is copied from `0x2400` to the stack pointer using `strcpy`. **Note that strcpy stops at null bytes.**
4. Finally, all the bytes at `0x2400` are overwritten with zeroes using `memset`.

Given this, our plan of attack is pretty clear: write some shellcode that opens the lock (without using any null bytes), overwrite the return pointer with the stack pointer,
and profit. *Note that, despite the similarities, our solution from Level 6 will NOT work, because that shellcode included null bytes*.

### Shellcode

First, we need to find out what opens the lock. Reading the manual reveals that calling an interrupt with code `0x7f` will do what we want:

![7f_interrupt]()

Given this, the most obvious shellcode would be

```
3012 7f00 // push 0x7f
b012 4c45 // call 454c (INT)
```

(note that the address of INT has changed since Level 6)

Unfortunately, since the assembly for `push 0x7f` contains a null byte, this won't work. We need to find another way of getting `0x7f` on the stack without pushing
it directly.

One idea would be to first put `0x7f` in a register, then push the register to the stack:

```
3f40 7f00 // mov 0x7f, r15
0f12 // push r15
b012 4c45 // call 454c (INT)
```

This is closer to what we're looking for, but still contains a null byte.

The key insight is to put `0x007f` in a register using a two-step process. First, we can put `0xff7e` in the register (for our purposes, we used `r15`, but any register
works equally well). Then we can add `0x0101` to the value in `r15`. This sum comes out to `0xff7e + 0x0101 = 0x1007f`; **however, since the registers can only store
two bytes, the overflow is discarded and the register is set to `0x007f`.**

Once we have `0x7f` in `r15`, we can simply push it and call an interrupt just as above. (Note that `mov` and `add` both take numbers in reverse endian order.)

```
3f40 7eff // mov 0xff7e, r15
3f50 0101 // add 0x0101, r15
0f12 // push r15
b012 4c45 // call 454c (INT)
```

This should do the trick - and comes in at a total just 14 bytes, meaning we can fit it comfortably in the allocated stack space. All that's left for us to do is
fill the remiaining 2 bytes with padding, followed by the stack pointer (in reverse endian order), and the door opens right up.

## Solution
Given that the stack pointer is `43ee` and using `a`s for padding, we get

`3f407eff3f5001010f12b0124c45aaaaee43` in hex.
