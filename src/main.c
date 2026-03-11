#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <curl/curl.h>
#include <sys/stat.h>
#include <signal.h>
#include "router.h"

#define PORT 8099

int main() {
    int server_fd;
    struct sockaddr_in addr;
    int opt = 1, addrlen = sizeof(addr);

#ifdef SIGPIPE
    signal(SIGPIPE, SIG_IGN);
#endif

    curl_global_init(CURL_GLOBAL_DEFAULT);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { perror("socket"); exit(EXIT_FAILURE); }
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) { perror("bind"); exit(EXIT_FAILURE); }
    if (listen(server_fd, 10) < 0) { perror("listen"); exit(EXIT_FAILURE); }

    // Ensure data directory exists.
    struct stat st_dir = {0};
    if (stat("data", &st_dir) == -1) mkdir("data", 0755);

    printf("Porphyrion v%s listening on port %d\n", PORPHYRION_VERSION, PORT);

    while (1) {
        int client = accept(server_fd, (struct sockaddr *)&addr, (socklen_t *)&addrlen);
        if (client < 0) { perror("accept"); break; }
        handle_client(client);
    }

    printf("Shutting down.\n");
    curl_global_cleanup();
    close(server_fd);
    return 0;
}
