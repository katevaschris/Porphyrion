#include "router.h"
#include <arpa/inet.h>
#include <curl/curl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define PORT 8099
int main(void)
{
    int server_fd;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

#ifdef SIGPIPE
    signal(SIGPIPE, SIG_IGN);
#endif

    curl_global_init(CURL_GLOBAL_DEFAULT);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("socket");
        return 1;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt");
        close(server_fd);
        return 1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        close(server_fd);
        return 1;
    }
    if (listen(server_fd, 10) < 0)
    {
        perror("listen");
        close(server_fd);
        return 1;
    }

    struct stat st_dir;
    memset(&st_dir, 0, sizeof(st_dir));
    if (stat("data", &st_dir) == -1)
    {
        mkdir("data", 0755);
    }

    printf("Porphyrion v%s listening on port %d\n", PORPHYRION_VERSION, PORT);

    while (1)
    {
        int client = accept(server_fd, (struct sockaddr *)&addr, &addrlen);
        if (client < 0)
        {
            perror("accept");
            break;
        }
        handle_client(client);
    }

    printf("Shutting down.\n");
    curl_global_cleanup();
    close(server_fd);
    return 0;
}
