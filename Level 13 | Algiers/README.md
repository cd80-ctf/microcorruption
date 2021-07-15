# Microcorruption CTF - Algiers (100 points)

## Problem

Same deal as always -- find an input that opens the door. This time, the program has substantially changed to use the heap. As you can tell from the point total, this
will be a difficulty spike for new users.

![manual](https://user-images.githubusercontent.com/86139991/125711569-5b000408-5588-4edd-8076-7cd0eab8c4e1.PNG)

## Reasoning

## The new system

As the manual tells us, the lock program is significantly different. We can now put in a username and a password, which are stored in sequential chunks
on the heap. Both chunks have size `0x10`, as can be seen in the corresponding calls to `malloc`:

![malloc](https://user-images.githubusercontent.com/86139991/125711884-b336abf9-4c2e-42e9-864a-21c24a98eac7.PNG)

By putting a breakpoint after these calls, we can see the structure of the heap:

![heap_after_two_chunks](https://user-images.githubusercontent.com/86139991/125711953-a3bf8e45-6f76-4abc-8af0-0bff6869e865.PNG)

The first eight bytes here seem to mark the start of the heap, as well as the address of the first chunk (`0x2408`.) After this, we get our two allocated chunks, which
seem to have the following format:

- First word: a pointer to the previous chunk (note the first chunk points to itself.)
- Second word: a pointer to the next chunk.
- Third word: twice the size of the chunk in bytes. The reason this is twice the size is unclear. Furthermore, the last bit of this chunk is used to signify that the
  chunk is in use. This can be seen because the `free` function sets this bit to zero.
- Rest of the chunk: allocated for our data.

## Heap corruption

As in previous exercises, the crucial bug in this program is an overflow: only `0x10` bytes are allocated on the heap for the username and password, but up to `0x30`
bytes are copied:

![overflow](https://user-images.githubusercontent.com/86139991/125712633-bbe3ca0e-7f63-4bb9-9f41-038d6a445531.PNG)

Since the heap is so far removed from the stack, we cannot use this to overwrite the return pointer. However, we *can* overflow the username chunk to overwrite the
metadata at the start of the password chunk. When the chunk is later `free`'d, the program will use this metadata in a way that will let us overwrite arbitrary
words in memory.

### The `free` function

The `free` function for this heap allocator is fairly simple. In pseudocode, it looks like this:

```
void free(address of chunk to free) {
    subtract 6 from the address of the chunk to free to get the address of that chunk's metadata;
    set the last bit of the current chunk's size to zero to denote that it is free;
    
    check the size of the previous chunk
    if the last bit of the previous chunk's size is zero {
        // merge the previous chunk and current chunk
        add the size of the current chunk (plus six, to account for the current chunk's metadata) to the size of the previous chunk;
        set the 'next chunk' value of the previous chunk to the 'next chunk' value of the current chunk;
        set the 'previous chunk' value of the next chunk to the previous chunk;
    
        set the current chunk to the previous chunk; // original chunk no longer exists
    }
    
    check the size of the next chunk
    if the last bit of the next chunk's size is zero {
        // merge the current chunk and next chunk
        add the size of the next chunk (plus six, to account for the current chunk's metadata) to the size of the current chunk;
        set the 'next chunk' value of the current chunk to the 'next chunk' value of the next chunk;
        set the 'previous chunk' value of the chunk after the next chunk to the current chunk; // next chunk no longer exists
    } 
}
```

### Abusing `free` with evil metadata

There are three values we can overwrite: the 'previous chunk' value of the password chunk, the 'next chunk' value of the password chunk, and the size of the password
chunk.

One interesting idea is to overwrite the 'previous chunk' value with the location of the return address minus `0x2`, and the 'next_chunk' value with the address
of `unlock_door`. That way, when `free` sets the 'next chunk' value of the previous chunk to the 'next chunk' value of the current chunk, the 'next chunk' value of
the previous chunk will be located at ((return address minus `0x2`) + `0x2`) = return address, and the return address will be overwritten with the address of 
`unlock_door`.

Unfortunately, this fails because of the next part of `free`, which sets the 'previous chunk' value of the next chunk to the previous chunk. This will set the word
at the start of `unlock_door` to the return address, overwriting the first instruction in `unlock_door` with nonsense. This will cause the program to crash when 
`unlock_door` is called.

In fact, it can be seen that if the 'previous chunk' and 'next chunk' values of the password chunk are overwritten with `addr1` and `addr2`, then `free` will do
the following:

- `addr1 + 0x2` will be replaced with `addr2`
- `addr2` will be replaced with `addr1`

- current chunk will become `addr1`

- `addr1 + 0x2` will be replaced with `addr2 + 0x2`
- `addr2` will be replaced with `addr 1`

As a result, the memory at both `addr1` and `addr2` will be overwritten. Therefore we cannot set `addr1` or `addr2` to the address of any executable code, because
it will be overwritten with nonsense and render the code unrunnable.

### Using the size value

The only remaining value over which we have control is the size of the chunk. This is the key to exploiting the program and unlocking the door.

Our target will be the return address of `free` itself -- in this case, `4390`. We will overwrite the previous chunk with the location of this return address minus 
`0x04`, so that `free` will think the return address is the size of the previous chunk.

Since the system is aligned on a two-byte boundary, the lowest bit of the return address will always be zero. This means the program will always think the 'previous
chunk' is free, and will combine it with the current chunk. When it does this, it will add `0x06` plus the size of the password chunk to the return address. To simplify
matters, the 'next chunk' value of the password chunk will be overwritten with an address in the middle of zeroed-out memory. This will cause `free` to think the next
chunk is also out of use (and has size zero), which will add another `0x06` to the return address. All in all, `free` will set the return address to `4396` plus the
overwritten size of the password chunk.

(note that the two words above the return address will also be overwritten, since `free` thinks they are the 'previous' and 'next' chunks of the previous chunk.
thankfully, these values aren't important.)

All we need to do, then, is overwrite the size of the password chunk with the address of `unlock_door` minus `0x4396`. Once we do this, `free` will contrive to
set its own return address to the address of `unlock_door` for us, and the door will open.

## Solution

Username: `aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa90431010b0fe` in hex. The first sixteen bytes are filler, the next two are the location of the return address minus `0x4`, the two after
that are our dummy address (which can be any aligned address in zeroed-out memory), and the final two are the offset of the legitimate return address to `unlock_door`
(minus `0x0b`, since `0x06` is added twice before `free` returns.

Password: nothing (password is irrelevant)
