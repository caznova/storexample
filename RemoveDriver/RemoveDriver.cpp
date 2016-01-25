// DeleteSvc.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>
#include <windows.h>


int main(int argc, char* argv[])
{
	SC_HANDLE   schSCManager;
	SC_HANDLE   schService;
	SERVICE_STATUS  serviceStatus;

//	//Debug("[KMS]%s:%d Enter...", __FUNCTION__, __LINE__);

	if (argc < 2)
	{

		printf("[사용방법]\n");
		printf("RemoveDriver.EXE [SERVICENAME]\n");
		printf("[SERVICENAME] : 제거할 서비스이름\n");
		return 0;
	}
	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!schSCManager)
	{
		//Debug("[KMS]%s:%d No Service Manager exist...", __FUNCTION__, __LINE__);
		printf("서비스메니져가 없습니다 error hex = %x\n", GetLastError());
		return 0;
	}

	//Debug("[KMS]%s:%d unregister service [%s]", __FUNCTION__, __LINE__, argv[1]);
	schService = OpenService(schSCManager, argv[1], SERVICE_ALL_ACCESS);
	if (schService == NULL)
	{
		//Debug("[KMS]%s:%d No prior service registered...[%s]", __FUNCTION__, __LINE__, argv[1]);
		printf("해당하는 서비스가 등록되어 있지 않습니다 error hex = %x\n", GetLastError());
	}
	else
	{
		ControlService(schService, SERVICE_CONTROL_STOP, &serviceStatus);
		if (serviceStatus.dwCurrentState != 1) {
			//Debug("[KMS]%s:%d SERVICE_CONTROL_STOP but can't stop service...[%s]", __FUNCTION__, __LINE__);
			printf("서비스의 중지를 할 수 없기 때문에 해당하는 요청을 수행할 수 없습니다 error hex = %x\n", GetLastError());
		}
		else
		{
			DeleteService(schService);
			//Debug("[KMS]%s:%d Delete service...", __FUNCTION__, __LINE__);
			printf("서비스를 중지하였습니다. 서비스를 제거하였습니다\n");
		}
		CloseServiceHandle(schService);
		//Debug("[KMS]%s:%d Service handle closed...", __FUNCTION__, __LINE__);
	}
	CloseServiceHandle(schSCManager);

	return 0;
}

