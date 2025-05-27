#ifndef MSG_DATA
#define MSG_DATA

#include <stdbool.h>
#include <unistd.h>


#define MAX_FILE_SIZE 256
#define MAX_HASH_SIZE 512

#define TYPE_BCKMSG_QUE 2
#define TYPE_HLLMSG_QUE 4
#define TYPE_FNDEND_BCKMSG_QUE 5

typedef struct {
    int blck_idx;
    int frst_idx;
    int lst_idx;
    bool isdone;
    pid_t pid;
} blckinfo; // aka task info

typedef struct {
    int t;
} thread_args_t;

// structures for queue

typedef struct { 
    long type;
    int frt_idx;
    int scd_idx;
    int tasks;
    char file_name[MAX_FILE_SIZE];
    char hash[MAX_HASH_SIZE];
} strdt; // stream data

typedef struct {
    long type;
    bool rec;
    bool found;
    int mtasks; // number of made tasks
    pid_t pid;
} bckmsg; 

typedef struct {
    long type;
    bool ready;
    int tasks; // declare how many tasks worker can do
    pid_t id;
} hllmsg;


#endif