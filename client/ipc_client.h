#include "windows.h"
#include <string>
#include <iostream>

#define MS_DATA_BUF     1024
#define PIPENAME       _T("\\\\.\\pipe\\MSIPC_Pipe")
#define LOG std::wcout


// Pipe communication events
#define MS_ERROR        100
#define MS_READ         101
#define MS_WRITE        102
#define MS_CLOSE        103
#define MS_INIT         104
#define MS_CLIENT_CONN  106
#define MS_THREAD_RUN   107
#define MS_CLIENT_TRY   109
#define MS_TERMINATE    110
#define MS_IOREAD       120
#define MS_IOWRITE      121
#define MS_IOPENDING    122
#define MS_IOWRITECLOSE 123

class CipcClient
{
public:

	CipcClient(std::wstring& sName);
	virtual ~CipcClient(void);

	int GetEvent() const;
	void SetEvent(int nEventID);
	HANDLE GetThreadHandle();
	HANDLE GetPipeHandle();
	void SetData(std::wstring& sData);
	void GetData(std::wstring& sData);
	void ConnectToServer();
	void OnEvent(int nEvent);
	static UINT32 __stdcall PipeThreadProc(void* param);
	void Close();
	bool Read();
	bool Write();

private:

	CipcClient(void);
	void Init();
	void Run();

	const std::wstring mPipeName;
	HANDLE mPipe;
	HANDLE mThread;
	int    mEvent;
	wchar_t* mbuffer;

};

