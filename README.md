# CVM

CVM is a header-only turing-complete virtual machine engine made in pure C and based on x86-64. It's not fully replicating the architecture, but can be used to implement any existing algorithm. It's created as an example of a VM for code protection purposes.

⚠️ The VM is still in developement and might be unstable. Be sure to report any problems you encounter in issues, that will be very helpful.

<div align=center style="background-color: transparent;">
    <img width=80% src="images/vm_preview.png"></img>
</div>

## Examples
Here's an example of a pow method implementation before and after virtualization.
You can find more examples in main.c.

```C
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
```

<div align=center style="background-color: transparent;">
    <img width=100% src="images/execution.png"></img><br/>
    <text>Execution flow (The results are the same)</text>
</div>

## Usage
To use the VM you will have to write the bytecode manually which can be hard and challenging, but remember it will be double as painful to reverse-engineer. Include the header into your projects by simply including the header.

## TODO
- [ ] Add macros to construct instructions easier (aka JMP(), ADD(), etc)
- [ ] Make a compiler with label support to write the bytecode easier
- [ ] Implement more instructions