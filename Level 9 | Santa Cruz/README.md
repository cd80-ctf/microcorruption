# Microcorruption CTF - Santa Cruz (50 points)

## Problem

Same deal as always -- find an input that opens the door. This time, several more complex checks have been added to make sure invalid passwords are rejected. This time,
we are also required to input a username!

![manual](https://user-images.githubusercontent.com/86139991/125367790-ce1aa280-e346-11eb-810b-1d360bc464ff.PNG)

## Reasoning

## Stack overflow with two variables

This `login` function is much more complex than ones we've faced in the past. The fatal flaw, however, is still there: only `0x2c` bytes are allocated on the stack,
but up to `0x63` bytes of username are read in:

![overflow](https://user-images.githubusercontent.com/86139991/125367782-ca871b80-e346-11eb-8882-934d86842708.PNG)
We are allowed to put in a password and a username. The username is copied to the address `sp - 0x2a`, while the password is copied to `sp - 0x13`. The fact that there
are two variables here will allow us to bypass the much more extensive length checks this level puts us through.

## Input validation

Before the username and password are read in, the following reference variables are stored on the stack: 

- `0x08` is stored at `sp - 0x19`
- `0x10` is stored at `sp - 0x18`
- `0x00` is stored at `sp - 0x02`

![reference_values](https://user-images.githubusercontent.com/86139991/125367775-c6f39480-e346-11eb-9f38-e63fecdb5ecd.PNG)

After both the username and password are copied, these are put into use. First, the password length is calculated by starting at `sp - 0x13` and iterating until a null
byte is found:

![password_length_calculation](https://user-images.githubusercontent.com/86139991/125367768-c2c77700-e346-11eb-9e3c-da67dcf062c5.PNG)


This length is then compared to the values at `sp - 0x19` and `sp - 0x18` (remember, these are `0x08` and `0x10` by default). If the calculated length is less than the
value at `sp - 0x19` or more than the value at `sp - 0x18`, then the program exits immediately, thwarting our overflow attempt:

![length_checks](https://user-images.githubusercontent.com/86139991/125367718-b0e5d400-e346-11eb-80f1-bced39860be7.PNG)

After these checks are done, our password is checked against the stored password using `0x7d`. Before returning, however, there is one last check. The value at `sp - 0x02`
is checked to make sure it's still `0x00` (which it was set to earlier); if it isn't, the program again exits prematurely.

![check_against_zero](https://user-images.githubusercontent.com/86139991/125367707-a9262f80-e346-11eb-9893-538ce4057226.PNG)


## Overflows

The first strategy one would think of would be to use the username to overwrite the return pointer with the address of `unlock_door` (which is still in the code at `0x444a),
then use the password to insert a null byte in the middle at `sp - 0x02`. However, this presents an issue. Since the password is stored at `sp - 0x13` and has a default
max length of `0x10`, and the null byte needs to be inserted at `sp - 0x02`, using the password to overwrite this would give the password a length of `0x11`. This would
cause the program to exit immediately.

To get around this, we can also use the username to overwrite the reference values for minimum and maximum length at `sp - 0x19` and `sp - 0x18`. The easiest method
here would be to replace them with `0x01` and `0xff` respectively, since our password length will certainly be within these bounds. After doing this, we can overwrite
the return pointer with the username, input a `0x11` character long password to set the null byte at `0x02`, and open the door.

To summarize, our strategy is:

- Input a long username that overwrites the 'minimum length' value at `sp - 0x19` with `0x01`, the 'maximum length' value at `sp - 0x18` with `0xff`, and the stored
return pointer `0x2c` bytes after the username with the address of `unlock_door` (`444a`)
- Input a `0x11`-byte password to set the null byte at `0x02`
- ????
- Profit


## Solution

Username: `aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa01ffaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa4a44` in hex. (`0x01` and `0xff` can be replaced with any values below
and above `0x11` respectively`.

Password: `bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb` in hex. (any `0x11`-byte string will do)
