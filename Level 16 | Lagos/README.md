# Microcorruption CTF - Lagos (150 points)

## Problem

Same deal as always -- find an input that opens the door. This time, we're only allowed to input alphanumeric characters!

![manual](https://user-images.githubusercontent.com/86139991/126241233-fe9e69c3-8f37-4629-b83d-555e5c5099c4.PNG)

## Reasoning

## Alphanumerics only!

By and large, this is a step down in difficulty from the last two levels. We have our standard stack overflow: `0x10` bytes are allocated on the stack, but up to `0x200`
bytes are copied:

![overflow](https://user-images.githubusercontent.com/86139991/126241379-cc332daa-65ce-4daf-bd3c-05016d4f9c56.PNG)

There's just one catch: the bytes aren't copied directly to the stack. Instead, they're read one by one starting at `0x2400`, and copied only if they fall in one of
the following ASCII ranges:

- `0x30 - 0x39` (numbers)
- `0x41 - 0x5A` (uppercase letters)
- `0x61 - 0x7A` (lowercase letters)

This is actually not that big of a restriction. It means we can't include shellcode in our password, sure, but there are still plenty of return addresses we can use
made up entirely of alphanumeric characters.

## The alphanumeric ROP chain

For example, the very useful function `getsn` is mostly within the alphanumeric address range:

![getsn](https://user-images.githubusercontent.com/86139991/126241694-0aeda2cc-43dd-42d7-a51e-7a5af7ec3a40.PNG)

If we could call this function with our own arguments, we could make the program read in a new string that isn't limited to alphanumerics. If we read it into
an alphanumeric address, we could even return to that address afterwards.

We won't have control over `r14` or `r15`, but we don't need to. If we return directly to the middle of `getsn` at `0x6454` (right after `r14` and `r15` are pushed)
then the program will expect these values to already be on the stack. We will just include these values after the `getsn` return address, and the program will think
they're arguments.

A quick check to a legit call to `getsn` in the code shows that the intended size is in `r14`, and the address is in `r15`: 

![legit_getsn](https://user-images.githubusercontent.com/86139991/126241980-62d028f2-8a64-45ac-bb96-8f130c8b9b62.PNG)

Knowing all this, our plan of attack is clear:

- Overwrite the return pointer with the address in the middle of `getsn`
- Provide an alphanumeric length and writing address, so they get through the filter
- Write our shellcode to that alphanumeric address
- Finally, return to that alphanumeric address

Note that because of quirks with how the password is read in, we will have to provide `0x17` bytes of filler, not `0x16`.

## Fancy shellcode

Since this was so straightforward, let's have fun with some codegolfing. 

We could easily write our entire shellcode to an alphanumeric address that's unused (for example, `0x6161`). However, we can shorten our exploit by using a call to
`INT` that already exists in the alphanumeric range.

Let's use `getsn`, which has a call to `INT` at `4656`:

![getsn](https://user-images.githubusercontent.com/86139991/126242471-b022083f-bd26-49a9-97dc-a954d4848361.PNG)

If we can sneak a `push 0x7f` into `4654`, we can simply return to `4654` and use the call to `INT` so helpfully provided for us.

This may seem like an academic matter, but shortening exploits can be important. In many cases, you won't have the generous overflows or large buffers that are
common in CTFs. Learning tricks like this to shorten your code is a helpful skill in these matters.

## Solution

Password: `61616161616161616161616161616161615446544630305446` in hex.

Shellcode: `30127f00` in hex.
