#include "router.h"
#include <arpa/inet.h>
#include <curl/curl.h>
#include <errno.h>
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
    (void)signal(SIGPIPE, SIG_IGN);
#endif

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK)
    {
        fprintf(stderr, "curl_global_init failed\n");
        return 1;
    }

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
        (void)close(server_fd);
        return 1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        (void)close(server_fd);
        return 1;
    }
    if (listen(server_fd, 10) < 0)
    {
        perror("listen");
        (void)close(server_fd);
        return 1;
    }

    struct stat st_dir;
    memset(&st_dir, 0, sizeof(st_dir));
    if (stat("data", &st_dir) == -1)
    {
        if (mkdir("data", 0755) != 0 && errno != EEXIST)
        {
            perror("mkdir");
        }
    }

    (void)printf("Porphyrion %s listening on port %d\n", PORPHYRION_VERSION,
                 PORT);

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

    (void)printf("Shutting down.\n");
    curl_global_cleanup();
    close(server_fd);
    return 0;
}
