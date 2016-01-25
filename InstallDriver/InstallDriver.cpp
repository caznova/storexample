// BuildSvc.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>
#include <windows.h>
//#include <winioctl.h>
#include <strsafe.h>

#define WIN32_ROOT_PREFIX "\\\\.\\KMS"

#define KMS_IOCTL(CODE) (CTL_CODE (FILE_DEVICE_UNKNOWN, 0x800 + (CODE), METHOD_BUFFERED, FILE_ANY_ACCESS))

#define KMS_IOCTL_MOUNT						KMS_IOCTL (1)
#define KMS_IOCTL_WRITE						KMS_IOCTL (2)
#define KMS_IOCTL_READ						KMS_IOCTL (3)


int FakeDosNameForDevice(const char *lpszDiskFile, char *lpszDosDevice, char *lpszCFDevice, BOOL bNameOnly)
{
	BOOL bDosLinkCreated = TRUE;
	sprintf_s(lpszDosDevice, MAX_PATH ,"KMS%lu", GetCurrentProcessId());

	if (bNameOnly == FALSE)
		bDosLinkCreated = DefineDosDevice(DDD_RAW_TARGET_PATH, lpszDosDevice, lpszDiskFile);

	if (bDosLinkCreated == FALSE)
		return 1;
	else
		sprintf_s(lpszCFDevice, MAX_PATH, "\\\\.\\%s", lpszDosDevice);
	printf("디바이스 이름lpszCFDevice = %s lpszDosDevice = %s \n", lpszCFDevice, lpszDosDevice);
	return 0;
}

volatile BOOLEAN FormatExResult;
#define FMIFS_DONE		0xB
#define FMIFS_HARDDISK	0xC

BOOLEAN __stdcall FormatExCallback(int command, DWORD subCommand, PVOID parameter)
{

	if (command == FMIFS_DONE)
		FormatExResult = *(BOOLEAN *)parameter;
	printf("포맷결과 = %d\n", FormatExResult);
	return TRUE;
}
typedef BOOLEAN(__stdcall *PFMIFSCALLBACK)(int command, DWORD subCommand, PVOID parameter);
typedef VOID(__stdcall *PFORMATEX)(PCHAR DriveRoot, DWORD MediaFlag, PCHAR Format, PCHAR Label, BOOL QuickFormat, DWORD ClusterSize, PFMIFSCALLBACK Callback);

BOOL FormatNtfs(int driveNo, int clusterSize)
{
	char dir[8] = { (char)driveNo + 'A', 0 };
	PFORMATEX FormatEx;
	HMODULE hModule = LoadLibrary("fmifs.dll");
	int i;

	if (hModule == NULL)
		return FALSE;

	if (!(FormatEx = (PFORMATEX)GetProcAddress(GetModuleHandle("fmifs.dll"), "FormatEx")))
	{
		printf("FormatEx를 가져올 수 없습니다 error hex = %x\n", GetLastError());
		FreeLibrary(hModule);
		return FALSE;
	}

	strcat_s(dir,MAX_PATH, ":\\");
	printf("포맷할 경로 dir = %s\n", dir);
	FormatExResult = FALSE;

	// Windows sometimes fails to format a volume (hosted on a removable medium) as NTFS.
	// It often helps to retry several times.
	for (i = 0; i < 50 && FormatExResult != TRUE; i++)
	{
		FormatEx(dir, FMIFS_HARDDISK, "NTFS", "", TRUE, 10 * 512/*FormatSectorSize*/, FormatExCallback);
	}

	// The device may be referenced for some time after FormatEx() returns
	Sleep(2000);

	FreeLibrary(hModule);
	return FormatExResult;
}

int GetLastAvailableDrive()
{
	DWORD dwUsedDrives = GetLogicalDrives();
	int i;

	for (i = 25; i > 2; i--)
	{
		if (!(dwUsedDrives & 1 << i))
			return i;
	}

	return -1;
}


int main(int argc, char* argv[])
{
	SC_HANDLE   schSCManager;
	SC_HANDLE   schService;

	if (argc < 3)
	{
		printf("[사용방법]\n");
		printf("InstallDriver.EXE [SERVICENAME] [SERVICEDRIVER's FULLPATHNAME] [R]\n");
		printf("[SERVICENAME] : 등록할 서비스이름\n");
		printf("[SERVICEDRIVER's FULLPATHNAME] : 드라이버가 들어있는 FullPath\n");
		printf("[R] : 서비스를 자동으로 실행하는 기능(생략하면 실행을 하지는 않는다)\n");
		return 0;
	}
	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!schSCManager)
	{
		printf("서비스를 등록할 메니져가 없습니다 error hex = %x\n", GetLastError());
		return 0;
	}

	schService = OpenService(schSCManager, argv[1], SERVICE_ALL_ACCESS);
	if (schService == NULL)
	{
		schService = CreateService(schSCManager, argv[1], argv[1], SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START
			, SERVICE_ERROR_NORMAL, argv[2], NULL, NULL, NULL, NULL, NULL);
		if (schService == NULL)
		{
			printf("기존에 서비스는 등록되어있지 않는데, 이상하게도 서비스생성이 거부되고 있습니다 error hex = %x\n", GetLastError());
		}
		else
		{
			printf("기존에 서비스가 등록된적이 없으며, 서비스가 성공적으로 생성되었습니다\n");
			unsigned char * pTemp;
			pTemp = (unsigned char *)argv[3];

			if ((argc > 3) && ((pTemp[0] == 'R') || (pTemp[0] == 'r')))
			{
				BOOLEAN bRet = StartService(schService, 0, NULL); // DriverEntry가 호출된다
				if (bRet == FALSE)
					printf("서비스가 실행될수 없습니다 error hex = %x\n", GetLastError());
				else
					printf("서비스가 실행되었습니다\n");
			}
			CloseServiceHandle(schService);
		}
	}
	else
	{
		printf("서비스가 이미존재하기때문에 서비스생성에 대한 기능은 수행하지 못합니다\n");
		unsigned char * pTemp;
		pTemp = (unsigned char *)argv[3];
		if ((argc > 3) && ((pTemp[0] == 'R') || (pTemp[0] == 'r')))
		{
			BOOLEAN bRet = StartService(schService, 0, NULL); // DriverEntry가 호출된다
			if (bRet == FALSE)
				printf("서비스가 실행될수 없습니다 error hex = %x\n", GetLastError());
			else
				printf("서비스가 실행되었습니다\n");
		} 
		else if ((argc > 3) && ((pTemp[0] == 'T') || (pTemp[0] == 't')))
		{



			char volumePath[MAX_PATH] = "\\Device\\Harddisk1\\Partition1";
			char dosDev[MAX_PATH] = { 0, };
			char devName[MAX_PATH] = { 0, };

			FakeDosNameForDevice(volumePath, dosDev, devName,false);


			HANDLE dev = INVALID_HANDLE_VALUE;
			dev = CreateFile(devName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);


			// 드라이버 통신 시작
			HANDLE hDevice;
			hDevice = CreateFile(WIN32_ROOT_PREFIX, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

			if (hDevice == INVALID_HANDLE_VALUE) {
				printf("Error: CreatFile Failed : %d\n", (int)GetLastError());
				return 0;
			}

			char OutputBuffer[100];
			char InputBuffer[100];
			ULONG bytesReturned;
			StringCbCopy(InputBuffer, sizeof(InputBuffer),
				"This String is from User Application; using METHOD_BUFFERED");

			printf("\nCalling DeviceIoControl METHOD_BUFFERED:\n");

			memset(OutputBuffer, 0, sizeof(OutputBuffer));
			BOOL retCode = DeviceIoControl(hDevice,
				(DWORD)KMS_IOCTL_MOUNT,
				&InputBuffer,
				(DWORD)strlen(InputBuffer) + 1,
				&OutputBuffer,
				sizeof(OutputBuffer),
				&bytesReturned,
				NULL
				);


			if (!retCode)
			{
				printf("Error in DeviceIoControl : %d", (int)GetLastError());
				return 0;

			}
			// 드라이버 통신 종료
			printf("    OutBuffer (%d): %s\n", bytesReturned, OutputBuffer);

			int clusterSize = 0;
			int driveNo = GetLastAvailableDrive();
			printf("사용가능한 드라이버 번호driveNo = %d \n", driveNo);
			retCode = FormatNtfs(driveNo, clusterSize);


			CloseHandle(hDevice);// 드라이버 닫기

		}
		else if ((argc > 3) && ((pTemp[0] == 'W') || (pTemp[0] == 'w'))) {


			HANDLE hDevice;
			char OutputBuffer[100];
			char InputBuffer[100] = "kiminho test";
			ULONG bytesReturned;

			hDevice = CreateFile(WIN32_ROOT_PREFIX, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

			if (hDevice == INVALID_HANDLE_VALUE) {
				printf("Error: writeFile Failed : %d\n", (int)GetLastError());
				return 0;
			}

			BOOL retCode = DeviceIoControl(hDevice,
				(DWORD)KMS_IOCTL_WRITE,
				&InputBuffer,
				(DWORD)strlen(InputBuffer) + 1,
				&OutputBuffer,
				sizeof(OutputBuffer),
				&bytesReturned,
				NULL
				);
			printf("writeFile success  \n");
		}

		else if ((argc > 3) && ((pTemp[0] == 'Z') || (pTemp[0] == 'z'))) {
			HANDLE hDevice;
			char OutputBuffer[100];
			char InputBuffer[100];
			ULONG bytesReturned;

			hDevice = CreateFile(WIN32_ROOT_PREFIX, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

			if (hDevice == INVALID_HANDLE_VALUE) {
				printf("Error: CreatFile Failed : %d\n", (int)GetLastError());
				return 0;
			}

			BOOL retCode = DeviceIoControl(hDevice,
				(DWORD)KMS_IOCTL_READ,
				&InputBuffer,
				(DWORD)strlen(InputBuffer) + 1,
				&OutputBuffer,
				sizeof(OutputBuffer),
				&bytesReturned,
				NULL
				);
			printf("readFile success %s \n", OutputBuffer);
		}



		CloseServiceHandle(schService);
	}
	CloseServiceHandle(schSCManager);

	return 0;
}
