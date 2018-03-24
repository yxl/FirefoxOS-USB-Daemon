// SocketService.h: interface for the SocketService class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "SocketComm.h"

class SocketServiceCallback
{
public:
	virtual void OnConnect() = 0;
	virtual void OnDisconnect() = 0;
	virtual void OnStringReceived(const char* utf8String) = 0;
};

class SocketService  
{
public:
	SocketService(SocketServiceCallback* pCallback)
		: m_bStarted(false)
		, m_bConnected(false)
		, m_pCallback(pCallback)
	{
	}

	virtual ~SocketService() 
	{
	}

	void Start();
	void Stop();

	bool StartNewServer();

	// Send message to all clients.
	void SendString(const char* utf8String);

	// Number of clients connected
	int GetClientCount() const;
private:
	class CSocketManager;

	void OnStringReceived(const char* utf8String);
	void OnEvent(UINT uEvent, CSocketManager* pManager);
private:
	class CSocketManager: public CSocketComm 
	{
	public:
		CSocketManager() : m_pParent(NULL) {}
		virtual ~CSocketManager() {}

		void SetParent(SocketService* pParent) { m_pParent = pParent; }

		void OnDataReceived(const LPBYTE lpBuffer, DWORD dwCount) override;
		virtual void OnEvent(UINT uEvent, LPVOID lpvData) override;
	private:
		SocketService* m_pParent;
	};

	bool m_bStarted;
	bool m_bConnected;
	SocketServiceCallback* m_pCallback;
	CSocketManager m_SocketManager;
	CCriticalSection m_csSendString;
	CCriticalSection m_csSocket;
};