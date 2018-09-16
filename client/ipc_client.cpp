#include "windows.h"
#include "tchar.h"
#include "process.h"
#include "ipc_client.h"


CipcClient::CipcClient(void)
{
}

CipcClient::CipcClient(std::wstring& sName) : mPipeName(sName), mThread(NULL), mEvent(MS_INIT)
{
	mbuffer = (wchar_t*)calloc(MS_DATA_BUF, sizeof(wchar_t));
	Init();
}

CipcClient::~CipcClient(void)
{
	delete mbuffer;
	mbuffer = NULL;
}

int CipcClient::GetEvent() const
{
	return mEvent;
}

void CipcClient::SetEvent(int nEventID)
{
	mEvent = nEventID;
}

HANDLE CipcClient::GetThreadHandle()
{
	return mThread;
}

HANDLE CipcClient::GetPipeHandle()
{
	return mPipe;
}

void CipcClient::SetData(std::wstring& sData)
{
	memset(&mbuffer[0], 0, MS_DATA_BUF);
	wcsncpy(&mbuffer[0], sData.c_str(), __min(MS_DATA_BUF, sData.size()));
}

void CipcClient::GetData(std::wstring& sData)
{
	sData.clear();
	sData.append(mbuffer);
}

void CipcClient::Init()
{
	if (mPipeName.empty())
		return;

	Run();
}

void CipcClient::Run()
{
	UINT threadID = 0;
	mThread = (HANDLE)::_beginthreadex(NULL, NULL, PipeThreadProc, this, CREATE_SUSPENDED, &threadID);

	if (NULL == mThread)
		OnEvent(MS_ERROR);
	else
	{
		SetEvent(MS_INIT);
		::ResumeThread(mThread);
	}
}

UINT32 __stdcall CipcClient::PipeThreadProc(void* pParam)
{
	CipcClient* pPipe = reinterpret_cast<CipcClient*>(pParam);
	if (pPipe == NULL)
		return 1L;

	pPipe->OnEvent(MS_THREAD_RUN);
	while (true)
	{
		int eventID = pPipe->GetEvent();
		if (eventID == MS_ERROR || eventID == MS_TERMINATE)
		{
			pPipe->Close();
			break;
		}

		switch (eventID)
		{
		case MS_INIT:
		{
			pPipe->ConnectToServer();
			break;
		}

		case MS_IOREAD:
		{
			if (pPipe->Read())
				pPipe->OnEvent(MS_READ);
			else
				pPipe->OnEvent(MS_ERROR);

			break;
		}

		case MS_IOWRITE:
		{
			if (pPipe->Write())
				pPipe->OnEvent(MS_WRITE);
			else
				pPipe->OnEvent(MS_ERROR);
		}
		break;

		case MS_CLOSE:
		{
			pPipe->OnEvent(MS_CLOSE);
			break;
		}

		case MS_IOWRITECLOSE:
		{
			if (pPipe->Write())
				pPipe->OnEvent(MS_CLOSE);
			else
				pPipe->OnEvent(MS_ERROR);

			break;
		}

		case MS_IOPENDING:
		default:
			Sleep(10);
			continue;
		};

		Sleep(10);
	};

	return 0;
}

void CipcClient::ConnectToServer()
{
	OnEvent(MS_CLIENT_TRY);
	mPipe = ::CreateFile(mPipeName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

	if (INVALID_HANDLE_VALUE == mPipe)
		OnEvent(MS_ERROR);
	else
		OnEvent(MS_CLIENT_CONN);
}

void CipcClient::OnEvent(int nEventID)
{
	switch (nEventID)
	{
	case MS_THREAD_RUN:
		LOG << "<<<<   Thread is running" << std::endl;
		break;

	case MS_INIT:
		LOG << "<<<<   Initializing IPC communication" << std::endl;
		break;

	case MS_CLIENT_TRY:
		LOG << "<<<<   Trying to connect to IPC server" << std::endl;
		break;

	case MS_CLIENT_CONN:
	{
		LOG << "<<<<   Connected to IPC server" << std::endl;
		SetEvent(MS_IOREAD);
		break;
	}

	case MS_READ:
	{
		std::wstring sData;
		GetData(sData);
		LOG << "Message from server: " << sData <<  std::endl;
		LOG << "<<<<   If message recieved close Application will be closed" << std::endl;
		
		if (wcscmp(sData.c_str(), _T("close")) == 0)
			SetEvent(MS_CLOSE);
		else
		{
			sData.clear();
			LOG << "<<<<   Enter Data to be communciated to server" << std::endl;
			std::getline(std::wcin, sData);
			SetData(sData);
			if (wcscmp(sData.c_str(), _T("close")) == 0)
				SetEvent(MS_IOWRITECLOSE);
			else
				SetEvent(MS_IOWRITE);
		}
			
		break;
	}

	case MS_WRITE:
		LOG << "<<<<   Wrote data to pipe" << std::endl;
		SetEvent(MS_IOREAD);
		break;

	case MS_ERROR:
		LOG << "<<<<   ERROR: Pipe error" << std::endl;
		SetEvent(MS_ERROR);
		break;

	case MS_CLOSE:
		LOG << "<<<<   Closing pipe" << std::endl;
		SetEvent(MS_TERMINATE);
		break;
	};
}

void CipcClient::Close()
{
	::CloseHandle(mPipe);
	mPipe = NULL;
}

bool CipcClient::Read()
{
	DWORD bytes = 0;
	BOOL status = FALSE;
	int read_bytes = 0;
	do
	{
		status = ::ReadFile(mPipe, &mbuffer[read_bytes], MS_DATA_BUF, &bytes, NULL);

		if (!status && ERROR_MORE_DATA != GetLastError())
		{
			status = FALSE;
			break;
		}
		read_bytes += bytes;

	} while (!status);

	if (FALSE == status || 0 == bytes)
		return false;

	return true;
}

bool CipcClient::Write()
{
	DWORD bytes;
	BOOL status = ::WriteFile(mPipe, mbuffer, ::wcslen(mbuffer)*sizeof(wchar_t) + 1, &bytes, NULL);

	if (FALSE == status || wcslen(mbuffer)*sizeof(wchar_t) + 1 != bytes)
		return false;

	return true;
}

