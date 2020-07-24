/*
文件名:		network.h
内容:		一套使用UDP协议的网络通讯接口
更新记录:
-- Ver 1.0 / 2011-07-02
*/

#ifndef NETWORK_H_
#define NETWORK_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

//错误代码
#define NET_OK 0
#define NET_ER -1

//网络接口对象的数据类型
typedef struct NetworkInterfaceTag
{
	int p_iSockfb;					//Socket file描述符
	struct sockaddr_in p_stLocal;	//本机地址
	struct sockaddr_in p_stRemote;	//远城地址
} net_int_t;

//接口函数
int NetInit(net_int_t *pstObj);	
int NetSetLocal(net_int_t *pstObj, unsigned dwPort);
int NetSetRemote(net_int_t *pstObj, char *szIp, unsigned dwPort);
int NetClose(net_int_t *pstObj);
int NetSend(const net_int_t *pstObj, const void *pvMsg, size_t dwSize);
int NetRecv(const net_int_t *pstObj, void *pvMsg, size_t dwSize);

/*
说明：
NetInit			初始化网络接口对象
NetSetLocal		绑定本机端口，默认监听全部IP
NetSetRemote	绑定远程主机IP及端口
NetClose		关闭网络接口对象，需要重新初始化
NetSend			发送消息
NetRecv			接收消息
*/
#endif
