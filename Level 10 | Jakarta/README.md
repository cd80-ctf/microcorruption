# Microcorruption CTF - Jakarta (40 points)

## Problem

Same deal as always -- find an input that opens the door. Once again, we can input a username and a password, but this time, we are told that the sum of their lengths
cannot be too long

![manual](https://user-images.githubusercontent.com/86139991/125369625-9d3c6c80-e34a-11eb-95a5-691caed91c48.PNG)

## Reasoning

## Overflows & faulty length checks

The `login` function again has two stack overflows, although they are seemingly protected by length checks. First, `0x21` bytes are allocated on the stack (although
since a value is pushed beforehand, it's actually `0x23` bytes to the return address). Next, `0xff` bytes worth of username are read in.

![username_overflow](https://user-images.githubusercontent.com/86139991/125369991-76326a80-e34b-11eb-98ce-dc6ed4c8a62a.PNG)

This time, though, there's a catch. The length of the username is calculated next, and if the lower byte of the result is greater than `0x21`, the program immediately
exits and kills our chance at an overflow:

![username_length_check](https://user-images.githubusercontent.com/86139991/125370083-af6ada80-e34b-11eb-8a6d-c28a1e17f960.PNG)

If the username length is less than `0x21`, it is then stored in `r11`.

## Integer overflow

Next, the allowed password length is calculated by taking `0x1f` and subtracting the length of the password. **This is the critical bug**. A username of length `0x20`
passes the length check, but since `0x1f - 0x20 = 0xffff`, such a username causes an integer overflow in the calculated length of the password. The program then allows
us to read in a number of bytes equal to the calculated password length, which is now easily enough for an overflow. (The problem limits this length to `0x1ff`, 
inadvertently providing a massive hint that an integer overflow is the way to go).

![integer_overflow](https://user-images.githubusercontent.com/86139991/125370640-e7265200-e34c-11eb-9233-f1a5e6dabfd2.PNG)

The password length is then calculated and added to the username length in `r11`. The last bit of this sum is compared to `0x21` as well, with a hard exit if it is 
too large:

![integer_overflow_two](https://user-images.githubusercontent.com/86139991/125370898-8fd4b180-e34d-11eb-89a6-9eb8b2840964.PNG)

To satisfy this, we must make sure the sum of the username length (which must be `0x20`) and the password length is between `0x100` and `0x121`, so the
lower byte satisfies this length check. If we can do that, we can overwrite the return pointer with the address of `unlock_door` (which is present in the code at `0x444c`)
and collect our payment.

## Solution

Username: `aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa` in hex. (any `0x20` bytes of filler will do)

Password: `aaaaaaaa4c44bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb`
in hex. (three bytes to reach the return address, two bytes to overwrite it, and filler bytes to ensure the length sum is between `0x100` and `0x121`)
