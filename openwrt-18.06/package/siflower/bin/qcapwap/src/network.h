#ifndef __CAPWAP_CWStevens_HEADER__
#define __CAPWAP_CWStevens_HEADER__

#include <sys/socket.h>
#include <netinet/in.h>
#include "capwap_message.h"

#define	CW_CONTROL_PORT						5246
#define	CW_DATA_PORT						5247

/**
 * Wrap functions, these functions in the standard library are different for IPV4 and IPV6,
 * thus we use these functions to have a compatibility for future IPV6 uses.
 */
char *sock_ntop_r(const struct sockaddr_storage *sa, char *str);
int sock_get_port(struct sockaddr_storage *sa);
void *sock_get_addr(struct sockaddr_storage *sa);
int sock_cpy_addr_port(struct sockaddr_storage *sa1, const struct sockaddr_storage *sa2);
void sock_set_port_cw(struct sockaddr_storage *sa, int port);

/**
 * Init a UDP socket listen on the assigned port, connect to client if exist.
 * @port: which port to listen on.
 * @client: if not NULL, the socket will connect to it, thus we can only receive and send to the client
 * @client_len: length of the client socket address structure.
 * @Return: the socket number. If error happens, return negative error number.
 * This function will set SO_REUSEADDR and SO_REUSEPORT on the socket, therefore we can listen on the
 * same port with different client addresses.
 */
int capwap_init_socket(int port, struct sockaddr_storage *client, int client_len);
/**
 * The same as recvfrom(sock, buff, len, 0, addr, addr_len), but will continue if a EINTR error happens.
 */
ssize_t capwap_recv_message(int sock, void *buff, int len, struct sockaddr *addr, socklen_t *addr_len);
/**
 * Used for connected sockets, read socket packets into buff, return received length.
 * @sock: which socket to read
 * @buff: where to put the received data
 * @len: total length of the buffer
 * @return: negative number for error, otherwise return the received data length.
 */
ssize_t capwap_recv_ctrl_message(int sock, void *buff, int len);
/**
 * Send a message described by iovec on socket FD.
 * Returns 0 on success, or negative number for errors.
 */
int capwap_send_message(int sock, struct iovec *io, size_t iovlen, struct sockaddr_storage *addr, int addr_len);
/**
 * Send an unserialized capwap control message on a connected socket SOCK.
 * Returns 0 on success, or negative number for errors.
 */
int capwap_send_ctrl_message(int sock, struct cw_ctrlmsg *msg);
/**
 * Send an unserialized capwap control message on an unconnected socket SOCK.
 * Returns 0 on success, or negative number for errors.
 */
int capwap_send_ctrl_message_unconnected(int sock, struct cw_ctrlmsg *msg, struct sockaddr_storage *addr, int addr_len);
/**
 * Send an unserialized capwap control message response on socket SOCK.
 * Returns 0 on success, or negative number for errors.
 * As it is a response, the socket must be connected, and the msg will be destoried
 * after send in this function.
 */
int capwap_send_response(int sock, struct cw_ctrlmsg *msg);
/**
 * Init and return a Unix socket file descriptor
 * @path: the path of the Unix socket.
 * @Return: the socket fd
 * This function will init a UDP socket for Unix socket domain, and bind it
 * to the incoming path, and then return it.
 */
int capwap_init_interface_sock(char *path);

#endif	/* __CAPWAP_CWStevens_HEADER__ */