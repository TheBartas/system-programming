#ifndef MSG_DATA
#define MSG_DATA

#include <stdbool.h>
#include <unistd.h>


#define MAX_FILE_SIZE 256
#define MAX_HASH_SIZE 512

typedef struct {
    int blck_idx;
    int frst_idx;
    int lst_idx;
    bool is_avbil;
} blckinfo; // aka task info

typedef struct { 
    long type;
    int frt_idx;
    int scd_idx;
    char file_name[MAX_FILE_SIZE];
} strdt; // stream data

typedef struct { 
    long type;
    int offset;
    int length;
    char file_name[MAX_FILE_SIZE];
    char hash[MAX_FILE_SIZE];
} strdt_2; // stream data

typedef struct {
    long type;
    bool rec;
    bool found;
} bckmsg; 

typedef struct {
    long type;
    int nwline;
    pid_t pid;
} errmsg;

typedef struct {
    long type;
    bool ready;
    int tasks;
    pid_t id;
} hllmsg;

#endif