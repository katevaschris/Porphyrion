#ifndef ROUTER_H
#define ROUTER_H

/*
 * Entry point for client connections.
 *
 * @param sock client socket fd
 */
void handle_client(int sock);

#endif
