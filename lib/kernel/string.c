#include "string.h"
#include "stddef.h"

void memset(void *dst, uint8_t value, size_t count){
    uint8_t *p = (uint8_t *)dst;
    for(size_t i = 0; i < count; i++){
        *p = value;
        p++;
    }
}



void memcpy(void *dst,const void* src, size_t count){
    uint8_t *p_dst = (uint8_t *)dst;
    const uint8_t *p_src = (const uint8_t *)src;

    if(dst == src || count == 0){
        return;
    }
    if(dst > src){
        for(size_t i = count - 1; i > 0; i--){
            p_dst[i] = p_src[i];
        }
        p_dst[0] = p_src[0];
    }
    else{
        for(size_t i = 0; i < count; i++){
           p_dst[i] = p_src[i];
        }
    }
}


int memcmp(const void* a,const void *b, size_t count){
    const uint8_t *p_a = (const uint8_t*) a;
    const uint8_t *p_b = (const uint8_t*) b;
    while(count--){
        if(*p_a != *p_b){
            return *p_a < *p_b?-1:1;       
        }
    }
    return 0;
}


size_t strlen(const char *str){
    size_t count = 0;
    while(*str){
        count++;
        str++;
    }
    return count;
}


char* strcpy(char* dst,const char * src){
    char *ret = dst;
    while(*src){
        *dst = *src;
        dst++;
        src++;
    }
    *dst = 0;
    return ret;
}

int strcmp(const char *a,const char *b){
    return memcmp(a, b, strlen(a) > strlen(b)?strlen(b)+1:strlen(a)+1);
}

char * strchr(const char* str, char ch){
    while(*str){
        if(*str == ch){
            return (char*)str;
        }
        str++;
    }
    return NULL;
}

char* strrchr(const char* str, char ch){
   char *ret = NULL;
   while(*str){
        if(*str == ch){
            ret = (char*)str;
        }
        str++;
   }
   return ret;
}


char *strcat(char *dst, const char*src){
    char *ret = dst;
    while(*dst){
        dst++;
    }
    while(*src){
        *dst = *src;
        dst++;
        src++;
    }
    *dst = 0;
    return ret;
}


size_t strchrs(const char* str, char ch){
    size_t count = 0;
    while(*str){
        if(*str == ch){
            count++;
        }
        str++;
    }
    return count;
}