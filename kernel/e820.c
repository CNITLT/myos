#include "e820.h"
#include "print.h"
#include "string.h"
#include "debug.h"
#define E820_LENGTH_ADDR 0x500
#define E820_TABLE_ADDR 0x504

void print_e820_table(){
    uint32_t *p_length = get_e820_length();
    e820_entry* e820_table = get_e820_table();
    if(*p_length == 0){
        put_str("there has not e820_table\n");
    }
    else{
        for(uint32_t i = 0; i < *p_length; i++){
            put_int(i);
            put_str(":range:");
            put_hex64(e820_table[i].addr);
            put_str("--");
            put_hex64(e820_table[i].addr + e820_table[i].length);    
            put_str(":type:");
            if(e820_table[i].type == e820_type_memory){
                put_str("usable\n");
            }
            else if(e820_table[i].type == e820_type_reserved){
                put_str("reserved\n"); 
            }
            else{
               put_str("other\n");  
            }
        }
    }
}

size_t get_e820_length(){
    return E820_LENGTH_ADDR;
}


const e820_entry* get_e820_table(){
    return (const e820_entry*) E820_TABLE_ADDR;
}

void get_max_memory_e820_entry(e820_entry* p_e820_entry){
    uint32_t *p_length = get_e820_length();
    e820_entry* e820_table = get_e820_table();
    assert(p_length != 0);
    uint64_t max_range = 0;
    uint32_t max_index = 0;
    for(uint32_t i = 0; i < *p_length; i++){
        if(e820_table[i].type == e820_type_memory && e820_table[i].length > max_range){
            max_index = i;
            max_range = e820_table[i].length;
        }
    }
    memcpy(p_e820_entry, &e820_table[max_index], sizeof(e820_entry));
}