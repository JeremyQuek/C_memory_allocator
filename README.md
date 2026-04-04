# Memory Allocator
As another part of system programming, I decided to implement a custom memory allocator ( `malloc`, `free`, and `realloc` )in C, built on top of `sbrk` to manually manage heap memory.
This taught me fundamental principles about heap memory layout, how blocks are handled and free lists are allocated. I also learned that malloc is internally managed with a lock to prevent threading race conditions,
which would be optimized with more fine grain synchronizations.

<br/>
<div align = "center" >
  <h3> Heap Block Layout </h3>
  <img width="646" height="548" alt="Screenshot 2026-04-03 at 11 19 26 PM" src="https://github.com/user-attachments/assets/d98597c2-85a2-445c-bbea-236385e90406" />
</div>
<br/>

## How it works
The heap is tracked as two linked lists — one for allocated blocks, one for free blocks. Every block has a small metadata header storing its size, whether it's in use, and pointers to its neighbors. 
However, programmers dnt get to see this, they just get a pointer to the data region.

```
[ size | used | next | prev | data ... ]
```

Think of it like a row of labeled boxes. The allocator knows exactly where each box starts, how big it is, and whether someone's using it.
<br/>

## Allocation
1. Walk the free list for a block that fits (first-fit)
2. If found and it's much bigger than needed, split it — use the front, return the rest to the free list
3. If nothing fits, call `sbrk` to push the heap boundary forward and carve out fresh memory
4. Zero-fill and return a pointer to the data region (past the header)

The split is important — without it, a 10 byte request could eat a 10,000 byte block and waste everything.
<br/>

## Deallocation
1. Move the block from the allocated list back into the free list (kept sorted by address)
2. Coalesce — scan neighbors and merge any adjacent free blocks into one

Without coalescing, repeated alloc/free cycles would slowly grind the heap into tiny unusable fragments. Keeping the free list sorted by address makes it cheap to check if neighbors are free.
<br/>

## Reallocation
1. `malloc` a new block of the requested size
2. `memcpy` the old data over — only as much as fits in the new block
3. `free` the original

If the new allocation fails, the original block is left completely untouched. Edge cases handled: `realloc(NULL, n)` → `malloc(n)`, `realloc(ptr, 0)` → `free(ptr)`.
