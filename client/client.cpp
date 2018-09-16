#include "tchar.h"
#include <io.h>
#include <fcntl.h>
#include "ipc_client.h"

int _tmain(int argc, _TCHAR* argv[])
{
	std::wcout << _T("-------------------------IPC CLIENT---------------------------") << std::endl;
	std::wstring sPipeName(PIPENAME);
	CipcClient* pClient = new CipcClient(sPipeName);
	::WaitForSingleObject(pClient->GetThreadHandle(), INFINITE);
	delete pClient;
	pClient = NULL;
	return 0;
}

