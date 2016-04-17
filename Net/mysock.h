#ifndef __MySock_h__
#define __MySock_h__

#ifdef WIN32
#include <WinSock2.h>
#include <Windows.h>
#include <mswsock.h>
#endif

#include "fxmeta.h"
#include "eventqueue.h"
#include "ifnet.h"
#include "loopbuff.h"
#include "lock.h"
//#include "packetparser.h"

enum ESocketState
{
	SSTATE_INVALID = 0,	// 无效//
	//SSTATE_START_LISTEN, // 监听开始请求状态//
	SSTATE_LISTEN,		// 监听状态//
	SSTATE_STOP_LISTEN,	// 监听停止请求状态//
	//SSTATE_ACCEPT,
	SSTATE_CONNECT,		// 链接请求状态//
	SSTATE_ESTABLISH,	// 链接建立状态//
	//SSTATE_DATA,		 // 数据发送状态//
	SSTATE_CLOSE,		// 关闭请求状态//
	//SSTATE_OK,
	SSTATE_RELEASE,
};

enum ENetEvtType
{
	NETEVT_INVALID = 0,
	NETEVT_ESTABLISH,
	NETEVT_ASSOCIATE,
	NETEVT_RECV,
	NETEVT_CONN_ERR,
	NETEVT_ERROR,
	NETEVT_TERMINATE,
	NETEVT_RELEASE,
};

#ifdef WIN32
enum EIocpOperation
{
	IOCP_RECV = 1,
	IOCP_SEND,
	IOCP_ACCEPT,
	IOCP_CONNECT,
};

struct SPerIoData
{
	OVERLAPPED		stOverlapped;
	SOCKET			hSock;
	EIocpOperation	nOp;
	WSABUF			stWsaBuf;
	char			Buf[128];
};
#endif // WIN32

struct SNetEvent
{
	ENetEvtType		eType;
	UINT32			dwValue;
};

class FxIoThread;

class FxListenSock : public IFxListenSocket
{
public:
	FxListenSock();
	virtual ~FxListenSock();

	virtual bool Init();
	virtual void OnRead();
	virtual void OnWrite();
	virtual bool Listen(UINT32 dwIP, UINT16 wPort);
	virtual bool StopListen();
	virtual bool Close();
	void Reset();

	bool PushNetEvent(ENetEvtType eType, UINT32 dwValue);

	void SetState(ESocketState eState){ m_nState = eState; }
	ESocketState GetState(){ return m_nState; }

	virtual void ProcEvent();

#ifdef WIN32
	virtual void OnParserIoEvent(bool bRet, SPerIoData* pIoData, UINT32 dwByteTransferred);		//
#else
	virtual void OnParserIoEvent(int dwEvents);		// 处理完成端口事件//
#endif // WIN32

public:

private:
	bool AddEvent();

	void	__ProcAssociate();
	void	__ProcError(UINT32 dwErrorNo);
	void	__ProcTerminate();

private:
	ESocketState		m_nState;

	FxCriticalLock			m_oLock;

	TEventQueue<SNetEvent>	m_oEvtQueue;

	FxIoThread* m_poEpollHandler;

#ifdef WIN32
	bool PostAccept(SPerIoData& oSPerIoData);
	bool InitAcceptEx();
	void OnAccept(SPerIoData* pstPerIoData);

	SPerIoData m_oSPerIoDatas[128];
	LPFN_ACCEPTEX       m_lpfnAcceptEx;
	LPFN_GETACCEPTEXSOCKADDRS   m_lpfnGetAcceptExSockaddrs;
#else
	void OnAccept();
#endif // WIN32
};

class FxConnection;
class FxConnectSock : public IFxConnectSocket
{
public:
	FxConnectSock();
	virtual ~FxConnectSock();

	virtual bool Init();
	virtual void OnRead();
	virtual void OnWrite();

	void Reset();

	virtual bool Close();
	
	virtual bool Send(const char* pData, int dwLen);

	bool PushNetEvent(ENetEvtType eType, UINT32 dwValue);

	void SetConnection(FxConnection* poConnection){ m_poConnection = poConnection; }
	FxConnection* GetConnection(){ return m_poConnection; }

	bool IsConnect(){ return m_nState == SSTATE_ESTABLISH; }
	void SetState(ESocketState eState){ m_nState = eState; }
	ESocketState GetState(){ return m_nState; }

	IFxDataHeader* GetDataHeader();

	void SetIoThread(FxIoThread* pIoThread){ m_poEpollHandler = pIoThread;}// LogScreen("thread id %d, socket id : %d", m_poEpollHandler, GetSockId()); }
	bool AddEvent();

	virtual void ProcEvent();

	SOCKET Connect();

#ifdef WIN32
	bool PostRecv();
	bool PostClose();

	virtual void OnParserIoEvent(bool bRet, SPerIoData* pIoData, UINT32 dwByteTransferred);		//
#else
	virtual void OnParserIoEvent(int dwEvents);		// 处理完成端口事件//
#endif // WIN32

private:

	bool PostSend();
	bool PostSendFree();
	bool SendImmediately();						// 将要缓冲区中数据全部发送//

	void	__ProcEstablish();
	void	__ProcAssociate();
	void	__ProcConnectError(UINT32 dwErrorNo);
	void	__ProcError(UINT32 dwErrorNo);
	void	__ProcTerminate();
	void	__ProcRecv(UINT32 dwLen);
	void	__ProcRelease();
private:
	void OnConnect();
#ifdef WIN32
	void OnRecv(bool bRet, int dwBytes);
	void OnSend(bool bRet, int dwBytes);
#else
	void OnRecv();
	void OnSend();
#endif // WIN32

private:
	ESocketState		m_nState;

	TEventQueue<SNetEvent>	m_oEvtQueue;
	bool					m_bSendLinger;		// 发送延迟，直到成功，或者30次后//

	FxCriticalLock			m_oLock;

	FxLoopBuff*         m_poSendBuf;
	FxLoopBuff*         m_poRecvBuf;
	FxConnection*		m_poConnection;

	FxIoThread* m_poEpollHandler;

	int m_nNeedData;        // 未处理完的逻辑数据包还需要的长度//
	int m_nPacketLen;       // 未处理完的逻辑数据包长度//

private:
#ifdef WIN32
	SPerIoData			m_stRecvIoData;
	SPerIoData			m_stSendIoData;
	LONG                m_nPostRecv;        // 未决的WSARecv操作数//
	LONG                m_nPostSend;        // 未决的WSASend操作数//

	UINT32              m_dwLastError;      // 最后的出错信息//
#else
	bool				m_bSending;
#endif // WIN32

};


#endif // !__MySock_h__
