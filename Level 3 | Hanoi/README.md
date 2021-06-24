# Microcorruption CTF — Hanoi (20 points)

## Problem

Same deal as always -- find an input to open the lock. The patch notes inform us that the password is now stored and checked on an external device, and will thus
no longer be showing up so conveniently in memory.

![manual](https://raw.githubusercontent.com/cd80-ctf/microcorruption/main/Level%203%20%7C%20Hanoi/manual.PNG)

## Reasoning

## Password checking

The first thing to notice is that most of the logic has been moved to the new `login` function. Quickly reading through the decompiled code reveals that the password
is read into memory at address `0x2400`, in response to a prompt that not-so-subtly reminds us that passwords must be under 16 characters in length:

![get_string](https://raw.githubusercontent.com/cd80-ctf/microcorruption/main/Level%203%20%7C%20Hanoi/get_string.PNG)

Reading further shows that external password checker indicates the password is valid by setting the byte at `0x2410` to `0x78`. If this value is found in memory,
the `login` function will unlock the door:

![compare_byte](https://raw.githubusercontent.com/cd80-ctf/microcorruption/main/Level%203%20%7C%20Hanoi/compare_byte.PNG)

No values are written into `0x2410` between inputting the password and checking this byte, and no length checks are done on the password. Thus, entering a 16 character
password followed by the byte `0x78` should unlock the door.

## Solution

Any password 17 bytes or longer will suffice, so long as the 17th byte is `0x78`. For our purposes, we used:

`aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa78` in hex.
