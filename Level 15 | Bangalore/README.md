# Microcorruption CTF - Bangalore (100 points)

## Problem

Same deal as always -- find an input that opens the door. This time, the new wild card is DEP.

![manual](https://user-images.githubusercontent.com/86139991/126053187-73a9d3ab-6886-4d59-92cf-aa491a47f75a.PNG)

## Reasoning

## Data execution prevention

We have by far the simplest stack overflow we've seen in a while: `0x10` bytes are allocated on the stack, but `0x30` bytes are read:

![overflow](https://user-images.githubusercontent.com/86139991/126053192-b5a55c4c-7dd5-4dda-ab16-45f3a5811914.PNG)

However, there is a catch. Before `login` is run, the program iterates over all `0xff` "pages" of memory (a page in this case is `0x100` bytes; for example, the `0x3f`
page would be addresses `0x3f00` to `0x3fff`.) and sets each to be either writeable or executable using the functions `set_page_executable` and `set_page_writeable`:

- The first page, where the interrupts are called, is set to executable.
- Pages `0x01` through `0x43` (addresses `0x0100` through `0x43ff`) are set to writeable. This includes both the heap and, more importantly for us, the stack.
- All remaining pages (where the program is located) are set to executable.

![set_up_protection](https://user-images.githubusercontent.com/86139991/126053195-4f6243a0-520b-4e8f-a0a8-b94c00784e7f.PNG)

Since the stack is non-executable, standard shellcode won't work. We'll have to get clever.

## Breaking DEP

Let us imagine a world where this program is simpler. Specifically, if the code included an `unlock_door` function, we wouldn't have to worry about DEP at all. We
could just overwrite the return address with the address of `unlock_door`. Since  `unlock_door` would be at an address in the program, which is set to executable, 
this would work just fine. 

Unfortunately, there is no `unlock_door` function. However, there are ways we can use the existing program code (which is executable) to our advantage.

In fact, since our overflow is so large (`0x20` bytes beyond the bounds of the stack,) we can write several return addresses and call several parts of the existing code
one by one. This is called **return-oriented programming**, or **ROP**, and it is an extremely powerful tool.

Our first thought should be whether we can chain together bits of existing code to make our own `unlock_door` function. Unfortunately, this is not possible.

But wait -- what if we return to `set_page_executable` and change whether a page is executable on the fly? If we can safely set the part of the stack with our
shellcode to executable, we'd be set.

## Dancing around page boundaries

We are very lucky to avoid a problem that can occur when pages are either executable or writeable. Namely, if we set the whole stack to executable, it is no longer
writeable. *This causes a segfault the next time a function is called, since calling a function pushes a return address to the (now unwriteable) stack.*

Thankfully for us, our password is copied to the stack starting at address `3fee`. This means that, if we can increment the stack pointer such that the stack
is solely in the `0x40` page, we can set the `0x3f` page to executable without breaking anything.

## The payload

Since this is likely many readers' first encounter with DEP, let's go in depth on how to construct our ROP chain.

At the end of `main` (right before we go to our overwritten return address), the stack pointer is at `0x3fff`. To ensure the stack is always in the `0x40` page, the
first thing we will return to is a snippet of code that increases the stack pointer, then returns. A convenient example lies at the end of `turn_on_dep`:

![add_0x6](https://user-images.githubusercontent.com/86139991/126053199-39209a75-5ed1-4d32-9593-9b347ff417f1.PNG)

This means our first return address will be `0x44d8`. We will then put six bytes of filler, since these will be skipped over when `add 0x6, sp` is executed.

The next order of business is to set the `0x3f` page to executable. Let's see how it's done in `set_page_executable`:

![mark_page_executable](https://user-images.githubusercontent.com/86139991/126053201-ff69f9f5-deac-4261-b296-50352243a66c.PNG)

This function pushes two arguments to the stack: the number of the page to set (which is meant to be in `r14`, and an `0x00` indicating executable.) 
It then calls an interrupt. 

We can't easily control the contents of `r14`, (or `r15`, which is copied to `r14` at the start). However, we can simply put these two values on the stack ourselves
after the previous return address, then return *into the middle* of `set_page_executable` right after the values are pushed. The interrupt will then be called
using those values.

To do all this, we will pin the address right after the values are pushed onto our ROP chain (`0x44ba`). We will then place the two arguments: `0x003f`, the page number, and `0x0000`, to tell the interrupt to set this page to executable.

Once the `0x3f` page is executable, we can finally return to the address of our shellcode (which should be at the top of the password, at `0x3fee`), and we'll have
our exploit.

## The shellcode

Since there is no `INT` function in the code this time, this shellcode will look different to our previous levels. Our goal, as always, is to call `INT 0x7f`.
To figure out how to do that, let's check how this code calls interrupts against the manual.

![manual_interrupts](https://user-images.githubusercontent.com/86139991/126053204-fc742559-ab47-475f-bc9e-01a0639d95cb.PNG)

According to the manual, `getsn` is interrupt `0x02`, and `turn_on_dep` is interrupt `0x10`. Let's see how those functions call these interrupts:

![getsn](https://user-images.githubusercontent.com/86139991/126053207-56fd19cf-00b9-44f8-9d10-b65037b65077.PNG)

**getsn**

![turn_on_dep](https://user-images.githubusercontent.com/86139991/126053208-2841c714-fc75-4d9a-a897-652a209d6694.PNG)

**turn_on_dep**

The new syntax seems to be to subtract `0x6` from `sp`, set `sr` to `0x(80 + interrupt number)00`, then `call 0x0010`. Knowing this, we can construct our new shellcode
easily:

31800600 (`sub sp, 0x6`)
324000ff (`mov 0xff00, sr`)
b0121000 (`call 0x0010`)

This is only twelve bytes, so we can fit it between `0x3fee` and the non-executable `0x40` page if we put it at the top of our password. We can then pad to `0x10` bytes,
put our ROP chain at the end, and collect our payment.

## Solution

`31800600324000ffb0121000aaaaaaaad844bbbbbbbbbbbbba443f000000ee3f` in hex.
