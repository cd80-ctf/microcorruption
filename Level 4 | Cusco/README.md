# Microcorruption CTF — Cusco (25 points)

## Problem status

Same deal as always -- find an input that unlocks the door. This time, the patch notes tell us that the `password_correct` bit is now set in a register
rather than in memory, ensuring we cannot overwrite it.

![manual]()

## Reasoning

### Out-of-bounds copy

After ensuring that the `password_correct` bit was indeed absent from memory, the first obvious place to look was the `login` function. Immediately, we can see that
`getsn` is called with the stack pointer as an argument, indicating the password is being copied to the stack. 

![overflow]()

We can see that `0x30` bytes are copied to the stack, despite only `0x10` bytes being allocated at the start of `login`. This gives us a very simple stack overflow attack.
Rather than using shellcode, we can simply overwrite the return pointer with the address of `unlock_door`, which is visible and (presumably) the same for all locks:

![unlock_door]()

## Solution

Any string consisting of 16 bytes followed by the address of `unlock_door` will suffice. Recall that the bytes in the address of `unlock_door` must be input in reverse
order due to endian concerns.

We used the hex input `0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa4644`.
