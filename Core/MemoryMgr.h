#pragma once
#include <iostream>
#include <Windows.h>
#include <string>
#include <vector>

#define DRAGON_DEVICE 0x8000
#define IOCTL_GET_PID CTL_CODE(DRAGON_DEVICE, 0x4452, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_GET_MODULE_BASE CTL_CODE(DRAGON_DEVICE, 0x4462, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_READ_PROCESS_MEMORY CTL_CODE(DRAGON_DEVICE, 0x4472, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_WRITE_PROCESS_MEMORY CTL_CODE(DRAGON_DEVICE, 0x4482, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_WRITE_PROCESS_MEMORY_PROTECTED CTL_CODE(DRAGON_DEVICE, 0x4492, METHOD_NEITHER, FILE_ANY_ACCESS)


class MemoryMgr
{
public:
	MemoryMgr();
	~MemoryMgr();

	bool ConnectDriver(const LPCWSTR);
	bool DisconnectDriver();
	bool Attach(const DWORD);

    DWORD64 GetModuleBase(const wchar_t*);
    DWORD GetProcessID(const wchar_t*);

    template <typename ReadType>
    bool ReadMemory(DWORD64 address, ReadType& value, SIZE_T size = sizeof(ReadType))
    {
        if (kernelDriver != nullptr && ProcessID != 0)
        {
            READ_PACK ReadPack;
            ReadPack.pid = ProcessID;
            ReadPack.address = reinterpret_cast<PVOID>(address);
            ReadPack.buff = &value;
            ReadPack.size = size;

            BOOL result = DeviceIoControl(kernelDriver,
                IOCTL_READ_PROCESS_MEMORY,
                &ReadPack,
                sizeof(ReadPack),
                &ReadPack,
                sizeof(ReadPack),
                nullptr,
                nullptr);

            return result == TRUE ; // && bytesReturned == size
        }
        return false;
    }

    //template <typename WriteType>
    //bool WriteMemory(DWORD64 address, WriteType& value, SIZE_T size = sizeof(WriteType))
    //{
    //    if (kernelDriver != INVALID_HANDLE_VALUE && ProcessID != 0)
    //    {
    //        WRITE_PACK WritePack;
    //        WritePack.pid = ProcessID;
    //        WritePack.address = reinterpret_cast<PVOID>(address);
    //        WritePack.buff = const_cast<void*>(value);
    //        WritePack.size = size;

    //        BOOL result = DeviceIoControl(kernelDriver,
    //            IOCTL_WRITE_PROCESS_MEMORY,
    //            &WritePack,
    //            sizeof(WritePack),
    //            nullptr,
    //            0,
    //            nullptr,
    //            nullptr);

    //        return result == TRUE;
    //    }
    //    return false;
    //}

    //template <typename WriteType>
    //bool WriteMemoryProtected(DWORD64 address, WriteType& value, SIZE_T size = sizeof(WriteType))
    //{
    //    if (kernelDriver != INVALID_HANDLE_VALUE && ProcessID != 0)
    //    {
    //        WRITE_PACK WritePack;
    //        WritePack.pid = ProcessID;
    //        WritePack.address = reinterpret_cast<PVOID>(address);
    //        WritePack.buff = const_cast<void*>(value);
    //        WritePack.size = size;

    //        BOOL result = DeviceIoControl(kernelDriver,
    //            IOCTL_WRITE_PROCESS_MEMORY_PROTECTED,
    //            &WritePack,
    //            sizeof(WritePack),
    //            nullptr,
    //            0,
    //            nullptr,
    //            nullptr);

    //        return result == TRUE;
    //    }
    //    return false;
    //}

	DWORD64 TraceAddress(DWORD64, std::vector<DWORD>);

private:
	DWORD ProcessID;
	HANDLE kernelDriver;

    // Structure for getting pid by name
    typedef struct _PID_PACK
    {
        UINT32 pid;
        WCHAR name[1024];
    } PID_PACK, * P_PID_PACK;

    // Structure for getting module address base
    typedef struct _MODULE_PACK {
        UINT32 pid;
        UINT64 baseAddress;
        SIZE_T size;
        WCHAR moduleName[1024];
    } MODULE_PACK, * P_MODULE_PACK;

    // Structure for writing memory to a process
    typedef struct _WRITE_PACK {
        UINT32 pid;
        PVOID address;
        SIZE_T size;
        PVOID buff;
    } WRITE_PACK, * P_WRITE_PACK;

    // Structure for reading memory from a process
    typedef struct _READ_PACK
    {
        UINT32 pid;
        PVOID address;
        SIZE_T size;
        PVOID buff;
    } READ_PACK, * P_READ_PACK;

};

inline MemoryMgr memoryManager;

