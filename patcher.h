#include <stdlib.h>
#include <stdio.h>
#include <string.h>


//Contains instructions for swapping a sequence of bytes with another in a file
typedef struct Mod {
   char* seq;  //sequence of bytes to search for
   long seq_len;   //length of search sequence
   long offset;       //offset between start of search sequence and where new sequence should be written. 
   char* swap;         //new sequence
   long swap_len;      //length of new sequence
   long default_pos;  //default position to begin searching in case already known.
} Mod;


Mod* infinite_lives();
Mod* zero_score_display();
Mod* zero_score_final();
Mod* keep_power1();
Mod* keep_power2();

Mod* load_mod(char seq[], long len, char swap[], long swap_len, long offset, long pos);
void free_mod(Mod* mod);

long scan(char* seq, long seq_len, char* buffer, long buf_size, long start, long end);
int apply_mod(Mod* mod, char* buffer, long size, FILE* f_out);



