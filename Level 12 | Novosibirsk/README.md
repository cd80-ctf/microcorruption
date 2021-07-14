# Microcorruption CTF - Novosibirsk (40 points)

## Problem

Same deal as always -- find an input that opens the door. We can only input a username (which is helpfully `printf`'d back to us), but Lockitall's gotten even better
at preventing stack overflows.

![manual](https://user-images.githubusercontent.com/86139991/125541327-0effa590-cbad-4e52-ac01-4b399b026be9.PNG)

## Reasoning

## The `printf` strikes back

It is very clear that overflows cannot be used to solve this level. Not only are there no variables we could overwrite on the stack, there isn't even a return address.
The entire program runs in `main`, which flows directly into `__stop_progExec__` when it is done:

![end_of_main](https://user-images.githubusercontent.com/86139991/125541787-49550687-4e0e-4f9d-ab17-aa06035dbaf4.PNG)

Thankfully, our password is `printf`'d back to us again, meaning we can again use a `printf` vulnerability. There's just one twist: this level uses `INT 0x7e` to unlock
the door, meaning there's no "password correct" byte we can overwrite.

Since `INT 0x7e` is called after the insecure `printf` and the program gives us a full `0x1f4` bytes of password, there is another easy overwrite target:
the argument passed to `INT`. This value is stored in the code segment in `conditional_unlock_door` at address `44c8`. If we can overwrite this with `0x7f`, then
`conditional_unlock_door` will unwittingly call `INT 0x7f`, unconditionally unlocking the door for us as described in the lock manual:

![0x7f](https://user-images.githubusercontent.com/86139991/125542102-f53176e6-152a-44ae-aca6-fd585d1c3e39.PNG)

Our plan of attack is very simple. We will start our password with the address of the `0x7e` value to overwrite (`44c8`.) We will then provide `0x7f - 0x02 = 0x7d`
bytes of filler to get the total bytes printed to `0x7f` (subtracting `0x02` accounts for printing the address at the start.) Finally, we will insert the format code
`%n`, causing `printf` to put the total number of bytes written so far (`0x7f`) into the address at the top of the stack (`44c8`). We will then sit back and collect
our money.

## Solution

In hex: `c844aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa`

The first two bytes are the address of the `0x7e` value we want to overwrite, while `256e` is the hex for "%n". Any 125 filler bytes in the middle will do.
