#include "server.h"
#include <getopt.h>

int use_log_info = 0;
int use_log_err = 0;
int use_log_debug = 0;

void help() {
    printf("Usage: server [options]\n");
    printf(
        "Options:\n"
        "    -d                | enable log_debug (default = not)\n"
        "    -e                | enable log_err (default = not)\n"
        "    -i                | enable log_info (default = not)\n"
        "    -h                | print this help\n"
        "    -p <port>         | Set port (default = %d)\n"
        "    -t <thread num>   | Set nth (Number of threads, default = %d)\n",
        DEFAULT_PORT,
        DEFAULT_NTH
    );
}

int main(int argc, char** argv) {
    // TODO - cli args

    /* initial configuration */
    server_conf* conf = (server_conf*)malloc(sizeof(server_conf));
    server_conf_init(conf);

    char ch = -1;
    while((ch = getopt(argc, argv, "deihp:t:")) != -1) {
        switch (ch) {
        case 'd':
            use_log_debug = 1;
            break;
        case 'e':
            use_log_err = 1;
            break;
        case 'i':
            use_log_info = 1;
            break;
        case 'h':
            help();
            return 0;
        case 'p':
        {
            int port = atoi(optarg);
            if(port <= 0){
                printf("error: port is supposed to be a number > 0\n");
                help();
                exit(-1);
            }
            else
                conf->port = port;
            break;
        }
        case 't':
        {
            int nth = atoi(optarg);
            if(nth <= 0){
                printf("error: nth is supposed to be a number > 0\n");
                help();
                exit(-1);
            }
            else
                conf->nth = nth;
            break;
        }
        default:
            help();
            return 0;
        }
    }

    server_run(conf);
    free(conf);
    return 0;
}
