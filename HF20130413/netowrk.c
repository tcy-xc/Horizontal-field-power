/*
文件：		network.h
内容：		网络接口对象的实现函数
注意：		编译时需要socket库
更新记录：
-- Ver 1.0 / 2011-07-22
*/

#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include "network.h"

/*
名称：		NetInit
功能：		初始化一个网络接口对象
返回值：	Socket文件描述符，或NET_ER错误
*/
int NetInit(net_int_t *pstObj)
{
	//清空对象
	memset(pstObj, 0, sizeof(net_int_t));
	
	//以UDP协议建立一个Socket
	pstObj->p_iSockfb = socket(AF_INET, SOCK_DGRAM, 0);
	
	return (pstObj->p_iSockfb == -1) ? NET_ER : pstObj->p_iSockfb;
}

/*
名称：		NetSetLocal
功能：		绑定本机端口，默认监听全部本机IP
返回值：	绑定的端口号，或NET_ER错误
*/
int NetSetLocal(net_int_t *pstObj, unsigned dwPort)
{
	int s;
	
	pstObj->p_stLocal.sin_family = AF_INET;
	pstObj->p_stLocal.sin_addr.s_addr = INADDR_ANY;
	pstObj->p_stLocal.sin_port = htons(dwPort);
	s = bind(pstObj->p_iSockfb, 
				(struct sockaddr *)&pstObj->p_stLocal, 
				sizeof(struct sockaddr_in));
	
	return (s == -1) ? NET_ER : dwPort;
}

/*
名称：		NetSetRemote
功能：		设置远程主机IP及端口号
返回值：	设置的端口号
*/
int NetSetRemote(net_int_t *pstObj, char *szIp, unsigned dwPort)
{
	pstObj->p_stRemote.sin_family = AF_INET;
	pstObj->p_stRemote.sin_addr.s_addr = inet_addr(szIp);
	pstObj->p_stRemote.sin_port = htons(dwPort);
	
	return dwPort;
}

/*
名称：		NetClose
功能：		关闭一个网络连接，再次使用前要重新初始化
返回值：	NET_OK, NET_ER
*/
int NetClose(net_int_t *pstObj)
{
	return (close(pstObj->p_iSockfb) == -1) ? NET_ER : NET_OK;
}

/*
名称：		NetSend
功能：		以无连接的UDP协议发送一个消息给远程主机。
			消息的长度由dwSize决定。
注意：		如果远程主机不在网络中，则该函数可能持续阻塞主调线程。
返回值：	发送消息的字节数，或NET_ER错误
*/
int NetSend(const net_int_t *pstObj, const void *pvMsg, size_t dwSize)
{
	ssize_t c;

	c = sendto(pstObj->p_iSockfb, pvMsg, dwSize, 0, 
					(struct sockaddr *)&pstObj->p_stRemote,
					sizeof(struct sockaddr_in));
	
	return (c == -1) ? NET_ER : c;
}

/*
名称：		NetRecv
功能：		从远程主机接收消息，以dwSize标识最大缓冲字节数。
注意：		该函数将阻塞主调线程，直到接收到消息。
返回值：	接收消息的字节数，或NET_ER错误
*/
int NetRecv(const net_int_t *pstObj, void *pvMsg, size_t dwSize)
{
	ssize_t c;
	
	c = recvfrom(pstObj->p_iSockfb, pvMsg, dwSize, 0, NULL, NULL);
	
	return (c == -1) ? NET_ER : c;
}

