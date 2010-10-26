/*
 * $Id$
 *
 * mngr505 unmount: Windows specific code
 *
 * Unmount functionality bazed on DevEject utility
 * by Matthias Withopf (http://www.withopf.com/tools/deveject)
 *
 */
#include <windows.h>
#include <QMessageBox>
#include "mngr505.h"

#define DN_REMOVABLE 0x00004000

class TDeviceEject
{
  public:
    bool bInit;

    enum
    {
        CR_SUCCESS          = 0x00,
        CR_NO_SUCH_DEVNODE  = 0x0D,
        CR_REMOVE_VETOED    = 0x17,
    };

    TDeviceEject();
    ~TDeviceEject();

    bool Eject(TCHAR* EjectDevSpec);
    static bool GetDriveDeviceId(TCHAR* DriveSpec,TCHAR* DeviceId,DWORD MaxDeviceIdSize);
private:
    HMODULE Lib;
    bool    UnloadLib;
    typedef DWORD  DEVINST,*PDEVINST;
    typedef TCHAR  *DEVINSTID_A;
    typedef enum
    {
        PNP_VetoTypeUnknown,
        PNP_VetoLegacyDevice,
        PNP_VetoPendingClose,
        PNP_VetoWindowsApp,
        PNP_VetoWindowsService,
        PNP_VetoOutstandingOpen,
        PNP_VetoDevice,
        PNP_VetoDriver,
        PNP_VetoIllegalDeviceRequest,
        PNP_VetoInsufficientPower,
        PNP_VetoNonDisableable,
        PNP_VetoLegacyDriver,
        PNP_VetoInsufficientRights
    } PNP_VETO_TYPE,* PPNP_VETO_TYPE;

    typedef DWORD (WINAPI *TCM_Locate_DevNodeW)      (OUT PDEVINST pdnDevInst,IN DEVINSTID_A pDeviceID,OPTIONAL IN ULONG ulFlags);
    typedef DWORD (WINAPI *TCM_Get_Child)            (OUT PDEVINST pdnDevInst,IN DEVINST dnDevInst,IN ULONG ulFlags);
    typedef DWORD (WINAPI *TCM_Get_Sibling)          (OUT PDEVINST pdnDevInst,IN DEVINST DevInst,IN ULONG ulFlags);
    typedef DWORD (WINAPI *TCM_Get_DevNode_Status)   (OUT PULONG pulStatus,OUT PULONG pulProblemNumber,IN DEVINST dnDevInst,IN ULONG ulFlags);
    typedef DWORD (WINAPI *TCM_Get_Device_ID_Size)   (OUT PULONG pulLen,IN DEVINST dnDevInst,IN ULONG ulFlags);
    typedef DWORD (WINAPI *TCM_Get_Device_IDW)       (IN DEVINST dnDevInst,OUT TCHAR* Buffer,IN ULONG BufferLen,IN ULONG ulFlags);
    typedef DWORD (WINAPI *TCM_Request_Device_EjectW)(IN DEVINST dnDevInst,OUT PPNP_VETO_TYPE pVetoType,OUT LPWSTR pszVetoName,IN ULONG ulNameLength,IN ULONG ulFlags);

    TCM_Locate_DevNodeW       Func_LocateDevNode;
    TCM_Get_Child             Func_GetChild;
    TCM_Get_Sibling           Func_GetSibling;
    TCM_Get_DevNode_Status    Func_GetDevNodeStatus;
    TCM_Get_Device_ID_Size    Func_GetDeviceIDSize;
    TCM_Get_Device_IDW        Func_GetDeviceID;
    TCM_Request_Device_EjectW Func_RequestDeviceEject;

    typedef struct _TEnumDeviceInfo
    {
        struct _TEnumDeviceInfo *Parent;
        DEVINST                  DevInst;
        ULONG                    Status;
        ULONG                    ProblemNumber;
    } TEnumDeviceInfo,* PEnumDeviceInfo;

    bool EnumDevices(PEnumDeviceInfo ParentEnumDeviceInfo,PDWORD EjectDeviceFound,TCHAR* EjectDevSpec,DEVINST DevInst,int Indent);
};

typedef TDeviceEject * PDeviceEject;

TDeviceEject::TDeviceEject()
{
    bInit = true;
    Lib                     = NULL;
    UnloadLib               = false;
    Func_LocateDevNode      = NULL;
    Func_GetChild           = NULL;
    Func_GetSibling         = NULL;
    Func_GetDevNodeStatus   = NULL;
    Func_GetDeviceIDSize    = NULL;
    Func_GetDeviceID        = NULL;
    Func_RequestDeviceEject = NULL;

    DWORD V = GetVersion();
    if (!(V & 0x80000000))
    {
        BYTE MajorVersion = (BYTE)V;
        if (MajorVersion < 5)
            bInit = false;
        else
        {
            const TCHAR* LibFileName = L"setupapi.dll";
            Lib = GetModuleHandle(LibFileName);
            if (!Lib)
            {
                Lib = LoadLibrary(LibFileName);
                if (Lib)
                    UnloadLib = true;
            }
            if (!Lib)
                bInit = false;
            else
            {
                Func_LocateDevNode      = (TCM_Locate_DevNodeW)      GetProcAddress(Lib,"CM_Locate_DevNodeW");
                Func_GetChild           = (TCM_Get_Child)            GetProcAddress(Lib,"CM_Get_Child");
                Func_GetSibling         = (TCM_Get_Sibling)          GetProcAddress(Lib,"CM_Get_Sibling");
                Func_GetDevNodeStatus   = (TCM_Get_DevNode_Status)   GetProcAddress(Lib,"CM_Get_DevNode_Status");
                Func_GetDeviceIDSize    = (TCM_Get_Device_ID_Size)   GetProcAddress(Lib,"CM_Get_Device_ID_Size");
                Func_GetDeviceID        = (TCM_Get_Device_IDW)       GetProcAddress(Lib,"CM_Get_Device_IDW");
                Func_RequestDeviceEject = (TCM_Request_Device_EjectW)GetProcAddress(Lib,"CM_Request_Device_EjectW");
            }
        }
    }
}

TDeviceEject::~TDeviceEject()
{
    if (Lib && UnloadLib)
        FreeLibrary(Lib);
}

bool TDeviceEject::GetDriveDeviceId(TCHAR* DriveSpec,TCHAR* DeviceId,DWORD MaxDeviceIdSize)
{
    bool Result = false;
    if (DeviceId && MaxDeviceIdSize)
        DeviceId[0] = '\0';
    if (DriveSpec && DriveSpec[0] && DeviceId && MaxDeviceIdSize)
    {
        HKEY Key;
        LONG L = RegOpenKeyEx(HKEY_LOCAL_MACHINE,L"SYSTEM\\MountedDevices",0,KEY_QUERY_VALUE,&Key);
        if (L == ERROR_SUCCESS)
        {
            DWORD Type;
            BYTE  Value[1024];
            DWORD ValueSize = sizeof Value;
            TCHAR ValueName[256];
            wcscpy(ValueName,L"\\DosDevices\\");
            wcsncat(ValueName,DriveSpec,255);
            ValueName[255] = '\0';
            L = RegQueryValueEx(Key,ValueName,0,&Type,(BYTE *)Value,&ValueSize);
            RegCloseKey(Key);
            if ((L == ERROR_SUCCESS) && (Type == REG_BINARY))
            {
                PWSTR p1 = (PWSTR)Value;
                ValueSize /= sizeof *p1;
                if ((p1[0] == '_') && (p1[1] == '?') && (p1[2] == '?') && (p1[3] == '_'))
                {
                    Result = true;
                    p1        += 4;
                    ValueSize -= 4;
                    TCHAR* p2 = DeviceId;
                    for (unsigned int i = 0;i < ValueSize;++i)
                    {
                        TCHAR c = p1[i];
                        if ((c == '{') || ((c == '#') && (p1[i + 1] == '{')))
                            break;
                        if (c == '#')
                            c = '\\';
                        if ((p2 - DeviceId) < (MaxDeviceIdSize - 1))
                            *p2++ = c;
                    }
                    *p2 = '\0';
                }
            }
        }
    }
    return Result;
}

static bool CompareWithWildcard(TCHAR* s1,TCHAR* s2)
{
    if (s1 && s2)
    {
        for (;;)
        {
            if (*s2 == '*')
                return true;
            if (!*s1)
            {
                if (!*s2)
                    return true;
                break;
            }
            if (!*s2)
            {
                if (!*s1)
                    return true;
                break;
            }
            if (toupper(*s1++) != toupper(*s2++))
                break;
        }
    }
    return false;
}

bool TDeviceEject::EnumDevices(PEnumDeviceInfo ParentEnumDeviceInfo,PDWORD EjectDeviceFound,TCHAR* EjectDevSpec,DEVINST DevInst,int Indent)
{
    bool bResult = true;

    DEVINST ThisDevInst = DevInst;
    while (bResult == true)
    {
        ULONG Status; // DN_...
        ULONG ProblemNumber;
        DWORD r = Func_GetDevNodeStatus(&Status,&ProblemNumber,ThisDevInst,0);
        if (r == CR_NO_SUCH_DEVNODE)
        {
        }
        else if (r != CR_SUCCESS)
        {
          bResult = false;
        }
        if (bResult == true)
        {
            ULONG DeviceIDLen;
            r = Func_GetDeviceIDSize(&DeviceIDLen,ThisDevInst,0);
            if (r != CR_SUCCESS)
                bResult = false;
            else
            {
                TCHAR* DeviceID = new TCHAR[DeviceIDLen + 1];
                if (DeviceID)
                {
                    r = Func_GetDeviceID(ThisDevInst,DeviceID,DeviceIDLen + 1,0);
                    if (r != CR_SUCCESS)
                        bResult = false;
                    else
                    {
                        bool Found = false;
                        if (EjectDevSpec && EjectDevSpec[0])
                        {
                            Found = CompareWithWildcard(DeviceID,EjectDevSpec);
                        }
                        if (Found)
                        {
                            ++*EjectDeviceFound;
                            DEVINST EjectDevInst = ThisDevInst;
                            if (!(Status & DN_REMOVABLE))
                            {
                                for (PEnumDeviceInfo EDI = ParentEnumDeviceInfo;EDI;EDI = EDI->Parent)
                                {
                                    if (EDI->Status & DN_REMOVABLE)
                                    {
                                        EjectDevInst  = EDI->DevInst;
                                        Status        = EDI->Status;
                                        ProblemNumber = EDI->ProblemNumber;
                                        break;
                                    }
                                }
                            }

                            if (!(Status & DN_REMOVABLE))
                                bResult = false;
                            else
                            {
                                if (ProblemNumber)
                                    bResult = false;
                                else
                                {
                                    PNP_VETO_TYPE VetoType;
                                    TCHAR VetoName[MAX_PATH];
                                    r = Func_RequestDeviceEject(EjectDevInst,&VetoType,VetoName,sizeof VetoName,0);
                                    if ((r == CR_SUCCESS) &&  ((VetoType == PNP_VetoDevice) || (VetoType == PNP_VetoDriver)))
                                        r = CR_REMOVE_VETOED;

                                    if (r != CR_SUCCESS)
                                        bResult = false;
                                }
                            }
                        }
                    }
                    delete [] DeviceID;
                }
            }
        }

        if (bResult == true)
        {
            DEVINST Child;
            r = Func_GetChild(&Child,ThisDevInst,0);
            if (r == CR_NO_SUCH_DEVNODE)
            {
            }
            else if (r != CR_SUCCESS)
                bResult = false;
            else
            {
                TEnumDeviceInfo EnumDeviceInfo;
                EnumDeviceInfo.Parent        = ParentEnumDeviceInfo;
                EnumDeviceInfo.DevInst       = ThisDevInst;
                EnumDeviceInfo.Status        = Status;
                EnumDeviceInfo.ProblemNumber = ProblemNumber;
                bResult = EnumDevices(&EnumDeviceInfo,EjectDeviceFound,EjectDevSpec,Child,Indent + 1);
            }
            if (bResult == true)
            {
                DEVINST Sibling;
                r = Func_GetSibling(&Sibling,ThisDevInst,0);
                if (r == CR_NO_SUCH_DEVNODE)
                    break;
                if (r != CR_SUCCESS)
                    bResult = false;
                ThisDevInst = Sibling;
            }
        }
    }
    return bResult;
}

bool TDeviceEject::Eject(TCHAR* EjectDevSpec)
{
    bool bResult = true;
    if (!Func_LocateDevNode || !Func_GetChild || !Func_GetSibling || !Func_GetDevNodeStatus || !Func_GetDeviceIDSize || !Func_GetDeviceID || !Func_RequestDeviceEject)
        bResult = false;
    else
    {
        DEVINST RootDevInst;
        DWORD r = Func_LocateDevNode(&RootDevInst,NULL,0/*CM_LOCATE_DEVNODE_NORMAL*/);
        if (r != CR_SUCCESS)
            bResult = false;
        else
        {
            DWORD EjectDeviceFound = 0;
            bResult = EnumDevices(NULL,&EjectDeviceFound,EjectDevSpec,RootDevInst,0);
            if ((bResult == true) && EjectDevSpec[0] && !EjectDeviceFound)
                bResult = false;
        }
    }
    return bResult;
}

///////////////////////////////////////////////////////////////////////
static bool umount1(TCHAR cDriveLetter)
{
    TCHAR EjectDrive[80]={0};
    TCHAR EjectDevSpec[1024]={0};
    bool bResult=false;

    swprintf(EjectDrive,20,L"%c:",cDriveLetter);
    if ( !TDeviceEject::GetDriveDeviceId(EjectDrive,EjectDevSpec,1024) )
        return false;

    PDeviceEject DeviceEject = new TDeviceEject();
    if ( !DeviceEject )
        return false;

    if (DeviceEject->bInit)
        bResult=DeviceEject->Eject(EjectDevSpec);

    delete DeviceEject;
    return bResult;
} // umount1

///////////////////////////////////////////////////////////////////////
void mngr505::umount()
{
    _ui.lFPanel->cd("C:", true);
    if (umount1(_ui.lFPanel->root().at(0).toAscii()))
    {
        QMessageBox::information(0, tr("umount complete"),
                                 tr("eBook on %1 successfully unmounted.")
                                 .arg(_ui.lFPanel->root()));
        _ui.lFPanel->notFound("C:");
    }
    else
        QMessageBox::information(0, tr("umount failure"),
                                 tr("Can't umnount eBook on %1")
                                 .arg(_ui.lFPanel->root()));
} // mngr505::umount
