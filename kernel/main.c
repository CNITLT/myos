#include "print.h"

int main(){
    char ch = 'A';
    while(1){
       for(short i = -100;i<=100;i++){
            put_int(i);
            put_char('\n');
            int j = 0;
            while(j++ < 1024*1024);
       }
    }
    return 0;
}