/*
 * Copyright 2015 Big Switch Networks, Inc
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef UBPF_H
#define UBPF_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Default values for maximum instruction count and stack size.
#if !defined(UBPF_MAX_INSTS)
#define UBPF_MAX_INSTS 65536
#endif

#if !defined(UBPF_STACK_SIZE)
#define UBPF_STACK_SIZE 512
#endif

struct ubpf_vm;
typedef uint64_t (*ubpf_jit_fn)(void *mem, size_t mem_len);

typedef void *(*ubpf_zmalloc_fn)(void *cookie, size_t size);
typedef void *(*ubpf_free_fn)(void *ptr);

/*
 * Create ubmf vm
 *
 * Zmalloc_cookie and pointers to zmalloc and free functions are stored
 * in the ubpf_vm struct.
 * Zmalloc function is invoked for allocation with zmalloc_cookie passed
 * as parameter; free function is invoked for deallocation.
 */
struct ubpf_vm *
ubpf_create(void *zmalloc_cookie,
            ubpf_zmalloc_fn zmalloc_fn,
            ubpf_free_fn free_fn);
void ubpf_destroy(struct ubpf_vm *vm);

/*
 * Enable / disable bounds_check
 *
 * Bounds check is enabled by default, but it may be too restrictive
 * Pass true to enable, false to disable
 * Returns previous state
 */
bool ubpf_toggle_bounds_check(struct ubpf_vm *vm, bool enable);


/*
 * Set the function to be invoked if the jitted program hits divide by zero.
 *
 * fprintf is the default function to be invoked on division by zero.
 */
void ubpf_set_error_print(struct ubpf_vm *vm, int (*error_printf)(FILE* stream, const char* format, ...));

/*
 * Register an external function
 *
 * The immediate field of a CALL instruction is an index into an array of
 * functions registered by the user. This API associates a function with
 * an index.
 *
 * 'name' should be a string with a lifetime longer than the VM.
 *
 * Returns 0 on success, -1 on error.
 */
int ubpf_register(struct ubpf_vm *vm, unsigned int idx, const char *name, void *fn);

/*
 * Load code into a VM
 *
 * This must be done before calling ubpf_exec or ubpf_compile and after
 * registering all functions.
 *
 * 'code' should point to eBPF bytecodes and 'code_len' should be the size in
 * bytes of that buffer.
 *
 * Returns 0 on success, -1 on error. In case of error a pointer to the error
 * message will be stored in 'errmsg' and should be freed by the caller.
 */
int ubpf_load(struct ubpf_vm *vm, const void *code, uint32_t code_len, char **errmsg);

/*
 * Load code from an ELF file
 *
 * This must be done before calling ubpf_exec or ubpf_compile and after
 * registering all functions.
 *
 * 'elf' should point to a copy of an ELF file in memory and 'elf_len' should
 * be the size in bytes of that buffer.
 *
 * The ELF file must be 64-bit little-endian with a single text section
 * containing the eBPF bytecodes. This is compatible with the output of
 * Clang.
 *
 * Returns 0 on success, -1 on error. In case of error a pointer to the error
 * message will be stored in 'errmsg' and should be freed by the caller.
 */
int ubpf_load_elf(struct ubpf_vm *vm, const void *elf, size_t elf_len, char **errmsg);

int ubpf_exec(const struct ubpf_vm *vm, void *mem, size_t mem_len, uint64_t* bpf_return_value);

ubpf_jit_fn ubpf_compile(struct ubpf_vm *vm, char **errmsg);

/*
 * Translate the eBPF byte code to x64 machine code, store in buffer, and 
 * write the resulting count of bytes to size.
 *
 * This must be called after registering all functions.
 *
 * Returns 0 on success, -1 on error. In case of error a pointer to the error
 * message will be stored in 'errmsg' and should be freed by the caller.
 */
int ubpf_translate(struct ubpf_vm *vm, uint8_t *buffer, size_t *size, char **errmsg);

/*
 * Instruct the uBPF runtime to apply unwind-on-success semantics to a helper
 * function. If the function returns 0, the uBPF runtime will end execution of
 * the eBPF program and immediately return control to the caller. This is used
 * for implementing function like the "bpf_tail_call" helper. 
 * Returns 0 on success, -1 on if there is already an unwind helper set.
 */
int ubpf_set_unwind_function_index(struct ubpf_vm *vm, unsigned int idx);

#endif
