#ifndef	DCCSOCKH
#define DCCSOCKH 

#include "unixhdrs.h"

extern int tcp_open_sc_server(const char *listen_addr, int listen_port);
extern int tcp_connect_sc_server(char *server_addr, int server_port);
extern int tcp_open_server(const char *listen_addr, int listen_port) ;
extern int tcp_accept_client(int listen_sock,int *client_addr, 
                             int *client_port); 
extern int tcp_connet_server(char *server_addr, int server_port,
                             int client_port);
extern int tcp_close_socket(int sockfd); 

extern int tcp_check_readable(int conn_sockfd,int ntimeout); 
extern int tcp_check_writable(int conn_sockfd,int ntimeout); 
extern int tcp_read_nbytes(unsigned int conn_sock, void *buffer, int nbytes);
extern int tcp_write_nbytes(int conn_sock, const void *buffer, int nbytes);
extern int read_msg_from_net(int conn_sockfd, void *msg_buffer, 
                             int nbufsize,int ntimeout);
extern int write_msg_to_net(int conn_sockfd, void *msg_buffer, 
                            int nbytes,int ntimeout); 

extern int read_msg_from_CON(int conn_sockfd, void *msg_buffer,
                             int nbufsize,int ntimeout);
extern int write_msg_to_CON(int conn_sockfd, void *msg_buffer, int nbytes,int ntimeout);

extern int tcp_pton(char *strAddr);

extern char *BCDtoDEC(char *tagstr,unsigned char *srcstr,int srclen);

                        
#endif // DCCSOCKH 
