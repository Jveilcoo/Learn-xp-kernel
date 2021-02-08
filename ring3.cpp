#include<Windows.h>
#include<stdio.h>
#include <winioctl.h>

#define SYMBOLICLINK_NAME "\\\\.\\HbgDevLnk"
#define OPER1 CTL_CODE(FILE_DEVICE_UNKNOWN,0x800,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define OPER2 CTL_CODE(FILE_DEVICE_UNKNOWN,0x900,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IN_BUFFER_MAXLENGTH 4
#define OUT_BUFFER_MAXLENGTH 4
//Load .sys(Driver)
VOID LoadDriver(LPCSTR DriverName, LPCSTR DriverPath)
{
	//Create SC Manager
	SC_HANDLE sh = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (sh == NULL)
	{
		printf("OpenSCManager Error!\n");
		return;
	}
	//Create Service
	SC_HANDLE m_hServiceDDK = CreateService(
		sh, \
		DriverName, \
		DriverName, \
		SERVICE_ALL_ACCESS, \
		SERVICE_KERNEL_DRIVER, \
		SERVICE_DEMAND_START, \
		SERVICE_ERROR_IGNORE, \
		DriverPath, \
		NULL, \
		NULL, \
		NULL, \
		NULL, \
		NULL);
	if (m_hServiceDDK == NULL)
	{
		printf("CreateService Error!\n");
		return;
	}

	//OpenService
	m_hServiceDDK = OpenService(sh, DriverName, SERVICE_ALL_ACCESS);

	if (m_hServiceDDK)
	{
		printf("Set Driver success\n");
	}
	if (!StartService(m_hServiceDDK, NULL, NULL))
	{
		DWORD dwErr = GetLastError();
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

void UnLoadDriver(LPCSTR DriverName)
{
	SC_HANDLE sh = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	SC_HANDLE m_hServiceDDK = OpenService(sh, DriverName, SERVICE_STOP | DELETE);
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
PVOID GetPDE()
{

}
//
PVOID GetPTE()
{

}

BOOL Ring0Communication()
{
	HANDLE hDevice = CreateFileA(SYMBOLICLINK_NAME, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	DWORD dwError = GetLastError();
	if (hDevice == INVALID_HANDLE_VALUE)
	{
		printf("获取设备句柄失败 %d.\n", dwError); // 如果返回1，请在驱动中指定 IRP_MJ_CREATE 处理函数
		getchar();
		return 1;
	}
	DWORD dwInBuffer;
	DWORD dwOutBuffer = 0xffffffff;
	DWORD dwOut;
	DeviceIoControl(hDevice, OPER2, &dwInBuffer, IN_BUFFER_MAXLENGTH, &dwOutBuffer, OUT_BUFFER_MAXLENGTH, &dwOut, NULL);
	CloseHandle(hDevice);
}

int main()
{
	//Load Driver
	char Driverpath[256];
	char DriverName[256];
	printf("Please Driver Name:");
	scanf("%s", DriverName);
	printf("Please Driver Path:");
	scanf("%s", Driverpath);
	LoadDriver(DriverName, Driverpath);



	//UnLoadDriver
	UnLoadDriver(DriverName);
}