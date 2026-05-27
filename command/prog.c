#include "prog.h"
#include "print.h"

int prog(int argc, char *argv[]) {
    printf("hello world argc:%d argv:0x%x\n",argc, argv);
    printf("&argc:0x%x\n &argv:0x%x\n", &argc, &argv);
    for(int i = 0; i < argc; i++) {
        printf("argv[%d] = %s\n", i, argv[i]);
    }

    while(1);
    return 0;
}