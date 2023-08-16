#include <stdio.h>
#include <malloc.h>
#include <errno.h>
#include <winsock.h>
#include <string.h>

//Used for each chunk of PNG 
//As defined here: https://en.wikipedia.org/wiki/PNG#File_format
struct Chunk {
    unsigned int length;
    unsigned int chunkType;
    char* chunkData;
    unsigned int crc;
};

/**
 * Checks first 8 bytes to see if it matches a PNG signature
 * Expected to see 89 50 4E 47 0D 0A 1A 0A
 * @param FILE* fp PNG FILE*
 * @return true iff the valid signature is present
*/
int check_signature(FILE* fp){
    char signature[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    char buf[8];
    int equal = 1;

    fread(buf, 1, 8, fp);

    for(int i=0; i<8; i++){
        equal = signature[i] == buf[i];
        if (!equal) {break;}
    }

    return equal;
}

/**
 * Prints the chunk name as a string
 * @param unsigned int chunk_type is the int representation of the chunk to print
 * @return void
*/
void print_chunk_type(unsigned int chunk_type){
    char read[5] = {0 * 5};
    memcpy(read, &chunk_type, 4);
    printf("Chunk Type: %s\n", read);
}

/**
 * Reads all chunks of PNG into a chunk array
 * @param FILE* fp PNG FILE*
 * @param int* num_chunks will be updated to the number of chunks stored in the array
 * @param struct Chunk* chunks is the chunk array to be updated
 * @return chunk array of all chunks in PNG
*/
struct Chunk* read_all_chunks(FILE* fp, int* num_chunks){
    int n = 0;
    struct Chunk* chunks = malloc(0);

    while(1) {
        //get len
        unsigned int len;
        if(fread(&len, 4, 1, fp) != 1){
            fprintf(stderr, "INVALID READ ON CHUNK %d LEN\n", n);
            break;
        }
        len = htonl(len);
        
        //get chunk type
        unsigned int chunk_type;
        if(fread(&chunk_type, 4, 1, fp) != 1){
            fprintf(stderr, "INVALID READ ON CHUNK %d CHUNK TYPE\n", n);
            break;
        }

        //check if IEND
        char isIEND = (chunk_type == *(unsigned int*)"IEND");

        //get chunk data
        //ignore for IEND because its len 0
        len = isIEND ? 1 : len;
        char chunk_data[len];
        if (!isIEND){ 
            if(fread(chunk_data, len, 1, fp) != 1){
                fprintf(stderr, "INVALID READ ON CHUNK %d: CHUNK DATA\n", n);
                break;
            }
        } else {
            chunk_data[0] = 0;
        }

        //get crc
        unsigned int crc;
        if(fread(&crc, 4, 1, fp) != 1){
            fprintf(stderr, "INVALID READ ON CHUNK %d CRC\n", n);
            break;
        }
        
        //init new chunk
        struct Chunk c;
        c.length = len;
        c.chunkType = chunk_type;
        c.chunkData = chunk_data;
        c.crc = crc;

        //add to chunk array
        n++;
        chunks = realloc(chunks, sizeof(struct Chunk) * n);
        chunks[n-1] = c;

        //end on last chunk
        if (isIEND){ 
            break;
        }
    }
    *num_chunks = n;
    return chunks;
}

/**
 * Reads the PNG file
 * @param char* filepath PNG file's path
 * @return -1 if error occurs 0 otherwise
*/  
int read_PNG(char* filepath){
    FILE *fp = fopen(filepath, "rb");

    //Break if file does not have PNG signature
    if (!check_signature(fp)) {
        fprintf(stderr, "INVALID SIG");
        return -1;
    }

    //read the chunks
    int num_chunks;
    struct Chunk* chunks = read_all_chunks(fp, &num_chunks);


    // uninit
    free(chunks);
    fclose(fp);
    return 0;
}

int main() {
    char* filepath = "test";
    char* imgpath = "DankChungus.png";
    char buf[1024];
    FILE *testp = fopen(filepath, "rb");

    int read = fread(buf, 1, 1024, testp);

    // sample read
    // for(int i = 0; i<read; i++){    
    //     printf("%c\n", buf[i]);
    // }

    printf("Image Below\n");

    read_PNG(imgpath);
    return 1;
}


