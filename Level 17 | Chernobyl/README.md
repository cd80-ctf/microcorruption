# Microcorruption CTF - Chernobyl (300 points)

## Problem

Same deal as always -- find an input that opens the door. This is one of the two final levels, where things get *really* interesting. If you can keep up and complete
this one, you'll (at time of writing) be in the top 1,000 of the 80,000 who've attempted this CTF.

![manual]()

## Reasoning

## The account manager

The new manual brags about the LockIt Pro Account Manager, a new system which can store hundreds of users and their PINs. When we run the program, we are prompted
to enter such a username and PIN:

![input_1]()

Entering random data for these will clearly get us nowhere. Let's look at the code to see what it's actually doing.

The bulk of the program is inside the `run` function, which is an infinite loop. This function allocates `0x600` bytes on the stack, and on each iteration reads in `0x550`
bytes. This, combined with the fact that `run` doesn't even have a `ret` instruction, means that looking for stack overflows in this function will be unfruitful.

Further inspection, however, reveals another command that the input prompt didn't tell us about. The function seems to check for a seven-letter word starting with "a" --
this is "access" -- but also checks for a three-letter word starting with "n". Looking at the data section, we find several strings related to adding accounts. Thus we
can guess that this keyword is intended to be "new", and will have the syntax "new [username] [PIN]".

![strings]()

Our first thought would obviously be to add an account and then immediately try to log in with it. However, this produces a new error message:

![not_activated]()

We'll either have to figure out how to activate accounts, or find another way to open the door.

## Activating accounts is useless

We can quickly determine that "activating" accounts is unhelpful. Checking the code for instances of the string "Access granted" (which we find in the data section)
shows that this doesn't actually open the door; in fact, it seems the loop just continues is normal. If we want to open this lock, we'll have to dig into how the
Account Manager actually works. This will not be easy, but will end with us developing by far the most realistic exploit of the CTF thus far.

## The hash table

Looking at what `run` does after getting the "new" command reveals that it adds new accounts to a hash table. The hash table is keyed by the username, and its values
are simple tuples with `0x10` bytes for the username and two for the PIN.

In order to progress further, we'll need to understand the basics of how a hash table actually works.

### Hash table internals

Normal lists store values in sequential order and can only find any specific value by iterating over the entire list. Hash tables, on the other hand, store items at
different locations in memory based on what the *hash* of their key is. This allows the table to drastically decrease the search space when looking up values: instead
of iterating over every value it has, it only has to iterate over values whose keys have the same hash as the lookup key.

Internally, this looks like this:

- No hash table uses the full result of the hash of a key. Instead, they take the modulus of the hash. In our case, the hash table uses the key hash modulo `0x8`
to determine where to put a value.
- For each value modulo `0x8`, the hash table creates a list (on the heap) where it will store values whose keys have that specific hash. For example, a key with
a hash that's 4 mod 8 will be stored in a list made specifically for objects whose key hashes are 4 mod 8.
- When too many items are added to the hash table, it undergoes a change known as a **rehash**. In a rehash: 
  - all the values mod 8 get new, bigger lists,
  - the old values are copied into these new, bigger lists, and
  - *the old, small lists are free()*'d

## The hash table's fatal flaw

If we look at the heap, we can see the heap chunks that the Account Manager allocates for the different lists in its hash map. Note that the `user` object we created
earlier is in the first list and takes up `0x12` bytes:

![hash_buckets]()

Further examining the `add_to_table` function reveals that a counter in memory is incremented every time the function is called. If this counter exceeds some
calculated value, the `rehash` function is called. Setting a breakpoint at this step reveals that, by default, up to `0xa` values can be stored in the hashmap before
a rehash is done.

Attentive readers may notice that this picture, the size of a `user` object, and the maximum values before rehash combine to spell out a serious vulnerability.

What is it? First, note that each `user` object is `0x12` bytes long, and up to `0xa` user objects can be added before the lists are expanded. Second, notice that
*there are only `0x60` bytes between two hash lists*.

Since these hash lists are heap objects, this means that *if we can submit enough values that go in the same hash list, we might be able to overwrite the heap
metadata at the start of the next list.* This will be the key observation for our exploit. In short, our battle plan is:

- Add several users whose usernames are grouped in the same hash list, overwriting the heap metadata at the start of the next list
- Add even more users to force a `rehash`, wherein the old hash lists (including our corrupted list) will be `free`'d
- Use this corrupted `free` to overwrite a return pointer with the address of some shellcode
- Profit

## Creating collisions

In order to easily create different usernames which fall in the same hash bucket, we created a simple Python script. This script takes the username (as a list of bytes),
computes its hash, and outputs where it will be placed in memory given how many elements are already in the hashmap. The full script can be found in `main.py`.

Let's try to add enough elements to the first hash list (starting at `0x503c`) to overwrite the headers of the second hash list (starting at `0x509c`). Before adding
our values, the hash lists look like this:

![hash_lists_pre_exploit]

After adding several values which our script confirms will land in the same list, they look like this (note our username/PIN pairs `0x10` bytes apart are highlighted
in red, and the vulnerable heap metadata is in green):

![hash_lists_almost_exploit]

Beautiful! The username of the next value we add to this list will start right on top of the next list's metadata.

## Heap corruption

We will use the same heap corruption exploit we used in ![Level 13](https://github.com/cd80-ctf/microcorruption/tree/main/Level%2013%20%7C%20Algiers). Namely, we will
overwrite the `previous chunk` value with an address `0x4` bytes before the return address of `free` on the stack, and the `size` value with the offset from the
return address to our shellcode, minus `0x6`. Since all addresses are aligned on two-byte boundaries, `free` will think the return address is the size of a free chunk
and will try to merge it with our corrupted chunk by adding the sizes (plus `0x6` bytes for the second chunk's now-useless metadata).

However, there are three problems with doing this exploit the exact same way. First off, while we set the `next chunk` value in Lesson 13 to a random address of zeroed-
out memory, doing this and then triggering a rehash will cause this program to overwrite our corrupted headers with legit ones. What's going on?

Well, unlike in the first program where no new chunks were `malloc`'d before our corrupted chunk is freed, the `rehash` tries to `malloc` twice before `free`ing
anything. When this happens, the zero lower byte in our corrupted chunk's size will tell `malloc` that this chunk can be recycled into the newly requested chunk. This
sets the chunk headers back to reasonable values and prevents our `free` exploit from working.

![evil_mallocs]()

How can we fix this? At first glance, it seems like our `size` must have a zero low bit, since the return address is at an even address and our shellcode must start
at an even address (lest we get an `isn address unaligned` error). However, closely examining the `free` function reveals that this is not a problem at all: before
doing any chunk merging, `free` will set the low bit of our size to zero for us to mark the chunk as free. Thus we can simply add one to our corrupted `size` value.
Now `malloc` will ignore it, but `free` will convert it to the correct offset before using it.

The second problem seems troubling, but ends up being easy. If we set the `next chunk` to be any random address, then `rehash` will throw a `Heap exhausted` error.
Why is this?

To answer this, we must understand how this `malloc` works. Namely, it iterates over the currently allocated chunks in order, checking if any of them are free.
If it finds a free chunk that is large enough, it will recycle it into the new chunk (as we saw above). Here we see the problem: when iterating over the list in order,
`malloc` follows the chain of `next chunk` values. If our `next chunk` value is nonsense, `malloc` will freak out and assume the heap memory is all used up.

Luckily, this is extremely easy to fix. Recall that we didn't actually need `next chunk` to point to zeroed-out memory -- we just used that as a hack to make sure
the merged size of the `next chunk` was small. Here, though, we can do something even better: *just set the `next chunk` to its actual value*. This will make sure
`malloc` keeps happily iterating over chunks until it reaches the end, finally allocating more memory without touching our headers.

Why does this work? Well, since the `rehash` will free the chunks in order, the next chunk will still be in use when our corrupted chunk is being freed. This means
`free` will not try to merge the next chunk with this one, and the size will not be affected at all.

The third problem is potentially more insidious: how do we make sure the username, which must have very specific values, falls in the same hash list as our other
values?

## The NOP sled (and why we don't need one)

The username we use to overwrite the heap metadata must have three parts: the `previous chunk` value (which must be exactly `return address - 0x4`), the `next chunk`
value (which must be exactly the address of the next chunk), and the offset from the return address to our shellcode (minus `0x6`). However, if we input this exact data,
we have no guarantee that it will be hashed in a way that puts it in the same list we have already filled up.

We can safely avoid this problem using a technique known as a *NOP sled*. Essentially, instead of starting our shellcode at one precise value, we can start it with a
long string of operations that do literally nothing (a good example is `and r1, r1`). Then all we have to do is point our return address to somewhere in this range,
and the program execution will slide right down the no-ops and reach our shellcode without issue.

Knowing all this, we can now construct our shellcode.

## The shellcode

Recall our battle plan:

- Add several users whose usernames are grouped in the same hash list, overwriting the heap metadata at the start of the next list
- Add even more users to force a `rehash`, wherein the old hash lists (including our corrupted list) will be `free`'d
- Use this corrupted `free` to overwrite a return pointer with the address of some shellcode

Normally, this would take several inputs to achieve. Thankfully, the Microcorruption devs have our back: we can submit several commands at the same time, as long as
we separate them with semicolons (hex `0x3b`).

We will store our shellcode on the stack, because it will lead to a convenient coincidence later. This will break our shellcode up into four parts:
- Several "new [username] [pid]" commands to fill the same hash list right up to the next hash list's metadata,
- A single "new [username] [pid]" command where the username contains our corrupted headers,
- Several more "new [username] [pid]" commands that fall into a different hash list and trigger a rehash,
- Our shellcode, which will be read onto the stack but never interpreted, since our exploit will trigger before then

In order, here is what we used (note the omission of a `0x3b` connecting each segment, which must be added when putting them together):

- Padding the list: `6e6e6e200820313b6e6e6e201020313b6e6e6e201820313b6e6e6e202820313b6e6e6e20302031`
- Corrpting headers: `6e6e6e20ca3dfc50abf42031`
- Trigger rehash: `6e6e6e200320313b6e6e6e200b20313b6e6e6e201320313b6e6e6e201b20313b6e6e6e202320313b6e6e6e202b2031`
- Shellcode: `aa31800600324000ffb0121000`

Note the extra `aa` at the beginning of the shellcode, which will ensure it ends up on a two-byte boundary. The actual shellcode is the same as what we used in Lesson 14.

Combine all these pieces, stick them together with `0x3b`s, and we find an amazing coincidence: the exact offset from the return address to where our shellcode
ends up on the stack is sorted into the same hash list as our padding values. Thanks, Microcorruption devs!

## Solution

`6e6e6e200820313b6e6e6e201020313b6e6e6e201820313b6e6e6e202820313b6e6e6e203020313b6e6e6e20ca3dfc50abf420313b6e6e6e200320313b6e6e6e200b20313b6e6e6e201320313b6e6e6e201b20313b6e6e6e202320313b6e6e6e202b20313baa31800600324000ffb0121000` in hex.
