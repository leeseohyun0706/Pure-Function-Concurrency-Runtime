# Basic Assembly Structure in ARM64

> This document explains assembly structure only in ARM64, particularly for macOS m1 and ARMv8+ (Because ARMv7 doesn't provide atomic operations as a single instruction, we need to manually insert memory barrier with using a `dme` and `dsb` instruction, and it's really hard to understand).

ARM64 provides 31 amounts of general-purpose registers. When we access such a register, there are two-way to access it in terms of 64bits and 32bits. For 64bits access, we can use register notation `x[0-30]`, while we can use `w[0-30]` for 32bits register access.

### Memory Address Access (Like Pointer in C)

In assembly code, pointer memory access can be used by using the notation `[register]`.
For example, supposes that we intend to read 32bits from the memory address indicated by a register `x1`. We can express it with the `ldr` (Load Register) instruction as

```assembly
ldr w0, [x1].
```

> Note that `w0` denotes 32bits memory access while `x0` denotes 64bits memory access.

Similarly, supposes that we intend to store 32bits to the memory address indicated by a register `x1`, then it can be expressed with the `str` (Store Register) instruction as

```assembley
str w0, [x1].
```

## Atomic Instruction for Memory Access

Because ARM64v8+ instructions provide LSE(Large System Extensions), we can execute a set of small atomic operations as a single instruction.
Like we used instructions `ldr` and `str`, for ensuring memory orders during atomic operations, we can use the instructions `ldar` and `stlr` (Load-Acquire Register and Store-Release Register)

```assembly
ldar w0, [x1].
```
```assembley
stlr w0, [x1].
```

`std::memory_order_acquire` and `std::memory_order_release` are mapping to these instructions `ldar` and `stlr` respectively.
