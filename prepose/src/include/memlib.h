#ifndef __MEMLIB_H__
#define __MEMLIB_H__
#include <libmemcached/memcached.h>

typedef struct {
	char keys[250];
	size_t keys_length;
	char value[1024];
	size_t value_length;
}KEY_VALUE_INFO;


/*
 函数名称：Mem_SavaKey
 函数功能：实现向缓存服务器memcached存储数据
 输入参数：key->存到memcached的key
 value->key所对应的value值
 value_len->value的长度
 expiration_time 过期时间。取值小于0或者取值为0表示永远；取值大于0，最大2592000秒，memcache支持最大30天。
 返回值：0->保存数据成功
 -1->保存数据失败
 */
int Mem_SavaKey(char *key, char *value, int value_len, int expiration_time);

/*
函数名称：Mem_GetKey
函数功能：实现根据key向缓存服务器memcached取数据
输入参数：key->想要取value值的key
		  buffer_len:用来存放value值的buffer长度
输出参数：value->取到的value值
		  value_len->取到的value值的长度
返回值：0->取数据成功
		-1->取数据失败
*/
int Mem_GetKey(char *key, char *value, int *value_len, int buffer_len);

/*
函数名称：Mem_DeleteKey
函数功能：实现根据key向缓存服务器memcached删除数据
输入参数：key->想要删除的key

返回值：0->删除key处理成功
		-1->删除key处理失败
*/
int Mem_DeleteKey(char *key);

#endif
