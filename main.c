#include "include/cvm.h"

static int pow_normal(int n, int b) {
    int r = n;
    for (int i = 1; i < b; i++)
        r*=n;
    return r;
}

static int pow_virt(int n, int b) {
    struct vm_context * ctx = vm_makectx();
    char code[] = {
        // Load params into registers
        // mov rax, n
        JOINBITS(11, VM_VAL2REG),
        VM_MOV,
        JOINBITS(0, 0),
        ENCODE_QWORD((qword)n), 
        // mov rbx, b
        JOINBITS(11, VM_VAL2REG),
        VM_MOV,
        JOINBITS(1, 0),
        ENCODE_QWORD((qword)b), 
        // int i = 0;
        // mov rdx, 1 
        JOINBITS(11, VM_VAL2REG),
        VM_MOV, JOINBITS(3, 0),
        ENCODE_QWORD(1), 
        
        // result
        // mov rcx, rax
        JOINBITS(3, VM_REG2REG), 
        VM_MOV,
        JOINBITS(2, 0),
        
        // loop start
        // cmp rdx, rbx
        JOINBITS(3, VM_REG2REG),
        VM_CMP,
        JOINBITS(3, 1),
        
        //  jge end_loop if (b >= i) jmp end_loop
        // out of scope 
        JOINBITS(10, VM_REG2REG),
        VM_JGE,
        ENCODE_QWORD(34), // jump to end_loop
        
        // mul rcx, rax
        JOINBITS(3, VM_REG2REG),
        VM_MUL,
        JOINBITS(2, 0),
        
         // add rdx, 1
        JOINBITS(11, VM_VAL2REG),
        VM_ADD, JOINBITS(3, 0),
        ENCODE_QWORD(1), 
        
        // jmp loop_start
        JOINBITS(10, VM_VAL2REG),
        VM_JMP,
        ENCODE_QWORD(-27),

        // end_loop
        // mov rax, rcx to return in rax
        JOINBITS(3, VM_REG2REG),
        VM_MOV,
        JOINBITS(0, 2)
    };
    vm_exec(ctx, code, sizeof(code));
    qword retval = ctx->rax;
    vm_destroyctx(ctx);
    return retval;
}

static void helloworld_vm() {
	 // Very basic hello world using CVM.h
    char code[] = {
        // Jump over .data since it shouldnt be executed
        JOINBITS(10, VM_VAL2REG),
        VM_JMP, ENCODE_QWORD(16),
         //.data
        'W', 'o', 'm', 'p', '\n', 0,
        // lea rax, [womp]
        JOINBITS(11, VM_VAL2REG),
        VM_LEA, JOINBITS(0, 0),
        ENCODE_QWORD(-6), // rip-relative offset of WOMP
        // push rax, 
        JOINBITS(3, VM_REG2REG),
        VM_PUSH, JOINBITS(0, 0),
        // call printf
        JOINBITS(11, VM_VAL2REG),
        VM_CALL,
        JOINBITS(1, 0),
        ENCODE_QWORD((qword)&printf),
        // ret
        JOINBITS(2, VM_REG2MEM),
        VM_RET,
    };
    struct vm_context * ctx = vm_makectx();
    vm_exec(ctx, code, sizeof(code));
    vm_destroyctx(ctx);
}

int main(int argc, char * argv[]) {
   // Example 1
   //helloworld_vm();
   
   // Example 2
   int i = 3;
   int a = pow_normal(i, i), b = pow_virt(i, i);
   printf("a: %d, b: %d\n", a, b);
   printf("Result: %d\n", a==b);
   
   (void)getchar();
   return 0;
}