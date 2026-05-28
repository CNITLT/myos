// crt0 – C runtime startup for user programs
// Called by execv via iretd. Stack layout at entry:
//   [esp+0] = 0      (fake return address, set by exec.c)
//   [esp+4] = argc   (argument count)
//   [esp+8] = argv   (argument vector)

void exit(int);

int main(int argc, char *argv[]);

void _start() {
    int argc;
    char **argv;
    // Compiler generates push ebp; mov ebp, esp due to -fno-omit-frame-pointer.
    // After that, [ebp+8] = argc, [ebp+12] = argv.
    asm volatile(
        "movl 8(%%ebp), %0\n\t"
        "movl 12(%%ebp), %1\n"
        : "=r"(argc), "=r"(argv)
    );
    int ret = main(argc, argv);
    exit(ret);
}
