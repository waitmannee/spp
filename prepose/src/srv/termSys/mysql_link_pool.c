/*****************************************************
*  mysql_link_pool.c
*  iPrepose,实现mysql 连接池的相关功能
*
*  Created by wuzf on 2015-08-03.
*  Copyright (c) 2015年 Chinaebi. All rights reserved.
********************************************************/
#include "extern_function.h"
#include "pub_function.h"
#ifdef __macos__
    #include </usr/local/mysql/include/mysql.h>
#elif defined(__linux__)
    #include </usr/include/mysql/mysql.h>
#elif defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
#else
    #include </usr/local/mysql/include/mysql.h>
#endif

typedef struct ConnectionPool_S
{
	int max_link_num;//最大连接数
	pthread_mutex_t lock;//线程锁
	MYSQL *sock;//存放mysql 数据库链接信息
	int  *linkstate;//存放连接状态
}ConnPoool;
ConnPoool *conn_pool = NULL ;//连接池

/**
 * 连接mysql数据库
 * 返回值：NULL 表示连接失败
 */
MYSQL * MysqlConect()
{
	MYSQL mysql, *sock;
	mysql_init(&mysql);
	char value = 1;
	my_bool secure_auth =0;
	mysql_options(&mysql, MYSQL_OPT_RECONNECT, (char *)&value);
	mysql_options(&mysql, MYSQL_SECURE_AUTH, (my_bool *)&secure_auth);
	if(!(sock = mysql_real_connect(&mysql, (char *)getenv("SERVER"), (char *)getenv("DBUSR"), (char *)getenv("DBPWD"), (char *)getenv("DBNAME"), 0, NULL, 0))) {
		dcs_log(0, 0, "host%s,user:%s,ps:%s\n\n", (char *)getenv("SERVER"),(char *)getenv("DBUSR"),(char *)getenv("DBPWD"));
		dcs_log(0, 0, "Couldn't connect to DB!\n%s\n\n", mysql_error(&mysql));
		return NULL ;
	}

	if(mysql_set_character_set(&mysql, "utf8")) {
		fprintf (stderr, "错误, %s\n", mysql_error(&mysql));
		dcs_log(0, 0, "错误, %s\n", mysql_error(&mysql));
		return NULL;
	}

	if(sock){
		dcs_log(0, 0, "数据库连接成功,name:%s", (char *)getenv("DBNAME"));
		return sock;
	}else{
		dcs_log(0, 0, "数据库连接失败,name:%s", (char *)getenv("DBNAME"));
		return NULL;
	}
	return sock;
}

/**
 * 初始化mysql 连接池
 * 输入：max_link_num 最大连接数
 * 输出：-1 表示创建失败
 * 		 0  表示创建成功
 */
int mysql_pool_init (int max_link_num)
{
	MYSQL  *sock;
	conn_pool = (ConnPoool *) malloc(sizeof(ConnPoool));
	conn_pool->max_link_num = max_link_num;
	pthread_mutex_init (&(conn_pool->lock), NULL);

	conn_pool->sock =(MYSQL *)malloc(max_link_num * sizeof(MYSQL));
	conn_pool->linkstate =(int *)malloc(max_link_num * sizeof(int));
	int i = 0;
	for(i = 0; i< max_link_num ; i++)
	{
		sock = MysqlConect();
		if(sock == NULL)
			return -1;
		memcpy(&(conn_pool->sock[i]), sock, sizeof(MYSQL));
		conn_pool->linkstate[i] = FREE_STAT;
	}
	return 0;
}


/**
 * 从连接池获取一个空闲的连接
 * 返回NULL表示没有可用空闲连接
 */
MYSQL * GetFreeLink()
{
	int i=0;
	pthread_mutex_lock(&(conn_pool->lock));
	for(i=0; i< conn_pool->max_link_num; i++)
	{
#ifdef __TEST__
		dcs_log(0,0,"111GetFreeLink___linkstate=%d",conn_pool->linkstate[i]);
#endif
		if(conn_pool->linkstate[i] == FREE_STAT)
		{
#ifdef __TEST__
			dcs_log(0,0,"2222GetFreeLink___linkstate=%d",conn_pool->linkstate[i]);
#endif
			conn_pool->linkstate[i] = BUSINESS_STAT;
			pthread_mutex_unlock(&(conn_pool->lock));
			return &(conn_pool->sock[i]);
		}
	}
#ifdef __TEST__
	dcs_log(0,0,"333");
#endif
	pthread_mutex_unlock(&(conn_pool->lock));
	return NULL;
}
/**
 * 将连接置为空闲状态，可供其他人员使用
 */
void SetFreeLink(MYSQL * sock)
{
	int i = 0;
	pthread_mutex_lock(&(conn_pool->lock));
	for(i = 0; i< conn_pool->max_link_num; i++)
	{
#ifdef __TEST__
		dcs_log(0,0,"111SetFreeLink___sock=%d",i);
#endif
		if(memcmp(&conn_pool->sock[i], sock, sizeof(MYSQL)) == 0)
		{
#ifdef __TEST__
			dcs_log(0,0,"222SetFreeLink___sock=%d",i);
#endif
			conn_pool->linkstate[i] = FREE_STAT;
			break;
		}
	}
	pthread_mutex_unlock(&(conn_pool->lock));
}


/*销毁连接池
 * */
int DestroyLinkPool ()
{
	int i = 0;

    free(conn_pool->linkstate);
    pthread_mutex_destroy(&(conn_pool->lock));

    for(i=0; i< conn_pool->max_link_num; i++)
    	mysql_close(&conn_pool->sock[i]);

    free(conn_pool->sock);
    free(conn_pool);
    conn_pool=NULL;
    return 0;
}


/*mysql断开重连的处理*/
int ReConnection(MYSQL *sock)
{
	int i =0;
	for(i = 0; i<7; i++)
	{
		#ifdef __TEST__
		if(i >=1)
			dcs_log(0, 0, "%d ci GetMysqlLink", i);
		#endif
		if(mysql_ping(sock)!=0)
			continue;
		else
			break;
	}
	return mysql_ping(sock);
}

/**
 * 从连接池获取一个空闲的连接
 * 返回NULL表示没有可用空闲连接
 */
MYSQL * GetFreeMysqlLink()
{
	int i = 0;
	MYSQL *sock = NULL;
	//全局变量为NULL，从连接池中获取
	for(i = 0; i<10; i++)
	{
		sock = GetFreeLink();
		if(sock != NULL)
		{
			ReConnection(sock);
			return sock;
		}
		dcs_log(0, 0, "当前无空闲连接，等待0.1m");
		u_sleep_us(100000);
	}
	return sock;
}


