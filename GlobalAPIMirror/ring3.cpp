#include<Windows.h>
#include<stdio.h>
#include <winioctl.h>
#include<winternl.h>
#define SYMBOLICLINK_NAME "\\\\.\\HbgDevLnk"
#define OPER1 CTL_CODE(FILE_DEVICE_UNKNOWN,0x800,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define OPER2 CTL_CODE(FILE_DEVICE_UNKNOWN,0x900,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IN_BUFFER_MAXLENGTH 4
#define OUT_BUFFER_MAXLENGTH 4
//Load .sys(Driver)

VOID LoadDriver(WCHAR* DriverName,WCHAR* DriverPath)
{
	//Create SC Manager
	WCHAR szDriverFullPath[MAX_PATH]={0};
	GetFullPathNameW(DriverPath,MAX_PATH,szDriverFullPath,NULL);
	DWORD dwErr=0;
	SC_HANDLE sh=OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
	if(sh==NULL)
	{
		printf("OpenSCManager Error!\n");
		return;
	}
	//Create Service
	SC_HANDLE m_hServiceDDK=CreateServiceW(
		sh,\
		DriverName,\
		DriverName,\
		SERVICE_ALL_ACCESS,\
		SERVICE_KERNEL_DRIVER,\
		SERVICE_DEMAND_START,\
		SERVICE_ERROR_IGNORE,\
		szDriverFullPath,\
		NULL,\
		NULL,\
		NULL,\
		NULL,\
		NULL);
	if(m_hServiceDDK==NULL)
	{
		printf("CreateService Error!\n");
		return;
	}

	//OpenService
	m_hServiceDDK=OpenServiceW(sh,DriverName,SERVICE_ALL_ACCESS);
	dwErr = GetLastError();
	if(m_hServiceDDK)
	{
		printf("Set Driver success\n");
	}
	if(!StartService(m_hServiceDDK,NULL,NULL))
	{
		dwErr = GetLastError();
		if (dwErr != ERROR_SERVICE_ALREADY_RUNNING)
		{
			printf("Driver run error!, %d\n", dwErr);
			return;
		}
	}
	printf("Driver run success!\n");
	if (sh)
	{
		CloseServiceHandle(sh);
	}
	if (m_hServiceDDK)
	{
		CloseServiceHandle(m_hServiceDDK);
	}
	return;
}

void UnLoadDriver(WCHAR* DriverName)
{
	SC_HANDLE sh=OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
	SC_HANDLE m_hServiceDDK=OpenServiceW(sh,DriverName,SERVICE_STOP | DELETE);
	DeleteService(m_hServiceDDK);
	if (sh)
	{
		CloseServiceHandle(sh);
	}
	if (m_hServiceDDK)
	{
		CloseServiceHandle(m_hServiceDDK);
	}
}
//
DWORD *GetPDE(DWORD addr)
{
    //return (DWORD *)(0xc0600000 + ((addr >> 18) & 0x3ff8));
    DWORD PDPTI = addr >> 30;
    DWORD PDI = (addr >> 21) & 0x000001FF;
    DWORD PTI = (addr >> 12) & 0x000001FF;
    return (DWORD *)(0xC0600000 + PDPTI * 0x1000 + PDI * 8);
}

DWORD *GetPTE(DWORD addr)
{
    //return (DWORD *)(0xc0000000 + ((addr >> 9) & 0x7ffff8));
    DWORD PDPTI = addr >> 30;
    DWORD PDI = (addr >> 21) & 0x000001FF;
    DWORD PTI = (addr >> 12) & 0x000001FF;
    return (DWORD *)(0xC0000000 + PDPTI * 0x200000 + PDI * 0x1000 + PTI * 8);
}

BOOL Ring0Communication(DWORD dwInBuffer)
{
	HANDLE hDevice=CreateFileA(SYMBOLICLINK_NAME,GENERIC_READ|GENERIC_WRITE,0,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
	DWORD dwError = GetLastError();
	if (hDevice == INVALID_HANDLE_VALUE)
    {        
        printf("获取设备句柄失败 %d.\n", dwError); // 如果返回1，请在驱动中指定 IRP_MJ_CREATE 处理函数
        getchar();
        return 1;
    }
	
	DWORD dwOutBuffer=0xffffffff;
	DWORD dwOut;
	DeviceIoControl(hDevice,OPER2,&dwInBuffer,IN_BUFFER_MAXLENGTH,&dwOutBuffer,OUT_BUFFER_MAXLENGTH,&dwOut,NULL);
	CloseHandle(hDevice);
}

BOOL UniCodeStrCmp(UNICODE_STRING a,UNICODE_STRING b)
{
	WCHAR cha;
	WCHAR chb;
	DWORD i=0;
	cha=a.Buffer[0];
	chb=b.Buffer[0];
	if(a.Length==b.Length&&wcsncmp(a.Buffer,b.Buffer,a.Length)==0)
	{
		return TRUE;
	}
	return FALSE;
}

DWORD FindModuleBase()
{
	PEB* peb;
	UNICODE_STRING b={0};
	b.Buffer=L"C:\\WINDOWS\\system32\\USER32.dll";
	//{Length=60 MaximumLength=62 Buffer=0x00252078 "C:\WINDOWS\system32\USER32.dll" }
	//0x004157f0 "C:\WINDOWS\system32\USER32.dll"
	b.Length=60;
	__asm
	{
		mov eax,fs:[0x30]
		mov peb,eax
	}
	PPEB_LDR_DATA pld=peb->Ldr;
	PLDR_DATA_TABLE_ENTRY pdte=(PLDR_DATA_TABLE_ENTRY)((DWORD)pld->InMemoryOrderModuleList.Flink-8);
	while((DWORD)pdte->InMemoryOrderLinks.Flink!=(DWORD)(&pld->InMemoryOrderModuleList))
	{
		pdte=(PLDR_DATA_TABLE_ENTRY)((DWORD)pdte->InMemoryOrderLinks.Flink-8);
		if(UniCodeStrCmp(pdte->FullDllName,b)==TRUE)
		{
			return (DWORD)pdte->DllBase;
		}
	}
	return 0;
}

int main()
{
	//Load Driver
	
	WCHAR Driverpath[]=L"driver_learn.sys";
	WCHAR DriverName[]=L"driver_learn";
	DWORD User32Base;
	LoadDriver(DriverName,Driverpath);
	//Search user32.dll position
	MessageBox(0,"Click True",0,0);
	User32Base=FindModuleBase();
	printf("User32DllBase:%x",User32Base);
	
	//User32 Addr send ring0
	Ring0Communication(User32Base);
	getchar();
	//UnLoadDriver
	UnLoadDriver(DriverName);
}
