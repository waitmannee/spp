#include "dccsock.h"
#include  "dccglobl.h"
#include  "dccpgdef.h"
#include  "dccdcs.h"

#define closesocket  close

int tcp_open_sc_server(const char *listen_addr, int listen_port)
{
    //description:
    //this function is called by the server end.
    //before listening on socket waiting for requests
    //from clients, the server should create the socket,
    //then bind its port to it.All is done by this function

    //arguments:
    //listen_addr--IP address or host name of the server,maybe NULL
    //listen_port--port# on which the server listen on

    int   arg=1;
    int   sock;
    struct sockaddr_in	sin;

    //create the socket
    sock=socket(AF_INET,SOCK_STREAM,0);
    if(sock < 0)
	return -1;

    //get the IP address and port#
    memset(&sin,0,sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port   = htons(listen_port);

    if(listen_addr == NULL)
        sin.sin_addr.s_addr = htonl(INADDR_ANY);
    else
    {
        int  addr;
	if ((addr=inet_addr(listen_addr)) != INADDR_NONE)
	    sin.sin_addr.s_addr = addr;
	else	//'listen_addr' may be the host name
	{
	    struct hostent *ph;

	    ph=gethostbyname(listen_addr);
	    if(!ph)
		goto lblError;
	    sin.sin_addr.s_addr = ((struct in_addr *)ph->h_addr)->s_addr;
	}
    }

    //set option for socket and bind the (IP,port#) with it
    arg=1;
    setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(char *)&arg,sizeof(arg));
    if(bind(sock,(struct sockaddr *)&sin,sizeof(sin)) < 0)
	goto lblError;

    //put the socket into passive open status
    if(listen(sock,128*8) < 0)
	goto lblError;

    return sock;

lblError:
    if(sock >= 0)
	closesocket(sock);
    return -1;
}


//短链接时客户端连接远方服务器

int tcp_connect_sc_server(char *server_addr, int server_port)
{   
    int    arg, sock, addr;
    struct sockaddr_in	sin;
    
    //the address of server must be presented
    if(server_addr == NULL || server_port == 0)
        return -1;
    
    //create the socket
    sock=socket(AF_INET,SOCK_STREAM,0);
    if(sock < 0)
        return -1;
        
    //prepare the address of server
    memset(&sin,0,sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port   = htons(server_port);
    
    if ((addr=inet_addr(server_addr)) != INADDR_NONE)
        sin.sin_addr.s_addr = addr;
    else  //'server_addr' may be the host name
    {
        struct hostent *ph;
        ph = gethostbyname(server_addr);
        if(!ph)
            goto lblError;
	char buf[32];
	dcs_log(0,0," first ip: %s\n", inet_ntop(ph->h_addrtype, ph->h_addr, buf, sizeof(buf)));
        
        sin.sin_addr.s_addr = ((struct in_addr *)ph->h_addr)->s_addr;
    }
    
    //try to connect to server
    if(connect(sock,(struct sockaddr *)&sin,sizeof(sin)) < 0)
        goto lblError;
    return sock;
    
lblError:
    if(sock >= 0)
        closesocket(sock);
    return -1;
}



int tcp_open_server(const char *listen_addr, int listen_port)
{
    //description:
    //this function is called by the server end.
    //before listening on socket waiting for requests
    //from clients, the server should create the socket,
    //then bind its port to it.All is done by this function

    //arguments:
    //listen_addr--IP address or host name of the server,maybe NULL
    //listen_port--port# on which the server listen on

    int   arg=1;
    int   sock;
    struct sockaddr_in	sin;

    //create the socket
    sock=socket(AF_INET,SOCK_STREAM,0);
    if(sock < 0)
	return -1;

    //get the IP address and port#
    memset(&sin,0,sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port   = htons(listen_port);

    if(listen_addr == NULL)
        sin.sin_addr.s_addr = htonl(INADDR_ANY);
    else
    {
        int  addr;
	if ((addr=inet_addr(listen_addr)) != INADDR_NONE)
	    sin.sin_addr.s_addr = addr;
	else	//'listen_addr' may be the host name
	{
	    struct hostent *ph;

	    ph=gethostbyname(listen_addr);
	    if(!ph)
		goto lblError;
	    sin.sin_addr.s_addr = ((struct in_addr *)ph->h_addr)->s_addr;
	}
    }

    //set option for socket and bind the (IP,port#) with it
    arg=1;
    setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(char *)&arg,sizeof(arg));
	
    int keepAlive = 1; // 开启keepalive属性
    int keepIdle = 600; // 如该连接在600秒内没有任何数据往来,则进行探测 
    int keepInterval = 5; // 探测时发包的时间间隔为5 秒
    int keepCount = 3; // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.

    setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepAlive, sizeof(keepAlive));
    setsockopt(sock, SOL_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(keepIdle));
    setsockopt(sock, SOL_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval));
    setsockopt(sock, SOL_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount));
    if(bind(sock,(struct sockaddr *)&sin,sizeof(sin)) < 0)
	goto lblError;

    //put the socket into passive open status
    if(listen(sock,5) < 0)
	goto lblError;

    return sock;

lblError:
    if(sock >= 0)
	closesocket(sock);
    return -1;
}

//---------------------------------------------------------------------------

int tcp_accept_client(int listen_sock,int *client_addr, int *client_port)
{
    struct sockaddr_in cliaddr;
    int  addr_len;
    int  conn_sock;

    for(;;)
    {
        //try accepting a connection request from clients
		dcs_log(0, 0, "hst.....dcssock....tcp_accept_client...before, listen_sock = %d", listen_sock);
        addr_len  = sizeof(cliaddr);
        conn_sock = accept(listen_sock, (struct sockaddr *)&cliaddr, &addr_len);
		dcs_log(0, 0, "hst.....dcssock....tcp_accept_client...before, listen_sock = [%d], conn_sock = [%d]", listen_sock, conn_sock);

        if(conn_sock >=0)
            break;
        if(conn_sock < 0 && errno == EINTR)
            continue;
        else
            return -1;
    }

    //bring the client info (IP_address, port#) back to caller
    if(client_addr)
        *client_addr = cliaddr.sin_addr.s_addr;
    if(client_port)
        *client_port = ntohs(cliaddr.sin_port);

    return conn_sock;
}

//---------------------------------------------------------------------------

int tcp_connet_server(char *server_addr, int server_port,int client_port)
{
    //this function is performed by the client end to
    //try to establish connection with server in blocking
    //mode
    
    int    arg, sock, addr;
    struct sockaddr_in	sin;
    
    //the address of server must be presented
    if(server_addr == NULL || server_port == 0)
        return -1;
    
    //create the socket
    sock=socket(AF_INET,SOCK_STREAM,0);
    if(sock < 0)
        return -1;
    
    //set option for socket
    arg=1;
    setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(char *)&arg,sizeof(arg));
    
    //if 'client_port' presented,then do a binding
    if(client_port > 0)
    {
        memset(&sin,0,sizeof(struct sockaddr_in));
        sin.sin_family = AF_INET;
        sin.sin_port   = htons(client_port);
        sin.sin_addr.s_addr = htonl(INADDR_ANY);
        
        arg=1;
        setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(char *)&arg,sizeof(arg));
        if(0 > bind(sock,(struct sockaddr *)&sin,sizeof(sin)))
            goto lblError;
    }
    int keepAlive = 1; // 开启keepalive属性
    int keepIdle = 600; // 如该连接在600秒内没有任何数据往来,则进行探测 
    int keepInterval = 5; // 探测时发包的时间间隔为5 秒
    int keepCount = 3; // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.

    setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepAlive, sizeof(keepAlive));
    setsockopt(sock, SOL_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(keepIdle));
    setsockopt(sock, SOL_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval));
    setsockopt(sock, SOL_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount));

    //prepare the address of server
    memset(&sin,0,sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port   = htons(server_port);
    
    if ((addr=inet_addr(server_addr)) != INADDR_NONE)
        sin.sin_addr.s_addr = addr;
    else  //'server_addr' may be the host name
    {
        struct hostent *ph;
        ph = gethostbyname(server_addr);
        if(!ph)
            goto lblError;
        
        sin.sin_addr.s_addr = ((struct in_addr *)ph->h_addr)->s_addr;
    }
    
    //try to connect to server
    if(connect(sock,(struct sockaddr *)&sin,sizeof(sin)) < 0)
        goto lblError;
    return sock;
    
lblError:
    if(sock >= 0)
        closesocket(sock);
    return -1;
}

//---------------------------------------------------------------------------

int tcp_close_socket(int sockfd)
{
    return closesocket(sockfd);
}

//---------------------------------------------------------------------------

int tcp_write_nbytes(int conn_sock, const void *buffer, int nbytes)
{
	int  nleft,nwritten;
	char *ptr;
	for(ptr = (char *)buffer, nleft = nbytes; nleft > 0;)
	{
		//if ((nwritten = (int)write(conn_sock, ptr, nleft)) <= 0)
		if ((nwritten = (int)send(conn_sock, ptr, nleft, MSG_DONTWAIT)) <= 0)
		{
			if (errno == EINTR)
			{
				errno = -1;
				nwritten = 0; //and call write() again
			}
			else
			{
				return -1;  //error
			}
		}

		nleft -= nwritten;
		ptr   += nwritten;
	}//for

	return  (nbytes - nleft);
}

//---------------------------------------------------------------------------
int tcp_read_nbytes(unsigned int conn_sock, void *buffer, int nbytes)
{
	int    nread = 0;
	char	*ptr=(char *)buffer;
	while(1)
	{
		if ((nread = (int)recv(conn_sock, ptr, nbytes,MSG_DONTWAIT)) < 0)
		{
			if(errno == EINTR)
			{
				errno = 0;
			 	continue; // and call read() again
			}
			else
			{
				//dcs_log(0, 0, "errno = %d",errno);
				return(-1);
			}
		}
		else if (nread == 0)
		{
			break;  //EOF
		}

		break;
	} //for
	return nread;  // return >= 0
}

//---------------------------------------------------------------------------

#define  CHECK_READABLE   1
#define  CHECK_WRITABLE   2

static int tcp_check_rw_helper( int conn_sockfd,int ntimeout,int check_what);

int tcp_check_readable(int conn_sockfd,int ntimeout)
{
    //determine if the socket is readable within 'ntimeout' ms.
    //if readable, this function returns 0
    //if timeout,  returns -1 and errno == ETIMEDOUT
    //other error, returns -1 and error indicates the detailed info
    return tcp_check_rw_helper(conn_sockfd,ntimeout,CHECK_READABLE);
}

int tcp_check_writable(int conn_sockfd,int ntimeout)
{
    //determine if the socket is writable within 'ntimeout' ms.
    //if writable, this function returns 0
    //if timeout,  returns -1 and errno == ETIMEDOUT
    //other error, returns -1 and error indicates the detailed info 
    return  tcp_check_rw_helper(conn_sockfd,ntimeout,CHECK_WRITABLE);
}

//---------------------------------------------------------------------------

static int tcp_check_rw_helper(int conn_sockfd,int ntimeout,int check_what)
{
    int    nready = 0;
    fd_set sock_set, *prset=NULL, *pwset=NULL;
    struct timeval  tmval, *tmval_ptr=NULL;
    //convert the time in ms to timeval struct
    memset(&sock_set,0,sizeof(fd_set));
    memset(&tmval,0,sizeof(struct timeval));
    if(ntimeout < 0) //waiting in blocked mode
        tmval_ptr = NULL;
    else
    {
        tmval_ptr = &tmval;
        tmval_ptr->tv_sec  = ntimeout / 1000;
        tmval_ptr->tv_usec = (ntimeout % 1000) * 1000;
    }
    for(;;)
    {
        //select the connected socket
        FD_ZERO(&sock_set);
        FD_SET(conn_sockfd, &sock_set);
        //wait for readability or writability
        switch(check_what)
        {
            case CHECK_READABLE:
                 prset = &sock_set;
                 pwset = NULL;
                 break;

            case CHECK_WRITABLE:
            default:
                 prset = NULL;
                 pwset = &sock_set;
                 break;
        }//switch
        nready = select(conn_sockfd+1,prset,pwset,NULL,tmval_ptr);
        if(nready < 0)
        {
            if(errno == EINTR)  //select again
                 continue;
	      dcs_log(0, 0, "sock: other error");
            return -1;
        }
        else  if(nready == 0)  //timeout
        {
            errno = ETIMEDOUT;
	     dcs_log(0, 0, "sock: timeout ");
            return -1;
        }
        else  //ok
        {
		return 0;
	}    
    }//for
}

//--------------------------------------------------------------------------
 //
/*从socket中读取报文,报文格式为
*
* bnk
* 报文前面4位为包长,最大包长度为9999,长度位和内容必须在一个byteArray里面发送
* 如长度为185的包，包头就是 "0185"
* 返回值:-1表示读网络失败
*                   -2表示读到的报文不符报文格式或是报文过长
*                    >=0表示读到的报文长度
*/
int read_msg_from_net(int conn_sockfd, void *msg_buffer,int nbufsize,int ntimeout)
{ 
	char  buffer[5];
	int   ret = 0, bytes_to_read = 0, msg_length = 0;
	memset(buffer,0,sizeof(buffer));

	errno = -1;
	// dcs_log(0, 0, "before: tcp_check_readable");
	if(tcp_check_readable(conn_sockfd,ntimeout) < 0)
	{
		//dcs_log(0, 0, "sock: timeout or other error");
		return -1;
	}
	//dcs_log(0, 0, "atfer: tcp_check_readable");

	ret = tcp_read_nbytes(conn_sockfd, buffer, 4);
	if(ret <= 0) //read error
	{
		dcs_log(0, 0, "sock: read error ,ret = %d",ret);
		return -1;
	}
	else if(ret <4)
	{
		dcs_debug(buffer, ret, "读到的报文不符合规则");
		return -2;
	}
	//dcs_debug(buffer, 4, "sock read buffer: ");

	buffer[4]   = 0;
	if(strlen(buffer) < 4)
	{
		dcs_debug(buffer, 4, "读到的报文不符合规则");
		return -2;
	}
	ret = check_str_or_int(buffer);
	if(ret < 0)
	{
		dcs_debug(buffer, 4, "读到的报文不符合规则");
		return -2;
	}
	msg_length = atoi(buffer);
	if(msg_length == 0)
	{
		return 0;
	}
	char bigbuff[msg_length +5];
	memset(bigbuff , 0, sizeof(bigbuff));
	memcpy(bigbuff , buffer,4);
	//dcs_log(0, 0, "msg_length ---- %d", msg_length);

	if(msg_length > nbufsize)
	{
		ret = tcp_read_nbytes(conn_sockfd, bigbuff+4, msg_length);
		if(ret <= 0)
		{
			dcs_debug(bigbuff, 4 , "读到的报文不符合规则");
			return -2;
		}
		else if(ret < msg_length)
		{
			if(ret +4 < 100 )
				dcs_debug(bigbuff, ret +4 , "读到的报文不符合规则,长度=%d",ret+4);
			else 
				dcs_debug(bigbuff, 100 , "读到的报文不符合规则,读到的报文太长(%d)只显示部分报文",ret+4);
			return -2;
		}
		else
		{
			if(ret +4 < 100 )
				dcs_debug(bigbuff, ret +4 , "读到的报文太长(%d),大于能接收的长度%d丢弃",ret+4,nbufsize);
			else 
				dcs_debug(bigbuff, 100 , "读到的报文太长(%d),大于能接收的长度%d丢弃",ret+4,nbufsize);
			return -2;
		}
	}	
	else
	{
		ret = tcp_read_nbytes(conn_sockfd, bigbuff+4, msg_length);
		 if( ret <= 0)
		{
			dcs_debug(bigbuff, 4 , "读到的报文不符合规则");
			return -2;
		}
		else if(ret < msg_length)
		{
			if(ret +4 < 100 )
				dcs_debug(bigbuff, ret +4 , "读到的报文不符合规则,长度=%d",ret+4);
			else 
				dcs_debug(bigbuff, 100 , "读到的报文不符合规则,读到的报文太长(%d)只显示部分报文",ret+4);
			return -2;
		}
		else//正确读取报文
		{
			memcpy(msg_buffer,bigbuff+4,msg_length);
			return msg_length;
		}
	}
}

int tcp_pton(char *strAddr)
{
	int addr;

	if(strAddr==NULL || *strAddr==0)
		return -1;

	if((addr=inet_addr(strAddr)) != INADDR_NONE)
		return addr;
	else  //'strAddr' may be the host name
	{
		struct hostent *ph;
		ph=gethostbyname(strAddr);
		if(!ph)
			return -1;

		return ((struct in_addr *)ph->h_addr)->s_addr;
	}
}

//--------------------------------------------------------------------
int write_msg_to_SWnet(int conn_sockfd, void *msg_buffer, int nbytes,int ntimeout)
{
    //
    // write a message to switch. the format of a message
    // is (length_indictor, message_content)
    //

    int   ret, bytes_to_write;
    struct nac_header sndNacHeader;
    unsigned char _bcd[3];
    //clear the error number
    errno = -1;

    //check for writablity within 'ntimeout' ms
    if(tcp_check_writable(conn_sockfd,ntimeout) < 0)
        //timeout or other error
        return -1;

    //first 4 bytes is length indictor
    if(nbytes < 0)  nbytes = 0;
    
    bytes_to_write=nbytes + sizeof(struct nac_header) - 2;
    sndNacHeader.length[0]=bytes_to_write / 256;
    sndNacHeader.length[1]=bytes_to_write - (sndNacHeader.length[0] * 256);
    
    ret = tcp_write_nbytes(conn_sockfd, &sndNacHeader, sizeof(sndNacHeader));
    if(ret < sizeof(sndNacHeader)) //write error
        return -1;
        
    //write the message
    if(nbytes > 0 && msg_buffer)
    {
        ret = tcp_write_nbytes(conn_sockfd, msg_buffer, nbytes);
        if(ret < nbytes) //write error
            return -1;
    }

    //return the number of bytes written to socket
    return ret;
}

//-------------------------------------------------------------------------

int read_msg_from_SWnet(int conn_sockfd, void *msg_buffer,
                      int nbufsize,int ntimeout)
{
    //
    // read a message from switch. the format of a message
    // is (length_indictor, message_content)
    //

    char  buffer[256];
    int   ret, bytes_to_read, msg_length,flags;
    unsigned char _bcd[3];
    struct nac_header rcvNacHeader;

    //clear the error number
    errno = -1;

    //check for readability within 'ntimeout' ms
    if(tcp_check_readable(conn_sockfd,ntimeout) < 0)
    {
        //timeout or other error
        closesocket(conn_sockfd); 
	return -1;
    }

    //first round read NAC header
    ret = tcp_read_nbytes(conn_sockfd, &rcvNacHeader, sizeof(rcvNacHeader));
    if(ret < sizeof(rcvNacHeader)) //read error
    {
	closesocket(conn_sockfd);
	    return -1;
    }
    dcs_debug((char *)&rcvNacHeader,sizeof(rcvNacHeader)," recv msg header len =%d",sizeof(rcvNacHeader));    
    msg_length = (rcvNacHeader.length[0]) * 256 +  rcvNacHeader.length[1];
    msg_length = msg_length - sizeof(rcvNacHeader) + 2;
 
    //read the message
    bytes_to_read = min(nbufsize, msg_length);
    if(bytes_to_read > 0)
    {
        ret = tcp_read_nbytes(conn_sockfd, msg_buffer, bytes_to_read);
		dcs_log(0,0,"read net msg ret=%d",ret);
        if(ret < bytes_to_read) //read error
        {
            closesocket(conn_sockfd);
            return -1;
        }

        if(bytes_to_read < msg_length)
        {
            //the 'msg_buffer' presented by caller is too small to hold
            //all data contained in socket, we must purge this extra bytes

            int nleft, nbytes;

            for(nleft= msg_length - bytes_to_read;nleft > 0; )
            {
                nbytes = min(sizeof(buffer), nleft);
//		dcs_log(0,0,"msg len=%d",nbytes);
                ret = tcp_read_nbytes(conn_sockfd,buffer,nbytes);
                if(ret < nbytes || ret<=0)
       		{         
	 		closesocket(conn_sockfd);	
		    return -1;
		}

                nleft -= ret;
            }//for
        }
    }

    //return the number of bytes placed into 'msg_buffer'
    return bytes_to_read;
}

int read_msg_from_AMP(int conn_sockfd, void *msg_buffer, int nbufsize,int ntimeout)
{
	//
	//没有包长
	//
	int   ret = 0, bytes_to_read = 0;

	//clear the error number
	errno = -1;

	//check for readability within 'ntimeout' ms
	if(tcp_check_readable(conn_sockfd,ntimeout) < 0)
	{
		return -1;
	}
	
	bytes_to_read=0;
	while(1)
	{
		ret = tcp_read_nbytes(conn_sockfd,(char *)msg_buffer+bytes_to_read,1);
		if( ret >0)
		{
			bytes_to_read++;
		}
		else if ( ret ==0 )
			break;
		else
		{
			if( bytes_to_read <=0)
				return -1;
			break;
		}
	}//for

	// 这里表示对端的socket已正常关闭.
	if(bytes_to_read == 0)
		return -1;
	
	return bytes_to_read;
}

////--------------------------------------------------------------------------
int write_msg_to_NAC(int conn_sockfd, void *msg_buffer, int nbytes,int ntimeout)
{
    //
    // write a message to net. the format of a message
    // is (length_indictor, message_content)
    //
    if(nbytes < 0)  nbytes = 0;

    char  buffer[nbytes+4];
    int   ret;
    //clear the error number
    errno = -1;
    memset(buffer,0,sizeof(buffer));
    //check for writablity within 'ntimeout' ms
    if(tcp_check_writable(conn_sockfd,ntimeout) < 0)
        //timeout or other error
        return -1;
    //first 4 bytes is length indictor
    buffer[0]=(unsigned char)(nbytes/256);
    buffer[1]=(unsigned char)(nbytes%256);
    memcpy(buffer+2,msg_buffer,nbytes);
    //write the message
    //    if(nbytes >= 0 && msg_buffer) //Freax TO DO: always true remove it
    {
        ret = tcp_write_nbytes(conn_sockfd, buffer, nbytes+2);
        if(ret < (nbytes+2)) //write error
            return -1;
    }
    
    //return the number of bytes written to socket
    return ret-2;
}

int write_msg_to_AMP(int conn_sockfd, void *msg_buffer, int nbytes,int ntimeout)
{
    //
    // write a message to net. the format of a message
    // is (length_indictor, message_content)
    //
    
    int   ret;
    
    //clear the error number
    errno = -1;
    //check for writablity within 'ntimeout' ms
    if(tcp_check_writable(conn_sockfd,ntimeout) < 0)
        //timeout or other error
        return -1;
    
    ret = tcp_write_nbytes(conn_sockfd, msg_buffer, nbytes);

    if(ret < nbytes) //write error
        return -1;
    //return the number of bytes written to socket
    return ret;
}
//--------------------------------------------------------------------------
int write_msg_to_CON(int conn_sockfd, void *msg_buffer, int nbytes,int ntimeout)
{    
	/**con
	 * 报文前面3位为包长，其中首位是固定包头'0x03',后面两字节为BCD码压缩的16进制长度.
	 * 如长度为2044的包，包头就是 [03 20 44] 
	 */
	if(nbytes < 0)  
	{
		nbytes = 0;
	}
	char  buffer[nbytes+4];
	char tmp[10];
	int   ret;

	//clear the error number
	errno = -1;
	   //check for writablity within 'ntimeout' ms
    	if(tcp_check_writable(conn_sockfd,ntimeout) < 0)
    	{
    		//timeout or other error
       	 return -1;
    	}
	memset(buffer,0,sizeof(buffer));
	memset(tmp,0,sizeof(tmp));
	sprintf(tmp,"%04d",nbytes);
	//first 3 bytes is length indictor
	
	buffer[0]=0x03;
	asc_to_bcd(buffer+1, tmp, 4, 0);
	memcpy(buffer+3,msg_buffer,nbytes);
	ret = tcp_write_nbytes(conn_sockfd, buffer, nbytes+3);
	if(ret < (nbytes+3)) //write error
		return -1;

	return ret-3;
}
 //
/*从socket中读取报文,报文格式为
*
*con
 * 报文前面3位为包长，其中首位是固定包头'0x03',后面两字节为BCD码压缩的16进制长度.
 * 如长度为2044的包，包头就是 [0x03 0x20 0x44] 
 * 返回值:-1表示读网络失败
*                   -2表示读到的报文不符报文格式或是报文过长
*                    >=0表示读到的报文长度
*/
int read_msg_from_CON(int conn_sockfd, void *msg_buffer,int nbufsize,int ntimeout)
{
	unsigned char buffer[4];
	char tmp[5];
	int  ret = 0, bytes_to_read = 0, msg_length = 0;

	//clear the error number
	errno = -1;

	//check for readability within 'ntimeout' ms
	if(tcp_check_readable(conn_sockfd,ntimeout) < 0)
	{
		//timeout or other error
		return -1;
	}

	memset(buffer,0,sizeof(buffer));
	ret = tcp_read_nbytes(conn_sockfd,(char *)buffer,3);
	//dcs_debug(buffer,ret,"<buffer:--ret =%d>",ret);
	if(ret <= 0) //read error
	{
		dcs_log(0, 0, "sock: read error ,ret = %d",ret);
		return -1;
	}
	else if(ret <3)
	{
		dcs_debug(buffer, ret, "读到的报文不符合规则");
		return -2;
	}
	buffer[3]   = 0;
	if(buffer[0] != 0x03)
	{
		dcs_debug(buffer, ret, "读到的报文不符合规则");
		return -2;
	}
	memset(tmp,0,sizeof(tmp));
	bcd_to_asc(tmp,(unsigned char *)buffer+1,4,0);
	ret = check_str_or_int(tmp);
	if(ret < 0)
	{
		dcs_debug(buffer, 3, "读到的报文不符合规则");
		return -2;
	}
	msg_length = atoi(tmp); 
	if(msg_length == 0)
	{
		return 0;
	}
	
	char bigbuff[msg_length +4];
	memset(bigbuff , 0, sizeof(bigbuff));
	memcpy(bigbuff , buffer,3);

	if(msg_length > nbufsize)
	{
		ret = tcp_read_nbytes(conn_sockfd, bigbuff+3, msg_length);
		if(ret <=0)
		{
			dcs_debug(buffer, 3 , "读到的报文不符合规则");
			return -2;
		}
		else if(ret < msg_length)
		{
			if(ret +3 < 100 )
				dcs_debug(bigbuff, ret +3 , "读到的报文不符合规则,长度=%d",ret+3);
			else 
				dcs_debug(bigbuff, 100 , "读到的报文不符合规则,读到的报文太长(%d)只显示部分报文",ret+3);
			return -2;
		}
		else
		{	if(ret +3 < 100 )
				dcs_debug(bigbuff, ret +3 , "读到的报文太长(%d),大于能接收的长度%d丢弃",ret+3,nbufsize);
			else 
				dcs_debug(bigbuff, 100 , "读到的报文太长(%d),大于能接收的长度%d丢弃",ret+3,nbufsize);
			return -2;
		}
	}	
	else
	{
		ret = tcp_read_nbytes(conn_sockfd, bigbuff+3, msg_length);
		if( ret <= 0)
		{
			dcs_debug(buffer, 3 , "读到的报文不符合规则");
			return -2;
		}
		else if(ret < msg_length)
		{
			if(ret +3 < 100 )
				dcs_debug(bigbuff, ret +3 , "读到的报文不符合规则,长度=%d",ret+3);
			else 
				dcs_debug(bigbuff, 100 , "读到的报文不符合规则,读到的报文太长(%d)只显示部分报文",ret+3);
			return -2;
		}
		else//正确读取报文
		{
			memcpy(msg_buffer,bigbuff+3,msg_length);
			return msg_length;
		}
	}    
}
//--------------------------------------------------------------------------


int write_msg_to_net(int conn_sockfd, void *msg_buffer, int nbytes,int ntimeout)
{
	//
	// write a message to net. the format of a message
	// is (length_indictor, message_content)
	//
	if(nbytes < 0)  
	{
		nbytes = 0;
	}
	char  buffer[nbytes+5];
	int   ret;
	memset(buffer,0,sizeof(buffer));
	//clear the error number
	errno = -1;

	//check for writablity within 'ntimeout' ms
	if(tcp_check_writable(conn_sockfd,ntimeout) < 0)
		//timeout or other error
		return -1;
	//first 4 bytes is length indictor

	sprintf(buffer,"%.4d",nbytes);
	memcpy(buffer+4,msg_buffer,nbytes);
	ret = tcp_write_nbytes(conn_sockfd, buffer, nbytes+4);
	if(ret < (nbytes+4)) //write error
		return -1;
	//return the number of bytes written to socket
	return ret-4;
}

////--------------------------------------------------------------------------
 //
/*从socket中读取报文,报文格式为
*
/**trm 格式
* 报文前面2位为包长，双字节16进制压缩
* 例如300字节长报文，长度填[0x01 0x2C]
	
* 返回值:-1表示读网络失败
*                   -2表示读到的报文不符报文格式或是报文过长
*                    >=0表示读到的报文长度
*/

int read_msg_from_NAC(int conn_sockfd, void *msg_buffer, int nbufsize,int ntimeout)
{
	char  buffer[3];
	int   ret = 0, bytes_to_read = 0, msg_length = 0 ;

	errno = -1;
	memset(buffer,0,sizeof(buffer));

	if(tcp_check_readable(conn_sockfd,ntimeout) < 0)
	{
		//timeout or other error
		return -1;
	}
	
	ret = tcp_read_nbytes(conn_sockfd, buffer, 2);	
	if(ret <= 0) //read error
	{
		dcs_log(0, 0, "sock: read error ,ret = %d",ret);
		return -1;
	}
	else if(ret <2)
	{
		dcs_debug(buffer, ret, "读到的报文不符合规则");
		return -2;
	}
	buffer[2]   = 0;
	msg_length = (unsigned char)buffer[0]*256+(unsigned char)buffer[1];
	if(msg_length == 0)
	{
		return 0;
	}
	char bigbuff[msg_length +3];
	memset(bigbuff , 0, sizeof(bigbuff));
	memcpy(bigbuff , buffer,2);

	if(msg_length > nbufsize)
	{
		ret = tcp_read_nbytes(conn_sockfd, bigbuff+2, msg_length);
		if(ret <=0)
		{
			dcs_debug(bigbuff, 2 , "读到的报文不符合规则");
			return -2;
		}
		else if(ret < msg_length)
		{
			if(ret +2 < 100 )
				dcs_debug(bigbuff, ret +2 , "读到的报文不符合规则,长度=%d",ret+2);
			else 
				dcs_debug(bigbuff, 100 , "读到的报文不符合规则,读到的报文太长(%d)只显示部分报文",ret+2);
			return -2;
		}
		else
		{
			if(ret +2 < 100 )
				dcs_debug(bigbuff, ret +2 , "读到的报文太长(%d),大于能接收的长度%d丢弃",ret+2,nbufsize);
			else 
				dcs_debug(bigbuff, 100 , "读到的报文太长(%d),大于能接收的长度%d丢弃",ret+2,nbufsize);
			return -2;
		}
	}	
	else
	{
		ret = tcp_read_nbytes(conn_sockfd, bigbuff+2, msg_length);
		if( ret <= 0)
		{
			dcs_debug(bigbuff, 2 , "读到的报文不符合规则");
			return -2;
		}
		else if(ret < msg_length)
		{
			if(ret +2 < 100 )
				dcs_debug(bigbuff, ret +2 , "读到的报文不符合规则,长度=%d",ret+2);
			else 
				dcs_debug(bigbuff, 100 , "读到的报文不符合规则,读到的报文太长(%d)只显示部分报文",ret+2);
			return -2;
		}
		else//正确读取报文
		{
			memcpy(msg_buffer,bigbuff+2,msg_length);
			return msg_length;
		}
	}    
}

////--------------------------------------------------------------------------
int write_msg_to_shortTcp(int conn_sockfd, void *msg_buffer, int nbytes)
{
	/**trm 格式
	* 最大包长度为2044(不算包头),长度位和内容可以分多次byteArray发送
	* 报文前面2位为包长，双字节16进制压缩
	* 例如300字节长报文，长度填[0x01 0x2C]
	*/
	if(nbytes < 0)  nbytes = 0;
	char  buffer[nbytes+4];
	int   ret;

	//clear the error number
	errno = -1;
	memset(buffer,0,sizeof(buffer));
	//check for writablity within 'ntimeout' ms
	//if(tcp_check_writable(conn_sockfd,ntimeout) < 0)
	//timeout or other error
	//return -1;

	//first 4 bytes is length indictor
	
	buffer[0]=(unsigned char)(nbytes/256);
	buffer[1]=(unsigned char)(nbytes%256);
	memcpy(buffer+2,msg_buffer,nbytes);
	//write the message
	//    if(nbytes >= 0 && msg_buffer) //Freax TO DO: always true remove it
	{
		ret = tcp_write_nbytes(conn_sockfd, buffer, nbytes+2);
		if(ret < (nbytes+2)) //write error
			return -1;
	}

	return ret-2;
}
//--------------------------------------------------------------------------


int write_msg_to_shortTcp_bnk(int conn_sockfd, void *msg_buffer, int nbytes)
{
	/**
	* bnk
	* 报文前面4位为包长,最大包长度为2074,长度位和内容必须在一个byteArray里面发送
	* 如长度为185的包，包头就是 "0185".getBytes{}
	* 
	*/
	if(nbytes < 0)  nbytes = 0;
	char  buffer[nbytes+5];
	int   ret;
	memset(buffer,0,sizeof(buffer));
	//clear the error number
	errno = -1;

	//first 4 bytes is length indictor
	
	sprintf(buffer,"%.4d",nbytes);
	memcpy(buffer+4,msg_buffer,nbytes);
	//write the message
	//    if(nbytes >= 0 && msg_buffer) //Freax TO DO: always true remove it
	{
		ret = tcp_write_nbytes(conn_sockfd, buffer, nbytes+4);
		if(ret < (nbytes+4)) //write error
			return -1;
	}

	//return the number of bytes written to socket
	return ret-4;
}

//--------------------------------------------------------------------------
int write_msg_to_shortTcp_CON(int conn_sockfd, void *msg_buffer, int nbytes)
{    
	/**con
	 * 最大包长度为2044(不算包头),长度位和内容可以分多次byteArray发送
	 * 报文前面3位为包长，其中首位是固定包头'0x03',后面两字节为BCD码压缩的16进制长度.
	 * 如长度为2044的包，包头就是 [03 32 68] 
	 */
	if(nbytes < 0)  nbytes = 0;
	char  buffer[nbytes+4];
	char tmp[4];
	int   ret;

	//clear the error number
	errno = -1;
	memset(buffer,0,sizeof(buffer));
	memset(tmp, 0, sizeof(tmp));
	//first 3 bytes is length indictor
	
	buffer[0]=0x03;
	sprintf(tmp, "%04d",nbytes);
	asc_to_bcd(buffer+1, tmp, 4, 0);
	memcpy(buffer+3,msg_buffer,nbytes);
	ret = tcp_write_nbytes(conn_sockfd, buffer, nbytes+3);
	if(ret < (nbytes+3)) //write error
		return -1;

	return ret-3;
}


int write_msg_to_shortTcp_AMP(int conn_sockfd, void *msg_buffer, int nbytes)
{
    //
    //没有包长
    //
    
    int   ret;
    
    //clear the error number
    errno = -1;
    ret = tcp_write_nbytes(conn_sockfd, msg_buffer, nbytes);

    if(ret < nbytes) //write error
        return -1;
    //return the number of bytes written to socket
    return ret;
}


//---------------------------------------------------------------------------

int http_tcp_read_nbytes(unsigned int conn_sock, void *buffer, int nbytes)
{
	int    nleft,nread;
	char	*ptr=NULL;
	for(ptr=(char *)buffer,nleft=nbytes; nleft > 0;)
	{
		if ((nread = (int)read(conn_sock, ptr, nleft)) < 0)
		{
			if(errno == EAGAIN ||errno == EINTR)
			{
				nread = 0; // and call read() again
			}
			else
			{
				return(-1);
			}

		}
		else if (nread == 0)
		{
			break;  //EOF
		}

		nleft -= nread;
		ptr   += nread;
	} //for

	return(nbytes - nleft);  // return >= 0
}

/*
*  根据http协议接收数据POST方式
*/
int http_read_msg_post(int conn_sockfd, void *msg_buffer, int nbufsize,int ntimeout)
{
	int nHttpHeadLen = 0;			/*http包头长度*/
	int nContentLen = 0;			/*http内容长度*/
	char *pszHttpEnd = NULL;
	char *pszContentLenStart = NULL;
	char *pszContentLenEnd = NULL;
	char pszLength[10 + 1] = {0} ;
	int   ret = 0, bytes_to_read = 0, length = 0,flags = 0;
	char buf[PACKAGE_LENGTH+2048];
	errno = -1;

	//check for readability within 'ntimeout' ms
	if(tcp_check_readable(conn_sockfd,ntimeout) < 0)
	{
		//timeout or other error
		return -1;
	}
	//flags=fcntl(conn_sockfd,F_GETFL);
   	//fcntl(conn_sockfd,F_SETFL,flags|O_NDELAY);
	length = 1;
	memset(buf,0,sizeof(buf));
	while(1)
	{
		ret = http_tcp_read_nbytes(conn_sockfd,(char *)buf+bytes_to_read,length);
		if (ret == 0)
		{
			break;
		}
		else if( ret >0)
		{
			bytes_to_read+= ret;
			//HTTP头未读完
			if(nHttpHeadLen == 0)
			{
				//结束符
				pszHttpEnd = strstr(buf, "\x0D\x0A\x0D\x0A");
				if(pszHttpEnd == NULL)
				{		
					continue;
				}
				nHttpHeadLen = pszHttpEnd - buf + 4;
			}
			
			if(nContentLen == 0)
			{
				//从head里找长度标签
				pszContentLenStart = strstr(buf, "Content-Length:");
				if(pszContentLenStart == NULL)
				{
					pszContentLenStart = strstr(buf, "content-length:");
				}
				
				if(pszContentLenStart == NULL)
				{
					dcs_log(0, 0, "报文格式错buf=\n%s",buf);
					return -1;
				}
				//处理长度的结束符
				pszContentLenStart += strlen("Content-Length:");
				pszContentLenEnd = strstr(pszContentLenStart, "\x0D\x0A");
				if(pszContentLenEnd == NULL)
				{
					dcs_log(0, 0, "报文格式错buf=\n%s",buf);
					return -1;
				}
				//取得内容长度
				memcpy(pszLength, pszContentLenStart, pszContentLenEnd - pszContentLenStart);
				nContentLen = atoi(pszLength);
				if(nContentLen > nbufsize)
				{
					dcs_log(0, 0, "报文太长Content-Length=%d\nbuf=\n%s",nContentLen,buf);
					return -1;
				}
			}
			//检查是否读完
			if(bytes_to_read < nHttpHeadLen + nContentLen)
			{
				length = nHttpHeadLen + nContentLen - bytes_to_read;
				continue;
			}
			break;
		}				
		else
		{
			if( bytes_to_read <=0)
			    	return -1;
			break;
		}	
	}
	
	if(nHttpHeadLen ==0 )
	{
		dcs_log(0, 0, "报文格式错误len=%d",bytes_to_read);
		return -1;
	}
	dcs_log(0, 0, "报文len=%d,buf=\n%s",bytes_to_read,buf);
	memcpy(msg_buffer, buf+nHttpHeadLen,nContentLen);
	return nContentLen;
}


/*
*  根据http协议接收数据处理异步通知
*/
int http_read_msg(int conn_sockfd, void *msg_buffer, int nbufsize,int ntimeout)
{
	int nHttpHeadLen = 0;			/*http包头长度*/
	int nContentLen = 0;			/*http内容长度*/
	char *pszHttpEnd = NULL;
	char *pszContentStart = NULL;
	char *pszContentEnd = NULL;
	char *pszContentLenStart = NULL;
	char *pszContentLenEnd = NULL;
	int   ret = 0, bytes_to_read = 0, length = 0;
	char buf[PACKAGE_LENGTH+2048];
	char pszLength[10 + 1] = {0} ;

	//check for readability within 'ntimeout' ms
	if(tcp_check_readable(conn_sockfd, ntimeout) < 0)
	{
		//timeout or other error
		return -1;
	}
	//flags=fcntl(conn_sockfd,F_GETFL);
   	//fcntl(conn_sockfd,F_SETFL,flags|O_NDELAY);
	length = 1;
	memset(buf,0,sizeof(buf));
	while(1)
	{
		ret = http_tcp_read_nbytes(conn_sockfd,(char *)buf+bytes_to_read,length);
		if (ret == 0)
		{
			break;
		}
		else if( ret >0)
		{
			bytes_to_read+= ret;
			//HTTP头未读完
			if(nHttpHeadLen == 0)
			{
				//结束符
				pszHttpEnd = strstr(buf, "\x0D\x0A\x0D\x0A");
				if(pszHttpEnd == NULL)
				{		
					continue;
				}
				nHttpHeadLen = pszHttpEnd - buf + 4;
			}
			//从head里找长度标签
			pszContentLenStart = strstr(buf, "Content-Length:");
			if(pszContentLenStart != NULL)
			{
				//处理长度的结束符
				pszContentLenStart += strlen("Content-Length:");
				pszContentLenEnd = strstr(pszContentLenStart, "\x0D\x0A");
				if(pszContentLenEnd == NULL)
				{
					dcs_log(0, 0, "报文格式错buf=\n%s",buf);
					return -1;
				}
				//取得内容长度
				memcpy(pszLength, pszContentLenStart, pszContentLenEnd - pszContentLenStart);
				nContentLen = atoi(pszLength);
				if(nContentLen > nbufsize)
				{
					dcs_log(0, 0, "报文太长Content-Length=%d\nbuf=\n%s",nContentLen,buf);
					return -1;
				}
			
				//检查是否读完
				if(bytes_to_read < nHttpHeadLen + nContentLen)
				{
					length = nHttpHeadLen + nContentLen - bytes_to_read;
					continue;
				}
				dcs_log(0, 0, "报文len=%d,buf=\n%s",bytes_to_read,buf);
				memcpy(msg_buffer, buf+nHttpHeadLen,nContentLen);
				break;
			}
			else
			{
				dcs_log(0, 0, "报文len=%d, buf=\n%s", bytes_to_read, buf);
				pszContentStart = strstr(buf, "?");
				pszContentEnd = strstr(pszContentStart, " HTTP");
				nContentLen = pszContentEnd - pszContentStart-1;
				memcpy(msg_buffer, pszContentStart+1, nContentLen);
				dcs_log(0, 0, "报文nContentLen=%d, msg_buffer=\n%s", nContentLen, msg_buffer);
			}
			break;
		}				
		else
		{
			if( bytes_to_read <=0)
			    return -1;
			break;
		}	
	}
	if(nHttpHeadLen ==0 )
	{
		dcs_log(0, 0, "报文格式错误len=%d", bytes_to_read);
		return -1;
	}
	return nContentLen;
}


