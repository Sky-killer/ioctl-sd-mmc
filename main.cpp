#include <stdio.h>
#include <Windows.h>
#include <SetupAPI.h>
#include <wchar.h>
#include <sffdisk.h>
#include <sddef.h>
#include <strsafe.h>
#pragma comment (lib,"setupapi.lib")

int read_extcsd(HANDLE hDevice);

void ErrorExit(LPTSTR lpszFunction)
{
	// Retrieve the system error message for the last-error code

	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	// Display the error message and exit the process

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"),
		lpszFunction, dw, lpMsgBuf);
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
	ExitProcess(dw);
}

int main()
{
	HANDLE hDevice = NULL;
	//HDEVINFO hDevInfo = NULL;
	BOOL bResult = false;
	hDevice = CreateFileA(
		"\\\\.\\globalroot\\Device\\00000031",
		GENERIC_ALL| STANDARD_RIGHTS_ALL| GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		//0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if (hDevice == INVALID_HANDLE_VALUE)
	{
		printf("Error: CreateFile. code = %u\n", GetLastError());
		return -1;
	}
	else printf("OK --- CreateFile.\n");

	wchar_t function[MAX_PATH] = L"read_extcsd";
	if (!read_extcsd(hDevice))
		ErrorExit(function);

	//SFFDISK_DEVICE_COMMAND_DATA* CMD;

	////=========================================CMD
	//// Declare Variables
	//int buffSize = 0;
	//int nSizeofCmd = 0;
	//SDCMD_DESCRIPTOR MMC_CMD_DATA = { 0 };
	//LPDWORD dwBytesReturned = 0;
	//BYTE outputData[512] = { 0 };
	//BYTE inputData[512] = { 0 };
	//SFFDISK_DEVICE_PASSWORD_DATA* pCmdPW;


	//// Build Structure SFFDISK_DEVICE_COMMAND_DATA for lpInBuffer in DeviceIOControl
	//buffSize = 512; // One data block
	//nSizeofCmd = sizeof(SFFDISK_DEVICE_COMMAND_DATA) + sizeof(SDCMD_DESCRIPTOR) + buffSize;

	//CMD = (SFFDISK_DEVICE_COMMAND_DATA*) new BYTE[nSizeofCmd];
	//memset(CMD, 0, nSizeofCmd);
	//CMD->HeaderSize = sizeof(SFFDISK_DEVICE_COMMAND_DATA);
	//CMD->Command = SFFDISK_DC_DEVICE_COMMAND;
	////pCmd->Command = SFFDISK_DC_GET_VERSION;
	////pCmd->Command = SFFDISK_DC_RESERVED1;
	////pCmd->Command = SFFDISK_DC_RESERVED1;

	//CMD->ProtocolArgumentSize = sizeof(SDCMD_DESCRIPTOR);

	/////Build sdCmdDesriptor, which is located as data member in SFFDISK_DEVICE_COMMAND_DATA structure
	//memset(&MMC_CMD_DATA, 0, sizeof(MMC_CMD_DATA));
	//MMC_CMD_DATA.Cmd = 8;//MMC_SEND_EXT_CSD
	//MMC_CMD_DATA.CmdClass = SDCC_STANDARD;//??
	//MMC_CMD_DATA.TransferDirection = SDTD_READ;
	//MMC_CMD_DATA.TransferType = SDTT_SINGLE_BLOCK;
	//MMC_CMD_DATA.ResponseType = SDRT_1;//?SDRT_2
	//memcpy((BYTE*)(&(CMD->Data[0])), &MMC_CMD_DATA, sizeof(SDCMD_DESCRIPTOR));

	//CMD->DeviceDataBufferSize = buffSize;
	//CMD->Information = 0;	// Argument bit[0] = 0 for write

	//memset(inputData, 0, sizeof(inputData));
	//memcpy((BYTE*)(&(CMD->Data[0]) + sizeof(SDCMD_DESCRIPTOR)), &inputData, sizeof(inputData));
	////=========================================================================
	//dwBytesReturned = 0;
	//UINT8 ext_csd[512];
	//bResult = DeviceIoControl(hDevice, IOCTL_SFFDISK_DEVICE_COMMAND, CMD, nSizeofCmd, ext_csd, sizeof(ext_csd), dwBytesReturned, NULL);
	//if (bResult == 0)
	//{
	//	printf("Error: DeviceIoControl. code = %u\n", GetLastError());
	//}
	//else {
	//	//read_csd(ext_csd);
	//	printf("Yes\n");
	//}
	//// Free Memory 
	//free(CMD);

	CloseHandle(hDevice);
	//SetupDiDestroyDeviceInfoList(hDevInfo);

	system("pause");
	return 0;
}

int read_extcsd(HANDLE hDevice) {
	BOOL bRet = FALSE;

	/*
	 * DeviceIoControl Parameter initialization
	 */
	int sizeOfInBuffer = sizeof(SFFDISK_DEVICE_COMMAND_DATA) + sizeof(SDCMD_DESCRIPTOR) + 16;
	SFFDISK_DEVICE_COMMAND_DATA* InBuffer = (SFFDISK_DEVICE_COMMAND_DATA *)malloc(sizeOfInBuffer); //DeviceIoContrl的InBuffer用SFFDISK_DEVICE_COMMAND_DATA填充
	UINT8 OutBuffer[512]; //DeviceIoContrl的输出
	DWORD ByTesReturned = 0;

	/*
	 * InBuffer（SFFDISK_DEVICE_COMMAND_DATA） Parameter initialization
	 */
	InBuffer->HeaderSize = sizeof(SFFDISK_DEVICE_COMMAND_DATA);
	InBuffer->Command = SFFDISK_DC_DEVICE_COMMAND;
	InBuffer->ProtocolArgumentSize = sizeof(SDCMD_DESCRIPTOR);

	/*
	 * InBuffer->Data（SDCMD_DESCRIPTOR） Parameter initialization
	 */
	SDCMD_DESCRIPTOR CMD8;
	CMD8.Cmd = 8;//CMD8
	CMD8.CmdClass = SDCC_STANDARD; // Indicates an SD card command from the standard command set. This command set includes command codes 0 to 63.
	CMD8.TransferDirection = SDTD_READ;
	CMD8.TransferType = SDTT_SINGLE_BLOCK;
	CMD8.ResponseType = SDRT_1;// find in JESD84-B51
	INT sizeofCMD8 = sizeof(CMD8);

	memcpy(&(InBuffer->Data[0]), &CMD8, sizeofCMD8); //save in Data

	InBuffer->DeviceDataBufferSize = sizeofCMD8;

	//-------------------------Note: Release Buffer----------------------------


	/*
	 * DeviceIoControl
	 */
//#define IOCTL_SFFDISK_DEVICE_COMMAND2 \
//            CTL_CODE( FILE_DEVICE_DISK, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
	memset(&OutBuffer, 1, sizeof(OutBuffer));
	bRet = DeviceIoControl(hDevice, IOCTL_SFFDISK_DEVICE_COMMAND, &InBuffer, sizeOfInBuffer, &OutBuffer, sizeof(OutBuffer), &ByTesReturned, NULL);
	if (bRet == 0)
	{
		return 0;
	}
	else {
		printf("DeviceIoControl CMD8 OK\n");
	}
	for (int i = 0; i < 512; ++i) {
		printf("%d:0x%06x\n",i, OutBuffer[i]);
	}



	return 1;
	//int nSizeOfCmd = sizeof(SFFDISK_DEVICE_COMMAND_DATA) + sizeof(SDCMD_DESCRIPTOR) + 16;
	//SFFDISK_DEVICE_COMMAND_DATA* pCmd = (SFFDISK_DEVICE_COMMAND_DATA*) new BYTE[nSizeOfCmd];
	//memset(pCmd, 0, nSizeOfCmd);
	//pCmd->HeaderSize = sizeof(SFFDISK_DEVICE_COMMAND_DATA);
	//pCmd->Command = SFFDISK_DC_DEVICE_COMMAND;
	//pCmd->ProtocolArgumentSize = sizeof(SDCMD_DESCRIPTOR);
	//pCmd->DeviceDataBufferSize = 16;
	//ULONG_PTR info = 0;
	//pCmd->Information = info;

	/////Command protocol
	//SDCMD_DESCRIPTOR sdCmdDescriptor = { 0 };
	//sdCmdDescriptor.Cmd = 10; //SDCMD_IO_RW_DIRECT;
	//sdCmdDescriptor.CmdClass = SDCC_STANDARD;
	//sdCmdDescriptor.TransferDirection = SDTD_READ;
	//sdCmdDescriptor.TransferType = SDTT_CMD_ONLY;
	//sdCmdDescriptor.ResponseType = SDRT_2;
	//memcpy((BYTE*)(&(pCmd->Data[0])), &sdCmdDescriptor, sizeof(SDCMD_DESCRIPTOR));
	///*
	// * DeviceIoControl
	// */
	//DWORD dwBytesReturned = 0;
	//BOOL bRet = DeviceIoControl(hDevice, IOCTL_SFFDISK_DEVICE_COMMAND, pCmd, nSizeOfCmd, pCmd, nSizeOfCmd, &dwBytesReturned, NULL);

	//if (!bRet)
	//{
	//	return 0;
	//}
	//else {
	//	printf("DeviceIoControl CMD8 OK\n");
	//}
	//return 1;
}