#include "patcher.h"

/*
Patches the Touhou 7 game executable for:
-Infinite lives
-No power loss upon death(assuming >16 power)
-All scores generated set to 0.

This is meant purely as a practice tool to learn and practice difficult 
patterns.  Therefore, no points/scores can be generated while having infinite lives.

For version 1.00b
Should work with english patched files.

Usage:
    patcher inputfile outputfile
Example:
    patcher th07.exe th07_infinite_lives.exe
*/

//Not a huge fan of how I set this up but it works.
//TODO: load mods from external file


const long BASE_OFFSET = 0x400C00;


//Don't lose lives on death(infinite lives)
/*  
  bytes: Byte sequence to search for in executable 
  swap:  Byte sequence to write at position of bytes + offset
  
  It would be simpler to just have the original and new byte sequences 
  the same size.  However, if the sequence occurs multiple times in the
  file it could cause issues.  By searching for a longer series of bytes,
  then using an offset from that location, it is less likely to 
  have false positives when searching the file.
*/
Mod* infinite_lives() {

    char bytes[] = {      
        0x8B, 0x45, 0xFC, //MOV EAX,DWORD PTR SS:[EBP-4]
        0x8B, 0x40, 0x08, //MOV EAX,DWORD PTR DS:[EAX+8]
        0xDB, 0x45, 0x08, //FILD DWORD PTR SS:[EBP+8]
        0xD8, 0x40, 0x5C, //FADD DWORD PTR DS:[EAX+5C]
    };
    char swap[] = {       //Sequence to write at location of bytes+offset
        0xD9, 0xEE,       //FLDZ
        0x90              //NOP
    };
    int offset = 6;
    int default_pos = 0x2c9f0;
    
    //sizes known at compile time so should work
    return load_mod(bytes, sizeof(bytes), swap, sizeof(swap), offset, default_pos);
}


//Zero the score display
Mod* zero_score_display() {
    char bytes[] = {
        0x03, 0x41, 0x08, //ADD EAX,DWORD PTR DS:[ECX+8]
        0x8B, 0x4D, 0xEC, //MOV ECX,DWORD PTR SS:[EBP-14]
        0x8B, 0x49, 0x08, //MOV ECX,DWORD PTR DS:[ECX+8]
        0x89, 0x01,       //MOV DWORD PTR DS:[ECX],EAX
        0x8B, 0x45, 0xEC, //MOV EAX,DWORD PTR SS:[EBP-14]
        0x8B, 0x40, 0x08, //MOV EAX,DWORD PTR DS:[EAX+8]
    };
    char swap[] = {
        0x83, 0xC0, 0x00  //ADD EAX,0
    };
    int offset = 0;
    int default_pos = 0;
    
    //sizes known at compile time so should work
    return load_mod(bytes, sizeof(bytes), swap, sizeof(swap), offset, default_pos);
}


//Zero points when calculating final scores
Mod* zero_score_final() {
    char bytes[] = {
        0x81, 0x78, 0x04, 0x00, 0xCA, 0x9A, 0x3B, //CMP DWORD PTR DS:[EAX+4],3B9ACA00
        0x72, 0x0C,                      //JB SHORT th07e_ol.0042F498
        0xA1, 0x78, 0x62, 0x62, 0x00,    //MOV EAX,DWORD PTR DS:[626278]
        0xC7, 0x40, 0x04, 0xFF, 0xC9, 0x9A, 0x3B //MOV DWORD PTR DS:[EAX+4],3B9AC9FF
    };
    char swap[] = {
        0x81, 0x78, 0x04, 0x00, 0x00, 0x00, 0x00, //CMP DWORD PTR DS:[EAX+4],00000000     //83 for 16 bit w/ NOPs
        //0x90, 0x90, 0x90,                //NOP NOP NOP
        0x72, 0x0C,                      //JB SHORT th07e_ol.0042F498
        0xA1, 0x78, 0x62, 0x62, 0x00,    //MOV EAX,DWORD PTR DS:[626278]
        0xC7, 0x40, 0x04, 0x00, 0x00, 0x00, 0x00 //MOV DWORD PTR DS:[EAX+4],00000000
    };
    int offset = 0;
    int default_pos = 0x0042F483 - BASE_OFFSET;
    
    //sizes known at compile time so should work
    return load_mod(bytes, sizeof(bytes), swap, sizeof(swap), offset, default_pos);
}



//Keep power upon death
Mod* keep_power1() {
    char bytes[] = {
        0x6A, 0xF0         //PUSH -10 //-10 = -16?
    };    
    char swap[] = {
        0x6A, 0x00          //PUSH 0
    };
    int offset = 0;
    int default_pos = 0x00440DD0-BASE_OFFSET;
    
    return load_mod(bytes, sizeof(bytes), swap, sizeof(swap), offset, default_pos);
}

//Keep power upon death pt2
Mod* keep_power2() {
    char bytes[] = {
        0x74, 0x0F        //JE SHORT th07e_ol.00432603
    };    
    char swap[] = {
        0xEB, 0x0F        //JMP SHORT th07e_ol.00432603
    };
    int offset = 0;
    int default_pos = 0x00004325F2-BASE_OFFSET;
    
    return load_mod(bytes, sizeof(bytes), swap, sizeof(swap), offset, default_pos);
}


//Allocate and initialize a new mod struct
Mod* load_mod(char seq[], long len, char swap[], long swap_len, long offset, long pos) {
    Mod* mod = malloc(sizeof(Mod));
    
    mod->seq = malloc(len);
    memcpy(mod->seq, seq, len);
    mod->seq_len = len;
    
    mod->swap = malloc(swap_len);
    memcpy(mod->swap, swap, swap_len);
    mod->swap_len = swap_len;
    mod->offset = offset;
    
    mod->default_pos = pos;
    
    return mod;
}



void free_mod(Mod* mod) {
    free(mod->seq);
    free(mod->swap);
    free(mod);
}


int apply_mod(Mod* mod, char* buffer, long size, FILE* f_out) {
    long pos = -1;
    //Try checking at default pos first
    if (mod->default_pos != 0)
        pos = scan(mod->seq, mod->seq_len, buffer, size, mod->default_pos, mod->default_pos + mod->seq_len);
    //If not found, scan whole file
    if (pos == -1)
        pos = scan(mod->seq, mod->seq_len, buffer, size, 0, -1);
        
    //Not found, return error code
    if (pos == -1) 
        return -1;
    
    //No output file to write to
    if (f_out == NULL)
        return pos;
    
    //Write new byte sequence
    fseek(f_out, pos + mod->offset, SEEK_SET); 
    fwrite(mod->swap, 1, mod->swap_len, f_out);
    
    return pos;
}


//Search buffer between start, end for seq
//start inclusive, end exclusive
//work with longs in the case of larger files
long scan(char* seq, long seq_len, char* buffer, long buf_size, long start, long end) {
    if (end == -1)
        end = buf_size;
    
    //Bad start/end values
    if (start >= end || end - start < seq_len || end < 1)
        return -1;
    
    //Clip to [0,size)
    if (end > buf_size)
        end = buf_size;
    if (start < 0)
        start = 0;
    
    //search buffer for index of seq
    for (long i = start; i < end - seq_len + 1; i++) {
        int found = 1;
        for (int k = 0; k < seq_len; k++) {
            if (seq[k] != buffer[i+k]) {
                found = 0;
                break;
            }
        }
        if (found) 
            return i;
    }      
    
    return -1;
}





int main(int argc, char** argv) {
    
    if (argc < 2) {
        printf("Insufficient arguments.  Must specify at least input file. Output file optional");
        return 0;
    }
    
    //long MAX_BUFFER; //TODO: Modify to have a maximum buffer size in bytes
    
    //Open first file for reading
    FILE* f_in = fopen(argv[1], "rb");
    if (f_in == NULL) {
        printf("Could not open %s\n", argv[1]);
        exit(1);
    }
    
    //Open second file for writing
    FILE* f_out = NULL;
    if (argc > 2) {
        f_out = fopen(argv[2], "wb");
        if (f_out == NULL) {
            printf("Could not open output file %s\n", argv[2]);
            exit(1);
        }
    }
    
    //Determine size of input file in bytes
    //Change longs to long long? 
    long file_size; //max of ~2GB. may be needed if extending to modern games 
    fseek(f_in, 0, SEEK_END); 
    file_size = ftell(f_in);  
    fseek(f_in, 0, SEEK_SET); 
    
    //Read input file into buffer.
    //Currently reads entire file. Consider a maximum buffer size in the future.
    char* buffer = malloc(file_size); //file size * 1 byte
    if (buffer == NULL) {
        fclose(f_in);
        exit(1);
    }
    fread(buffer, 1, file_size, f_in);

    //Copy input file to output file
    if (f_out != NULL)
        fwrite(buffer, 1, file_size, f_out);
    
    int num_mods = 5;
    Mod* mods[num_mods];
    //Score mods first, in order to abort in case of fail later
    mods[0] = zero_score_display(); 
    mods[1] = zero_score_final();
    mods[2] = infinite_lives();
    mods[3] = keep_power1();
    mods[4] = keep_power2();
    
    //Search and apply each mod
    for (int i = 0; i < num_mods; i++) {
        printf("Part %d...\n", i);
        long result = apply_mod(mods[i], buffer, file_size, f_out);
        if (result == -1) {
            printf("Not found.\n");
            //Abort if score wasn't successfully applied.
            //Make sure no scores can be generated if infinite lives is applied
            if (i < 2) {
                printf("Failed to apply zero score patch. Aborting.\n");
                break;
            }
        }
        else
            printf("Found at: 0x%x(0x%x)\n", result, result+BASE_OFFSET);
    }
   
    free(buffer);
    for (int i = 0; i < num_mods; i++) {
        free_mod(mods[i]);
    }
    
    if (f_in != NULL)
        fclose(f_in); 
    if (f_out != NULL)
        fclose(f_out);
    
    printf("Done\n");
    
    return 0;
}






//Notes
//Worth saving just in case.

/*


No loss of lives, crashes?
Subtracts -1 from lives
-1 pushed onto stack from calling function

Caller
0044116A  |> 6A FF          PUSH -1
0044116C  |. B9 70626200    MOV ECX,th07.00626270
00441171  |. E8 57C4FEFF    CALL th07.0042D5CD



0042D5CD  /$ 55             PUSH EBP
0042D5CE  |. 8BEC           MOV EBP,ESP
0042D5D0  |. 51             PUSH ECX
0042D5D1  |. 57             PUSH EDI
0042D5D2  |. 894D FC        MOV DWORD PTR SS:[EBP-4],ECX
0042D5D5  |. 8B4D FC        MOV ECX,DWORD PTR SS:[EBP-4]
0042D5D8  |. E8 037AFDFF    CALL th07.00404FE0
0042D5DD  |. 85C0           TEST EAX,EAX
0042D5DF  |. 74 0F          JE SHORT th07.0042D5F0
0042D5E1  |. B9 B4000000    MOV ECX,0B4
0042D5E6  |. 83C8 FF        OR EAX,FFFFFFFF
0042D5E9  |. BF 50595700    MOV EDI,th07.00575950
0042D5EE  |. F3:AB          REP STOS DWORD PTR ES:[EDI]
0042D5F0  |> 8B45 FC        MOV EAX,DWORD PTR SS:[EBP-4]
0042D5F3  |. 8B40 08        MOV EAX,DWORD PTR DS:[EAX+8]
0042D5F6  |. DB45 08        FILD DWORD PTR SS:[EBP+8]
0042D5F9  |. D840 5C        FADD DWORD PTR DS:[EAX+5C]
0042D5FC  |. 8B45 FC        MOV EAX,DWORD PTR SS:[EBP-4]
0042D5FF  |. 8B40 08        MOV EAX,DWORD PTR DS:[EAX+8]
0042D602  |. D958 5C        FSTP DWORD PTR DS:[EAX+5C]
0042D605  |. 8B4D FC        MOV ECX,DWORD PTR SS:[EBP-4]
0042D608  |. E8 A33CFDFF    CALL th07.004012B0
0042D60D  |. 5F             POP EDI
0042D60E  |. C9             LEAVE
0042D60F  \. C2 0400        RETN 4

*/


/*
Don't lose powerups


00440DD2  |> 6A F0           PUSH -10                                 ; /Arg1 = FFFFFFF0
00440DD4  |. B9 70626200     MOV ECX,th07e_ol.00626270                ; |
00440DD9  |. E8 0218FFFF     CALL th07e_ol.004325E0                   ; \th07e_ol.004325E0

00440DD2  |> 6A F0           PUSH -10   
to
00440DD2  |> 6A 00           PUSH 0  
-0 only works on one type of death
-1 increases

TEST EAX,EAX will be false if EAX==0
004325F2     74 0F          JE SHORT th07e_ol.00432603
004325F2     EB 0F          JMP SHORT th07e_ol.00432603

Still resets if under 16


max=128?

*/



/*
Points

0 points
0042DF8B     0341 08        ADD EAX,DWORD PTR DS:[ECX+8]
0042DF8E     8B4D EC        MOV ECX,DWORD PTR SS:[EBP-14]
0042DF91  |. 8B49 08        MOV ECX,DWORD PTR DS:[ECX+8]
0042DF94  |. 8901           MOV DWORD PTR DS:[ECX],EAX
0042DF96  |. 8B45 EC        MOV EAX,DWORD PTR SS:[EBP-14]
0042DF99  |. 8B40 08        MOV EAX,DWORD PTR DS:[EAX+8]


0042DF8B     0341 08        ADD EAX,DWORD PTR DS:[ECX+8]
CHANGE TO
0042DF8B     83C0 00        ADD EAX,0
Crashes?

or
0042DF94  |. 8901           MOV DWORD PTR DS:[ECX],EAX
to
0042DF94  |. ????           MOV DWORD PTR DS:[ECX],0



Kills
0042DF94  |. 8901           MOV DWORD PTR DS:[ECX],EAX

Pickups
0042DFC2  |. 8901           MOV DWORD PTR DS:[ECX],EAX


END OF LEVEL
0042A575
0042F4A6
0042EEF5

0042EEF2     8B40 04        MOV EAX,DWORD PTR DS:[EAX+4] -> 31 C0 90 XOR EAX,EAX
0042EEF5     8901           MOV DWORD PTR DS:[ECX],EAX




Value 1(display?)
0042DF94 - 
0042DFC2 - 
0042A575  |. 8908           |MOV DWORD PTR DS:[EAX],ECX - eos 
0042F4A6 - eos
0042EEF5 - eos


1 = displayed score, 2 = actual score?????
so need to zero eax+4?

Value 2(more accurate?)
00421364 - hit counter
00421A21 - kill
0043303F - Red pickups
00433261 - Blue pickups
0043ED5C - ?
004336FF - ?
004219DB - boss
00421BFC - boss
004334FD - ?
0041021B - boss
00410282 - boss
...
Boss end or final points
004416CF
...
Boss end/stage end
0042B4F1
0042B50C
0042B527
0042B542
0042B55D
0042B578
0042B593 
0042B5AE
0042B5C9
0042B5E4 
...
Stage start/stage end
0042F28D
0042DEF4



0042DF55  |. 0341 08         ADD EAX,DWORD PTR DS:[ECX+8]


Cherry border 10,000 bonus?
0044167A  |. 68 10270000     PUSH 2710



Last 2 after end death
00421A18
00421A21
Score screen
0040425B
0040425B  |. 8B51 04         MOV EDX,DWORD PTR DS:[ECX+4]
Return to Title
0042F483
0042F4A3
???????
00446B83 - zeroing ecx here doesnt do it


0040425B
0042F483
0040425E
00446B83


Reimu is different?
0042DF6F
0042DFBF
Hitting dont continue(open score screen)
0040425B
0042F483 - max score? -maybe promising
0042F4A3
00446B83



CHANGED
0042F483     8378 04 00     CMP DWORD PTR DS:[EAX+4],0
0042F491     C740 04 000000>MOV DWORD PTR DS:[EAX+4],0
00446B83     33C9           XOR ECX,ECX
NOP
0040425B     33D2           XOR EDX,EDX
NOP




CHANGING TO THIS SEEMS TO REMOVE SCORE AT END SCREEN, BUT NOT ON SAVE?
0042F4A3     31C9            XOR ECX,ECX


?
004416CF
*/
