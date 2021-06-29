# Microcorruption CTF - Reykjavik (35 points)

## Problem

Same deal as always -- find the input that opens the door.

This time, however, the code has significantly changed. The majority of the code is encrypted, and would be nearly impossible to crack with just static analysis.
Luckily for us, we've got a full-fledged debugger. This will serve as Microcorruption's introduction to dynamic analysis.

![manual](https://raw.githubusercontent.com/cd80-ctf/microcorruption/main/Level%205%20%7C%20Reykjavik/manual.PNG)

## Reasoning

The main function serves to de-encrypt the meat of the code, which is then stored at `0x2400` and called:

![main](https://raw.githubusercontent.com/cd80-ctf/microcorruption/main/Level%205%20%7C%20Reykjavik/main.PNG)

In order to actually see this code, we set a breakpoint at `call 0x2400` and manually stepped through the decrypted code. This code simply sets up the password prompt
and calls an interrupt. Entering a noticable pattern (in our case, `0x112233445566778899`), shows that the password is simply copied to the stack. The decrypted code
then simply compares the first two bytes of the password to `0xbaae`. If the comparison succeeds, the interrupt `0x7f` is called, which the manual tells us unlocks the door.

## Solution

In hex: `aeba`.
