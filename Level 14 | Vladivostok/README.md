# Microcorruption CTF - Vladivostok (100 points)

## Problem

Same deal as always -- find an input that opens the door. We're back to using the stack on this challenge, but a new challenge has been introduced -- ASLR.

![manual](https://user-images.githubusercontent.com/86139991/125876158-c891183b-faea-4b9e-b867-7581d0faae79.PNG)

## Reasoning

## Address randomization

This version of the lock starts by moving the entire program (as a whole) to a random offset in memory. This offset is generated from `rand` every time the program is run,
ensuring we cannot predict it beforehand:

![aslr](https://user-images.githubusercontent.com/86139991/125876208-8d9d73c8-e1de-4894-8161-c6b3b6b0e830.PNG)

The stack is also moved to a randomly generated offset from the program code. This means that (spoilers) even if we discover an address in the code, it will not help us
figure out the address of the stack.

This randomization, known as Address Space Layout Randomization (or ASLR) is a standard security measure in common devices. In a vacuum, it prevents us from overwriting
any function addresses with addresses of code in the program or on the stack, since we won't know where the program or the stack are stored in memory.

## Breaking ASLR

The most common way to break ASLR is to somehow get the program to tell you an address in the program or the stack. *Since the entire program is moved as a whole,
figuring out any address in the program will let us deduce any other address in the program, since the difference between the addresses of two points in the code does
not change.*

For example, say we can get the program to tell us that the randomized address of `main` is `0x1000`, and we want to figure out the address of `conditional_unlock_door`.
By looking at the code before running the program, we know that the default address of `main` is `4438`:

![main_addr](https://user-images.githubusercontent.com/86139991/125876791-75c1a30b-11d2-4940-9483-7fc946e75d0f.PNG)

and the default address of `conditional_unlock_door` is `4a42`:

![cad_addr](https://user-images.githubusercontent.com/86139991/125876819-2547fc44-0ddd-4006-accd-cc73ec60a54a.PNG)

From this, we can figure out that the *offset* from `main` to `conditional_unlock_door` is `4a42 - 4438 = 5fa`.

Here is the key: since ASLR moves the entire program as a chunk, the offset between any two functions does not change. Therefore, since the program has told us that
the randomized address of `main` is `1000`, *we can easily deduce that the randomized address of `conditional_unlock_door` is `1000 + 5fa = 15fa`.*

Being able to discover the true address of any function is extremely important. However, it is not the only step to our exploit. We still have to find another bug
that allows us to overwrite some function pointer in memory with the function we want to redirect control flow to.

## Dynamically finding the exploit

### Leaking an address

Static analysis of this code is quite hard, since everything has been made more complicated to account for the ASLR. To find an exploit, we must resort to the
time-honored hacking trick of entering random nonsense.

Let's run the program and enter a random long username:

![long_username](https://user-images.githubusercontent.com/86139991/125877309-6e1745b2-ab3f-4436-813c-c680feab168a.PNG)

Checking out the program's memory immediately afterwards shows us that only the first `0x08` bytes of the username are read, and that they are stored at `0x2402`:

![username_in_memory](https://user-images.githubusercontent.com/86139991/125877397-ee0e83f4-c45b-4fe9-9670-e07c9d999b52.PNG)

Next, we can step through the program using the `s` instruction. Doing this shows that the username is never copied to the stack, so a stack overflow
with the username is off the table. However, we do discover something exciting: the username is printed back to us. If you've been paying attention over the past
several levels, this should immediately make you think about `printf` vulnerabilities.

In fact, `printf` vulnerabilities are a powerful tool in breaking ASLR. This is because, if our username includes format characters such as `%x`, the program will
start printing items on the stack. If one of those items happens to be an address of something on the stack or in the code, then we're in business.

Let's run the program again, this time using `%x%x%x%x` as our username:

![leaked_address](https://user-images.githubusercontent.com/86139991/125877728-058326b0-b870-497e-a139-15ea018f51e8.PNG)

The second word here seems interesting. Checking the registers immediately after printing this reveals that this is only a few hundred bytes below `pc` (the instruction
pointer). This strongly suggests that *this is an address in the program*. This is a big deal. It means that, if we can find a way to overwrite the return pointer
or a function pointer, we can execute to any function in the program.

### And a stack overflow to boot

Let's continue with our dynamic analysis by inputting a very long password:

![long_password](https://user-images.githubusercontent.com/86139991/125878023-8d051a7e-e7a6-4aaa-8697-55d96c3b7987.PNG)

Checking memory afterwards shows us that the first `0x12` bytes of the password are copied to the stack. This is promising. Could we have a stack overflow? Let's
let the program finish to find out:

![overwritten_pc](https://user-images.githubusercontent.com/86139991/125878154-dca226ca-f82f-46a2-9489-a2dc0e0dbd67.PNG)

This is exactly what every hacker wants to see: the program has crashed due to a bad instruction address, and the instruction pointer (`pc`) appears to be
taken from our password. Let's put in a pattern to see exactly where we overwrite the return pointer:

![password_pattern](https://user-images.githubusercontent.com/86139991/125878291-5d6d0f8c-586e-49bb-bfee-49a3d6dfe4e3.PNG)

Once again, the program crashes, and this time we see that `pc` has become `aa99`:

![aa99](https://user-images.githubusercontent.com/86139991/125878351-c59acbd3-2a88-48c5-ae84-bdb0741e71eb.PNG)

Since these are the ninth and tenth bytes of our pattern, this suggests that we overwrite the return pointer after `0x08` bytes. Perfect! Now we just need to figure out
how to open the lock using functions in the code, and we're set.

### The convenient `_INT`

Unfortunately, there is no `unlock_door` in this version of the code. However, there is an `_INT` function that seems to call an interrupt with whatever code is at
`sp + 0x2`:

![_int](https://user-images.githubusercontent.com/86139991/125878541-d1c7798d-0c70-4473-8a5e-0b38cfb056ae.PNG)

If we can call this function and have `sp + 0x2 = 0x7f00`, we'll have our exploit. Can we do that?

Of course we can! Recall that the return pointer is overwritten after `0x8` bytes of password, and that we can write up to `0x12` bytes. After a function returns, 
the new stack pointer is just the word immediately below the return address on the stack. That means if we overwrite the return address with the address of `_INT`,
`sp` will be set to the `0xa`th byte of our password, and `sp + 0x2` will be the `0xc`th and `0xd`th bytes. 

We're almost there. There's just one step left: calculating the randomized address of `_INT`. 

Calculating this address is fairly easy. Before running the code, we take note of the first ten bytes of `_INT`'s code:

![_int](https://user-images.githubusercontent.com/86139991/125878541-d1c7798d-0c70-4473-8a5e-0b38cfb056ae.PNG)

In this case, these bytes are `1e41 0200 0212 8f10 024f`. We will use these bytes as a fingerprint to find `_INT` in the randomized code.

Say we run the program again and the leaked address is `0x1000`. We can then search through the memory (using CTRL-F) to find the first several bytes of `_INT`.
In this case, we would find these bytes starting at address `0x15fa`. Since ASLR copies code as one big block, this means that, when we get the leaked address,
we can simply add `0x15fa - 0x1000 = 0x05fa` to get the address of `_INT`.

Knowing all this, here is our plan of attack:

- Enter `%x%x%x%x` as our username, causing the program to leak an address in the program. Use this to calculate the randomized address of `_INT`.
- For our password, use eight filler bytes, followed by the calculated address of `_INT`, followed by two filler bytes, and finally followed by `7f00`.

If you made it this far, congratulations! You've created your first two-step exploit. Since essentially all modern exploits have multiple steps to them,
learning how to create chains of bugs like this will serve you well in your future endeavors.

## Solution

Username: `%x%x` in ASCII, `25782578` in hex. 

Password: Eight bytes of filler, followed by the calculated address of `_INT`, followed by two bytes of filler, followed by `7f00`
