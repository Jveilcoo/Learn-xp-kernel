#include<ntifs.h>
#include<ntddk.h>


#define DEVICE_NAME L"\\Device\\HbgDev"
#define SYMBOLICLINK_NAME L"\\??\\HbgDevLnk"

#define OPER1 CTL_CODE(FILE_DEVICE_UNKNOWN,0x800,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define OPER2 CTL_CODE(FILE_DEVICE_UNKNOWN,0x900,METHOD_BUFFERED,FILE_ANY_ACCESS)

typedef NTSTATUS(*_PspTerminateProcess)(PEPROCESS pEprocess, NTSTATUS ExitCode);
typedef struct
{
    LIST_ENTRY InLoadOrderLinks;
    LIST_ENTRY InMemoryOrderLinks;
    LIST_ENTRY InInitializationOrderLinks;
    PVOID DllBase;
    PVOID EntryPoint;
    UINT32 SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    UINT32 Flags;
    UINT16 LoadCount;
    UINT16 TlsIndex;
    LIST_ENTRY HashLinks;
    PVOID SectionPointer;
    UINT32 CheckSum;
    UINT32 TimeDateStamp;
    PVOID LoadedImports;
    PVOID EntryPointActivationContext;
    PVOID PatchInformation;

}LDR_DATA_TABLE_ENTRY, * PLDR_DATA_TABLE_ENTRY;

_PspTerminateProcess PspTerminateProcess;
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING reg_path);
UINT32 *GetPDE(UINT32 addr);
UINT32 *GetPTE(UINT32 addr);
VOID DriverUnload(PDRIVER_OBJECT pDriver);
NTSTATUS IrpCreateProc(PDEVICE_OBJECT pDevObj, PIRP pIrp);
NTSTATUS IrpCloseProc(PDEVICE_OBJECT pDevObj ,PIRP pIrp);
NTSTATUS IrpDeviceControlProc(PDEVICE_OBJECT pDevObj, PIRP pIrp);

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING reg_path)
{
    NTSTATUS status;
    PDEVICE_OBJECT pDeviceObj;
    UNICODE_STRING Devicename;
    UNICODE_STRING SymbolicLinkName;
    PVOID KernelBase=NULL;
    UINT32 uKernelImageSize;

    //初始化设备名
    RtlInitUnicodeString(&Devicename, DEVICE_NAME);
    //创建设备对象
    status=IoCreateDevice(pDriver, 0, &Devicename, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE,&pDeviceObj);

    pDeviceObj->Flags |= DO_BUFFERED_IO;

    if (status != STATUS_SUCCESS)
    {
        IoDeleteDevice(pDeviceObj);
        DbgPrint("创建设备失败.\n");
        return status;
    }

    //创建符号链接名称
    RtlInitUnicodeString(&SymbolicLinkName, SYMBOLICLINK_NAME);
    //创建符号链接
    IoCreateSymbolicLink(&SymbolicLinkName, &Devicename);

    //创建分发函数
    pDriver->DriverUnload = DriverUnload;
    pDriver->MajorFunction[IRP_MJ_CREATE] = IrpCreateProc;
    pDriver->MajorFunction[IRP_MJ_CLOSE] = IrpCloseProc;
    pDriver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IrpDeviceControlProc;
    

    return STATUS_SUCCESS;
}

UINT32 *GetPDE(UINT32 addr)
{
    //return (DWORD *)(0xc0600000 + ((addr >> 18) & 0x3ff8));
    UINT32 PDPTI = addr >> 30;
    UINT32 PDI = (addr >> 21) & 0x000001FF;
    UINT32 PTI = (addr >> 12) & 0x000001FF;
    return (UINT32 *)(0xC0600000 + PDPTI * 0x1000 + PDI * 8);
}

UINT32 *GetPTE(UINT32 addr)
{
    //return (DWORD *)(0xc0000000 + ((addr >> 9) & 0x7ffff8));
    UINT32 PDPTI = addr >> 30;
    UINT32 PDI = (addr >> 21) & 0x000001FF;
    UINT32 PTI = (addr >> 12) & 0x000001FF;
    return (UINT32 *)(0xC0000000 + PDPTI * 0x200000 + PDI * 0x1000 + PTI * 8);
}

VOID DriverUnload(PDRIVER_OBJECT pDriver)
{
    DbgPrint("驱动卸载成功\n");
}

NTSTATUS IrpCreateProc(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
    DbgPrint("Application connect device");
    //返回状态如果不设置，Ring3返回值是失败
    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;
    
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS IrpCloseProc(PDEVICE_OBJECT pDevObj ,PIRP pIrp)
{
    DbgPrint("Application disconnection");
    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS IrpDeviceControlProc(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;

    PIO_STACK_LOCATION pIrpStack;
    PVOID pIoBuffer;
	UINT32* phyAddr;
    ULONG uIoControlCode;
    ULONG uOutLength;
    ULONG uInLength;
    ULONG uWrite;
    ULONG uRead;
    PEPROCESS pE;

    //设置临时变量的值
    uRead = 0;
    uWrite = 0x12345678;
    //获取IRP数据
    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    //获取控制码
    uIoControlCode = pIrpStack->Parameters.DeviceIoControl.IoControlCode;
    //获取缓冲区地址（输入输出是同一个）
    pIoBuffer = pIrp->AssociatedIrp.SystemBuffer;
    //Ring3 发送数据的长度
    uInLength = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;
    //Ring0 发送数据的长度
    uOutLength = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;
    DbgPrint("execute\n");
    switch (uIoControlCode)
    {
		
        case OPER1:
        {
            DbgPrint("IrpDeviceControlProc -> OPER1...\n");
            pIrp->IoStatus.Information = 0;
            status = STATUS_SUCCESS;
            break;
        }
        case OPER2:
        {
            DbgPrint("IrpDeviceControlProc -> OPER2 输入字节数: %d\n", uInLength);
            DbgPrint("IrpDeviceControlProc -> OPER2 输出字节数: %d\n", uOutLength);
            memcpy(&uRead, pIoBuffer, 4);
			DbgPrint("%d\n",uRead);
			*GetPDE(uRead)=*phyAddr|0x867;
			*GetPTE(uRead)=*phyAddr|0x867;
			uRead=200;
			memcpy(pIoBuffer,&uRead,4);
            pIrp->IoStatus.Information = 4; // 返回两字节
            status = STATUS_SUCCESS;
            break;
        }
    }

    return STATUS_SUCCESS;
}
