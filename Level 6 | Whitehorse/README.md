# Microcorruption CTF - Whitehorse (50 points)

## Problem
Same deal as always - find an input to open the lock. This time, the patch notes tell us that the `unlock_door` function has been removed.

![manual]()

## Reasoning

### Stack overflow

A quick check of `login` tells us that the stack overflow from Level 4 is still present; that is, `0x10` bytes are allocated for the stack, but `0x30` bytes of password
are copied to the stack pointer.

![overflow]()

This gives us the ability to overwrite the return pointer. However, since `unlock_door` is no longer present in memory, we must figure out something else to overwrite
the return pointer with.

### Shellcode

Since we have 16 bytes to work with, the first natural idea is to see if we can hide a door-unlocking function in the password, then overwrite the return pointer
with the address of the password.

The [lock manual](https://microcorruption.com/manual.pdf) tells us that calling an interrupt with the code `0x7f` and no arguments will unlock the door:

![interrupt]()

This seems simple enough to fit in 16 bytes. We can use the segment of `conditional_unlock_door` where an interrupt is called with code `0x7e` as reference:

![interrupt_in_conditional]()

Based on this, our shellcode will look something like this:

```
3012 7f00 // push 0x7f
b012 3245 // call 4532 (INT)
```

This is eight total bytes of shellcode. We can append eight extra trash bytes to fill the password buffer, then append the stack pointer (where the password is stored).
Doing this successfully redirects control flow and opens the lock.

(thankfully, the `getsn` function which copies the password does not terminate on null bytes; if it did, this would be much more difficult).

## Solution
`30127f00b0123245aaaaaaaaaaaaaaaa103c` in hex. (note that the 8 padding bytes in the middle are irrelevant)
