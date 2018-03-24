// SocketManager.cpp: implementation of the SocketService class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <atlconv.h>
#include "SocketService.h"
#include "MainFrame.h"
#include "App.h"

#define WSA_VERSION  MAKEWORD(2,2)

void SocketService::Start()
{
	if (m_bStarted)
	{
		return;
	}

	WSADATA WSAData = {0};
	if (0 != WSAStartup(WSA_VERSION, &WSAData))
	{
		// Tell the user that we could not find a usable
		// WinSock DLL.
		if (LOBYTE(WSAData.wVersion) != LOBYTE(WSA_VERSION) ||
			HIBYTE(WSAData.wVersion) != HIBYTE(WSA_VERSION))
		{
			::MessageBox(NULL, _T("Incorrect version of WS2_32.dll found"), _T("Error"), MB_OK);
		}

		WSACleanup( );
		return;
	}

	m_SocketManager.SetParent(this);

	StartNewServer();

	m_bStarted = true;
}

void SocketService::Stop()
{
	if (!m_bStarted)
	{
		return;
	}

	// Disconnect
	if (m_SocketManager.IsOpen())
	{
		m_SocketManager.StopComm();
	}

	// Terminate use of the WS2_32.DLL
	WSACleanup();
}

bool SocketService::StartNewServer()
{
	m_csSocket.Enter();
	if (m_bConnected)
	{
		m_csSocket.Leave();
		return true;
	}
	// connect to TCP socket server
	if (!m_SocketManager.ConnectTo(_T("127.0.0.1"), _T("9500"), AF_INET, SOCK_STREAM))
	{
		// No availalbe port found.
		TRACE(_T("Failed to conect to 127.0.0.1:9500.\n"));
		m_csSocket.Leave();
		return false;
	}
	if (!m_SocketManager.WatchComm())
	{
		TRACE(_T("Failed to connect to server.\n"));
		m_SocketManager.CloseComm();
		m_csSocket.Leave();
		return false;
	}
	TRACE(_T("Connected. Port=9500\n"));
	MainFrame::GetInstance()->ExecuteOnUIThread([this]()
	{
		m_pCallback->OnConnect();
	});
	m_csSocket.Leave();
	return true;
}

void SocketService::SendString(const char* utf8String)
{
	CStringA str = utf8String;
	if (str.GetLength() <= 0)
	{
		return;
	}

	str.Replace("\n", "\r\n");

	stMessageProxy msgProxy;
	int nLen = str.GetLength();
	nLen = min(sizeof(msgProxy.byData) - 1, nLen);
	memcpy(msgProxy.byData, (LPCSTR)str, nLen + 1);

	m_csSendString.Enter();
	// Send to all clients
	if (m_SocketManager.IsOpen())
	{
		m_SocketManager.WriteComm(msgProxy.byData, nLen, INFINITE);
	}
	m_csSendString.Leave();
}

int SocketService::GetClientCount() const
{
	return m_bConnected ? 1 : 0;
}

void SocketService::OnStringReceived(const char* utf8String)
{
	m_pCallback->OnStringReceived(utf8String);
}

void SocketService::OnEvent(UINT uEvent, CSocketManager* pManager)
{
	switch(uEvent)
	{
	case EVT_CONFAILURE: // Fall through
	case EVT_CONDROP:
		TRACE(_T("Connection failed or abandoned\n"));
		m_csSocket.Enter();
		pManager->StopComm();
		m_bConnected = false;
		m_csSocket.Leave();
		StartNewServer();
		MainFrame::GetInstance()->ExecuteOnUIThread([this]()
		{
			m_pCallback->OnDisconnect();
		});
		break;
	case EVT_ZEROLENGTH:
		TRACE( _T("Zero Length Message\n") );
		break;
	default:
		TRACE(_T("Unknown Socket event\n"));
		break;
	}
}

// 
// SocketService::CSocketManager
//

void SocketService::CSocketManager::OnDataReceived(const LPBYTE lpBuffer, DWORD dwCount)
{
	if (dwCount <= 0)
	{
		return;
	}

	char* utf8String = new char[dwCount + 1];
	memcpy(utf8String, lpBuffer, dwCount);
	utf8String[dwCount] = '\0';
	CStringA received = utf8String;
	delete[] utf8String;
	if (received == "hello\n") {
		m_pParent->m_csSocket.Enter();
		m_pParent->m_bConnected = true;
		m_pParent->m_csSocket.Leave();
	}
	MainFrame::GetInstance()->ExecuteOnUIThread([received, this]()
	{
		m_pParent->OnStringReceived(received);
	});
}

void SocketService::CSocketManager::OnEvent(UINT uEvent, LPVOID lpvData)
{
	MainFrame::GetInstance()->ExecuteOnUIThread([this, uEvent]()
	{
		m_pParent->OnEvent(uEvent, this);
	});
}