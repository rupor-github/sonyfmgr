/*
 * $Id$
 *
 * mngr505 unmount: Windows specific code
 *
 * Unmount functionality bazed on WASM.RU resource information
 *
 */
#include <windows.h>
#if !defined(_MSC_VER)
#   include <ddk\ntddstor.h>
#   include <ddk\cfgmgr32.h>
#else
#   include <ntddstor.h>
#   include <cfgmgr32.h>
#endif
#include <setupapi.h>
#include <QMessageBox>
#include "mngr505.h"

DEVINST GetDevInstByDeviceNumber(long lDeviceNumber, UINT iDriveType, TCHAR* szDosDeviceName)
{

    GUID guid={0x53f56307L, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b}; //GUID_DEVINTERFACE_DISK;

    HDEVINFO hDevInfo = SetupDiGetClassDevs(&guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if ( INVALID_HANDLE_VALUE == hDevInfo )
        return 0;

    DWORD dwIndex = 0;
    DWORD dwSize=0;
    BYTE sBuf[1024];
    PSP_DEVICE_INTERFACE_DETAIL_DATA pspdidd=(PSP_DEVICE_INTERFACE_DETAIL_DATA)sBuf;
    SP_DEVICE_INTERFACE_DATA spdid;
    SP_DEVINFO_DATA spdd;

    ZeroMemory((PVOID)&spdd, sizeof(spdd));
    spdid.cbSize = sizeof(spdid);
    while (true)
    {
        if ( !SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &guid, dwIndex, &spdid) )
            break;

        SetupDiGetDeviceInterfaceDetail(hDevInfo, &spdid, NULL, 0, &dwSize, NULL);

        if ( dwSize!=0 && dwSize<=(sizeof(sBuf)/sizeof(sBuf[0])) ) {
            pspdidd->cbSize = sizeof(*pspdidd);
            ZeroMemory((PVOID)&spdd, sizeof(spdd));
            spdd.cbSize = sizeof(spdd);
            if ( SetupDiGetDeviceInterfaceDetail(hDevInfo, &spdid, pspdidd, dwSize, &dwSize, &spdd) )
            {
                HANDLE hDrive = CreateFile(pspdidd->DevicePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
                if ( INVALID_HANDLE_VALUE != hDrive )
                {
                    STORAGE_DEVICE_NUMBER sdn;
                    DWORD dwBytesReturned = 0;
                    if ( DeviceIoControl(hDrive, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &sdn, sizeof(sdn), &dwBytesReturned, NULL))
                    {
                        if ( lDeviceNumber == (long)sdn.DeviceNumber )
                        {
                            CloseHandle(hDrive);
                            SetupDiDestroyDeviceInfoList(hDevInfo);
                            return spdd.DevInst;
                        }
                    }
                    CloseHandle(hDrive);
                }
            }
        }
        dwIndex++;
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);
    return 0;
}

static bool umount1(TCHAR cDriveLetter)
{
    TCHAR szRootPath[4] = {0};
    TCHAR szDevicePath[3] ={0};
    TCHAR szVolumeAccessPath[7] = {0};
    TCHAR szDosDeviceName[MAX_PATH]={0};
    UINT iDriveType=0;
    STORAGE_DEVICE_NUMBER sdn;
    DWORD dwBytesReturned = 0;
    DEVINST DevInst=0;
    DEVINST DevInstParent=0;

    swprintf(szRootPath,L"%c:\\",cDriveLetter);
    swprintf(szDevicePath,L"%c:",cDriveLetter);
    swprintf(szVolumeAccessPath,L"\\\\.\\%c:",cDriveLetter);

    HANDLE hVolume = CreateFile(szVolumeAccessPath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (INVALID_HANDLE_VALUE != hVolume)
    {
        if (! DeviceIoControl(hVolume, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &sdn, sizeof(sdn), &dwBytesReturned, NULL) )
        {
            CloseHandle(hVolume);
            return false;
        }
        CloseHandle(hVolume);

        iDriveType = GetDriveType(szRootPath);
        if (DRIVE_REMOVABLE == iDriveType)
        {
            if ( QueryDosDevice(szDevicePath, szDosDeviceName, sizeof(szDosDeviceName)/sizeof(szDosDeviceName[0])) )
            {
                DevInst = GetDevInstByDeviceNumber(sdn.DeviceNumber,iDriveType, szDosDeviceName);
                if ( 0 != DevInst )
                {
                    if ( CR_SUCCESS == CM_Get_Parent(&DevInstParent, DevInst, 0))
                    {
                        if ( CR_SUCCESS == CM_Request_Device_Eject(DevInstParent, NULL, NULL, 0, 0))
                            return true;
                    }
                }
            }
        }
    }
    return false;
} // umount1

///////////////////////////////////////////////////////////////////////
void mngr505::umount()
{
char win_path[ MAX_PATH + 1 ];

    if( 0 == GetWindowsDirectoryA( win_path, MAX_PATH ) )
	    strcpy( win_path, "C:" );
	 else
       win_path[ 2 ] = '\0';

    _ui.lFPanel->cd(win_path, true);

    if (umount1(_ui.lFPanel->root().at(0).toAscii()))
    {
        QMessageBox::information(0, tr("umount complete"),
                                 tr("eBook on %1 successfully unmounted.")
                                 .arg(_ui.lFPanel->root()));
        _ui.lFPanel->notFound(win_path);
    }
    else
        QMessageBox::information(0, tr("umount failure"),
                                 tr("Can't umnount eBook on %1")
                                 .arg(_ui.lFPanel->root()));
} // mngr505::umount
