#include "server.h"
#include <getopt.h>

void help() {
    printf("Usage: server [options]\n");
    printf(
        "Options:\n"
        "    -h                | print this help\n"
        "    -p <port>         | specify port (default = 10000)\n"
        "    -t <thread num>   | specify nth (Number of threads)\n"
        "    -E                | use epoll, (default = not)\n"    
    );
}

int main(int argc, char** argv) {
    // TODO - cli args

    /* initial configuration */
    server_conf conf;
    server_conf_init(&conf);

    char ch = -1;
    while((ch = getopt(argc, argv, "hp:t:E")) != -1) {
        switch (ch) {
        case 'h':
            help();
            break;
        case 'p':
        {
            int port = atoi(optarg);
            if(port <= 0){
                printf("error: port is supposed to be a number > 0\n");
                exit(-1);
            }
            else
                conf.port = port;
            break;
        }
        case 't':
        {
            int nth = atoi(optarg);
            if(nth <= 0){
                printf("error: nth is supposed to be a number > 0\n");
                exit(-1);
            }
            else
                conf.nth = nth;
            break;
        }
        case 'E':
            conf.use_epoll = 1;
            break;
        default:
            help();
            break;
        }
    }

    run_server(&conf);
    return 0;
}
