#include "kernel/types.h"
#include "user/user.h"


int main(int argc, char *argv[]) {
    int val;
    int i;
    
    for(i = 1;i < argc;i++) {
        val = atoi(argv[i]);
        printf("%d\n",val);
        sleep(val);
    }
    exit();
    return 0;
}
