#include "server.h"

int main(int argc, char** argv) {
    // TODO - cli args
    run_server(MAX_TH_NUM, SERVER_PORT);
    return 0;
}