#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*

 ________  ___      ___ _____ ______      
|\   ____\|\  \    /  /|\   _ \  _   \    
\ \  \___|\ \  \  /  / | \  \\\__\ \  \   
 \ \  \    \ \  \/  / / \ \  \\|__| \  \  
  \ \  \____\ \    / /   \ \  \    \ \  \ 
   \ \_______\ \__/ /     \ \__\    \ \__\
    \|_______|\|__|/       \|__|     \|__|

CVM is a demo VM (Virtual machine) based on x86-64 CPU architecture.
Github: https://github.com/ac3s0r/cvm

*/

//1 make mov support stack 
// make a method to get from memory of VM

#ifndef CVM_H
#define CVM_H

#ifndef ASM_TYPES
#define ASM_TYPES
#include <stdint.h>
#define byte int8_t
#define word int16_t
#define dword int32_t
#define qword int64_t
#endif

#define ENCODE_DWORD(x)(x >> 0) & 0xff, \
    (char)((x >> 8) & 0xff), \
    (char)((x >> 16) & 0xff), \
    (char)((x >> 24) & 0xff)

#define ENCODE_QWORD(x)(char)((x >> 0) & 0xff), \
    (char)((x >> 8) & 0xff), \
    (char)((x >> 16) & 0xff), \
    (char)((x >> 24) & 0xff), \
    (char)((x >> 32) & 0xff), \
    (char)((x >> 40) & 0xff), \
    (char)((x >> 48) & 0xff), \
    (char)((x >> 56) & 0xff)

#define ALLOCSTRUCT(x)(struct x*) malloc(sizeof(struct x));
#define JOINBITS(x, y)((0b00001111 & x) | (y << 4))
#define GETFIRST(x)(x & 0b00001111)
#define GETSECOND(x)((x >> 4) & 0b00001111)

#define VM_DEBUG 0

enum opcode {
    // Math operations
    VM_ADD, VM_SUB,
    VM_DIV, VM_MUL,
    VM_NEG,
    // Logical operations
    VM_XOR, VM_SHR,
    VM_SHL, VM_AND,
    VM_OR, VM_NOT,
    // General opcodes
    VM_MOV, VM_LEA,
    VM_CMP, VM_RET,
    VM_PUSH, VM_POP,
    // Branching
    VM_CALL, VM_JMP,
    VM_JZ, VM_JNZ,
    VM_JE, VM_JNE,
    VM_JG, VM_JL,
    VM_JLE, VM_JGE,
    // Vm-specific
    VM_CPUID, VM_ABORT
};

// Variants flags for some instructions
enum variants {
    VM_REG2REG,
    VM_REG2MEM,
    VM_MEM2REG,
    VM_VAL2REG,
    VM_MEM2MEM,
    VM_VMEM2REG, // Interact with VM memory
    VM_REG2VMEM
};

// flags register possible values
enum flags {
    VM_FLAG_EQUALS=0b10000000,
    VM_FLAG_GREATER=0b01000000,
    VM_FLAG_LESSER=0b00100000
};

struct stack_entry {
    struct stack_entry * next;
    qword value;
};

struct vm_context {
    qword rax, rbx, rcx, rdx; // General-use registers
    qword r8, r9, r10, r11, r12, r13, r14, r15; // Additional registers
    qword rip, flags, rsi, rbp, rsp; // Special registers

    qword code_base, code_size; // base address of the code
    struct stack_entry * stack;
    qword stack_size;
};

static struct vm_context * vm_makectx() {
    struct vm_context * ctx = ALLOCSTRUCT(vm_context);
    if (ctx) {
        memset(ctx, 0, sizeof(struct vm_context));
    }
    return ctx;
}

static void vm_destroyctx(struct vm_context * ctx) {
    if (ctx) {
        while (ctx->stack) {
            struct stack_entry * temp = ctx->stack;
            ctx->stack = temp->next;
            free(temp);
        }
        free(ctx);
    }
}

static void vm_push(struct vm_context * ctx, qword value) {
    if (ctx) {
        if (!ctx->stack) { // first entry
            ctx->stack = ALLOCSTRUCT(stack_entry);
            memset(ctx->stack, 0, sizeof(struct stack_entry));
        }
        struct stack_entry * entry = ALLOCSTRUCT(stack_entry);
        memset(entry, 0, sizeof(struct stack_entry));
        entry->value = value;
        entry->next = ctx->stack->next;
        ctx->stack->next = entry;
        ctx->stack_size++;
    }
}

static qword vm_pop(struct vm_context * ctx) {
    if (ctx) {
        if (!ctx->stack || !ctx->stack->next)
            return 0;
        struct stack_entry * entry = ctx->stack->next;
        qword value = entry->value;
        ctx->stack->next = entry->next;
        free(entry);
        ctx->stack_size--;
        return value;
    }
}

static void vm_debug(struct vm_context *ctx) {
	printf("Registers:\n"
	           "    rax: %llu\n"
               "    rbx: %llu\n"
               "    rcx: %llu\n"
               "    rdx: %llu\n"
               "    rip: %llu\n"
               "    flags: %d%d%d%d%d%d%d%d\n"
               "Stack size: %llu\n",
               ctx->rax, ctx->rbx,
               ctx->rcx, ctx->rdx,
               ctx->rip, 
               
               (ctx->flags & 0b10000000) !=0,
               (ctx->flags & 0b01000000) != 0,
               (ctx->flags & 0b00100000) != 0,
               (ctx->flags & 0b00010000) != 0,
               (ctx->flags & 0b00001000) != 0,
               (ctx->flags & 0b00000100) != 0,
               (ctx->flags & 0b00000010) != 0,
               (ctx->flags & 0b00000001) != 0,
               ctx->stack_size);
}

static void vm_eval(struct vm_context * ctx, char * instr) {
    if (ctx) {
        #if VM_DEBUG
        printf("Instr:  %lld\n", ctx->rip-ctx->code_base);
        vm_debug(ctx);
        (void)getchar();
        #endif
        qword next_instr_offset = (qword) GETFIRST(instr[0]);
        switch (instr[1]) {
        
        case VM_ADD: {
            switch (GETSECOND(instr[0])) {
                case VM_VAL2REG: {
                    qword * reg = (qword * )((char*)ctx + GETFIRST(instr[2]) * sizeof(qword));
                    qword value = *(qword * )(instr + 3);
                    * reg += value;
                    break;
                }
                case VM_REG2REG: {
                    qword *reg1 = (qword * )((char*)ctx + GETFIRST(instr[2]) * sizeof(qword)),
                          *reg2 = (qword * )((char*)ctx + GETSECOND(instr[2]) * sizeof(qword));
                    * reg1 += *reg2;
                    break;
                }
            }
            break;
        }
        case VM_SUB: {
            switch (GETSECOND(instr[0])) {
                case VM_VAL2REG: {
                    qword * reg = (qword * )((char*)ctx + GETFIRST(instr[2]) * sizeof(qword));
                    qword value = * (qword * )(instr + 3);
                    * reg -= value;
                    break;
                }
                case VM_REG2REG: {
                    qword * reg1 = (qword * )((char*)ctx + GETFIRST(instr[2]) * sizeof(qword)),
                        * reg2 = (qword * )((char*)ctx + GETSECOND(instr[2]) * sizeof(qword));
                    * reg1 -= * reg2;
                    break;
                }
            }
            break;
        }
        case VM_MUL: {
        	switch (GETSECOND(instr[0])) {
                case VM_VAL2REG: {
                    qword *reg = (qword*)((char*)ctx + GETFIRST(instr[2]) * sizeof(qword));
                    qword value = *(qword*)(instr + 3);
                    *reg *= value;
                    break;
                }
                case VM_REG2REG: {
                    qword *reg1 = (qword*)((char*)ctx + GETFIRST(instr[2]) * sizeof(qword)),
                          *reg2 = (qword*)((char*)ctx + GETSECOND(instr[2]) * sizeof(qword));
                    *reg1 *= *reg2;
                    break;
                }
            }
            break;
        }
        case VM_DIV: {
        	switch (GETSECOND(instr[0])) {
                case VM_VAL2REG: {
                    qword *reg = (qword*)((char*)ctx + GETFIRST(instr[2]) * sizeof(qword)),
                          *rdx = (qword*)((char*)ctx + 3 * sizeof(qword));
                    qword value = *(qword*)(instr + 3);
                    *rdx = *reg % value;
                    *reg /= value;
                    break;
                }
                case VM_REG2REG: {
                    qword *reg1 = (qword*)((char*)ctx + GETFIRST(instr[2]) * sizeof(qword)),
                          *reg2 = (qword*)((char*)ctx + GETSECOND(instr[2]) * sizeof(qword)),
                          *rdx = (qword*)(ctx + 3 * sizeof(qword));
                    *rdx = *reg1 % *reg2;
                    *reg1 /= *reg2;
                    break;
                }
            }
            break;
        }
        
        case VM_CALL: {
        	void* target_addr = (void*)*(qword*)(instr+3);
        	typedef qword(*call_t)(void*, ...);
        	call_t call = (call_t)target_addr;
            
        	word args_count = GETFIRST(instr[2]);
            void* ptrs[16];
            memset(&ptrs, 0, sizeof(ptrs)/sizeof(ptrs[0]));
            for(int i = 0; i < args_count; i++) {
                ptrs[i] = (void*) vm_pop(ctx);
            }
          
            ctx->rax = call(ptrs[0], ptrs[1], ptrs[2], ptrs[3], ptrs[4], ptrs[5], ptrs[6], ptrs[7], ptrs[8], 
                            ptrs[9], ptrs[10], ptrs[11], ptrs[12], ptrs[13], ptrs[14], ptrs[15]);
            break;
        }
        
        case VM_RET: {
        	qword offset = vm_pop(ctx);
        	if (offset)
        		next_instr_offset = (dword) offset;
        	else
        		next_instr_offset = ctx->code_size; // kill ourselves
        	break;
        }
        
        case VM_MOV: {
            switch (GETSECOND(instr[0])) {
                case VM_VAL2REG: {
                    qword value = *(qword*)(instr + 3);
                    qword *reg = (qword *)((char*)ctx + GETFIRST(instr[2]) * sizeof(qword));
                    *reg = value;
                    break;
                }
                case VM_REG2REG: {
                    qword *reg1 = (qword *)((char*)ctx + GETFIRST(instr[2]) * sizeof(qword)),
                          *reg2 = (qword *)((char*)ctx + GETSECOND(instr[2]) * sizeof(qword));
                    *reg1 = *reg2;
                    break;
                }
                case VM_MEM2REG: {
                    qword *reg = (qword *)((char*)ctx + GETFIRST(instr[2]) * sizeof(qword)),
                          *addr = (qword*)*(qword*)(instr + 3);
                    *reg = *addr;
                    break;
                }
                case VM_REG2MEM: {
                    qword *reg = (qword *)((char*)ctx + GETFIRST(instr[2]) * sizeof(qword)),
                          *addr = (qword*)*(qword*)(instr + 3);
                    *addr = *reg;
                    break;
                }
                case VM_VMEM2REG: {
                	 qword *reg = (qword *)((char*)ctx + GETFIRST(instr[2]) * sizeof(qword)),
                                 offset = *(qword*)(instr + 3);
                	 *reg = *(qword*)((char*)ctx->code_base + offset);
                	 break; 
                }
                case VM_REG2VMEM: {
                	 qword *reg = (qword *)((char*)ctx + GETFIRST(instr[2]) * sizeof(qword)),
                                 offset = *(qword*)(instr + 3);
                	 *(qword*)((char*)ctx->code_base + offset) = *reg;
                	 break; 
                }
            }
            break;
        }
        case VM_PUSH: {
        	switch (GETSECOND(instr[0])) {
                case VM_VAL2REG: {
                    qword value = *(qword*)(instr + 2);
                    vm_push(ctx, value);
                    break;
                }
                case VM_REG2REG: {
                    qword *reg = (qword*)((char*)ctx + GETFIRST(instr[2]) * sizeof(qword));
                    vm_push(ctx, *reg);
                    break;
                }
            }
            break;
        }
        case VM_POP: {
        	*(dword*)((char*)ctx + GETFIRST(instr[2]) * sizeof(qword)) = vm_pop(ctx);
            break;
        }
        case VM_LEA: {
        	switch (GETSECOND(instr[0])) {
                case VM_VAL2REG: {
                    qword *reg = (qword*)((char*)ctx + GETFIRST(instr[2]) * sizeof(qword));
                    *reg = ctx->rip + (dword)*(qword*)(instr + 3);
                    break;
                }
                case VM_REG2REG: {
                    qword *reg1 = (qword*)((char*)ctx + GETFIRST(instr[2]) * sizeof(qword)),
                          *reg2 = (qword*)((char*)ctx + GETSECOND(instr[2]) * sizeof(qword));
                    *reg1 = ctx->rip + (dword) *reg2; 
                    break;
                }
            }
            break;
        }
        
        case VM_XOR: {
            switch (GETSECOND(instr[0])) {
                case VM_VAL2REG: {
                    qword *reg = (qword*)((char*)ctx + GETFIRST(instr[2]) * sizeof(qword));
                    qword value = *(qword*)(instr + 3);
                    *reg ^= value;
                    break;
                }
                case VM_REG2REG: {
                    qword *reg1 = (qword*)((char*)ctx + GETFIRST(instr[2]) * sizeof(qword)),
                          *reg2 = (qword*)((char*)ctx + GETSECOND(instr[2]) * sizeof(qword));
                    *reg1 ^= *reg2;
                    break;
                }
            }
            break;
        }
        case VM_SHL: {
            switch (GETSECOND(instr[0])) {
                case VM_VAL2REG: {
                    qword *reg = (qword*)((char*)ctx + GETFIRST(instr[2]) * sizeof(qword));
                    qword value = *(qword*)(instr + 3);
                    *reg <<= value;
                    break;
                }
                case VM_REG2REG: {
                    qword *reg1 = (qword*)((char*)ctx + GETFIRST(instr[2]) * sizeof(qword)),
                          *reg2 = (qword*)((char*)ctx + GETSECOND(instr[2]) * sizeof(qword));
                    *reg1 <<= *reg2;
                    break;
                }
            }
            break;
        }
        case VM_SHR: {
            switch (GETSECOND(instr[0])) {
                case VM_VAL2REG: {
                    qword *reg = (qword*)((char*)ctx + GETFIRST(instr[2]) * sizeof(qword));
                    qword value = *(qword*)(instr + 3);
                    *reg >>= value;
                    break;
                }
                case VM_REG2REG: {
                    qword *reg1 = (qword*)((char*)ctx + GETFIRST(instr[2]) * sizeof(qword)),
                          *reg2 = (qword*)((char*)ctx + GETSECOND(instr[2]) * sizeof(qword));
                    *reg1 >>= *reg2;
                    break;
                }
            }
            break;
        }
        case VM_CMP: {
            switch (GETSECOND(instr[0])) {
                case VM_VAL2REG: {
                    qword *reg = (qword*)((char*)ctx + GETFIRST(instr[2]) * sizeof(qword));
                    qword value = *(qword*)(instr + 3);
                    ctx->flags = (qword) 0; // reset flags
                    if (*reg == value)
                        ctx->flags |= VM_FLAG_EQUALS;
                    if (*reg > value)
                        ctx->flags |= VM_FLAG_GREATER;
                    if (*reg < value)
                        ctx->flags |= VM_FLAG_LESSER;
                    break;
                }
                case VM_REG2REG: {
                    qword *reg1 = (qword*)((char*)ctx + GETFIRST(instr[2]) * sizeof(qword)),
                          *reg2 = (qword*)((char*)ctx + GETSECOND(instr[2]) * sizeof(qword));
                    ctx->flags = (qword) 0; // reset flags
                    if (*reg1 == *reg2)
                        ctx->flags |= VM_FLAG_EQUALS;
                    if (*reg1 > *reg2)
                        ctx->flags |= VM_FLAG_GREATER;
                    if (*reg1 < *reg2) 
                        ctx->flags |= VM_FLAG_LESSER;
                    break;
                }
            }
            break;
        }
        case VM_JMP: {
            switch (GETSECOND(instr[0])) {
                case VM_VAL2REG: {
                    qword value = *(qword*)(instr + 2);
                    next_instr_offset = (dword) value;
                    break;
                }
                case VM_REG2REG: {
                    qword *reg1 = (qword*)((char*)ctx + GETFIRST(instr[2]) * sizeof(qword));
                    next_instr_offset = *reg1;
                    break;
                }
            }
            break;
        }
        case VM_JZ: {
            if (ctx->flags == 0) {
                qword value = *(qword*)(instr + 2);
                next_instr_offset = (dword)value;
            }
            break;
        }
        case VM_JNZ: {
            if (ctx->flags != 0) {
                qword value = *(qword*)(instr + 2);
                next_instr_offset = (dword)value;
            }
            break;
        }
        case VM_JLE: {
            if ((ctx->flags & VM_FLAG_EQUALS) 
                 || (ctx->flags & VM_FLAG_LESSER) ) {
                qword value = *(qword*)(instr + 2);
                next_instr_offset = (dword)value;
            }
            break;
        }
        case VM_JGE: {
            if ((ctx->flags & VM_FLAG_EQUALS) 
                 || (ctx->flags & VM_FLAG_GREATER)) {
                qword value = *(qword*)(instr + 2);
                next_instr_offset = (dword)value;
                
            }
            break;
        }
        case VM_JNE: {
        	if (!(ctx->flags & VM_FLAG_EQUALS)) {
        		qword value = *(qword*)(instr + 2);
                next_instr_offset = (dword)value;
        	}
        }
        case VM_JE: {
        	if (ctx->flags & VM_FLAG_EQUALS) {
        		qword value = *(qword*)(instr + 2);
                next_instr_offset = (dword)value;
        	}
        }
       
        // TODO: implement more instructions & implement existing
        
        default: // All invalid opcodes are just ignored
            break;
        }
        ctx->rip += next_instr_offset;
    }
}

static void vm_exec(struct vm_context *ctx, char *code, int size) {
    ctx->code_base = (qword)code; // bad idea but whatever
    ctx->code_size = (qword)size;   // bad idea but whatever
    ctx->rip = (qword)code;
    while (ctx->rip < (qword)(code+size))
        vm_eval(ctx, (char*) ctx->rip);
}

#endif