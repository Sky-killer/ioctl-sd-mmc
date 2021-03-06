------
> > **目录**
>
> [TOC]
>

------

### 之前问题的解决

#### 环境问题

```C
/* 1. WinBase.h 头文件 已经过时 用windows.h代替
 * 2.SetupApi.h 及其头文件除了头文件之外还需要链接静态库
 */
 #pragma comment (lib,"setupapi.lib")
 /*
  * 3.sffdisk.h sddef.h 头文件 在使用VS Install安装的SDK中有时会缺失，从官网下载SDK并添加到项目路径中即可使用
  */
```

#### GUID获取失败

​	按照之前利用Set*系列函数获取句柄，在第一步得到GUID类设备集后，第二部获取详细信息后失败，后来通过直接获取硬件物理名称和地址绕过GUID

-----

### **获取devpath方法与格式：**

#### 方法

> 通过设备管理器查询物理名称：
>
> ![image-20210111145904333](.\pic\image-20210111145904333.png)

#### 格式

> ​	要创建或打开的文件或设备的名称,ANSI版本中，名称限制为**MAX_PATH**个字符。若要将此限制扩展为32,767个宽字符，请调用该函数的Unicode版本，并在路径前添加“ \\？\”。
>
> ​																																	——MSDN
>
> ​	实际在代码中，需要添加转义字符，所以其格式为：
>
> ```C
> wchar_t devPath[MAX_PATH] = L"\\\\.\\Device\\00000045";
> ```
>
> ​	但实际编译运行会有报错：
>
> ```C
> Error: CreateFile. code = 3  //GetLastError() 返回的错误代码，路径无效
> /* 另外ERROR 5是缺少权限报错 所以访问驱动需要使用管理员权限运行*/
> ```
>
> ​	最后，发现另外一种地址格式是有效的，并且能够发送相关命令：
>
> ```C
> wchar_t devPath[MAX_PATH] = L"\\\\.\\globalroot\\Device\\00000045" ;
> ```

-----

### **Windows中SD/MMC命令的发送格式**

​	既然能够直接找到设备的IpFilename,那么就可以调用CreateFile()得到设备句柄，用DeviceIoControl发送命令为IOCTL_SFFDISK_DEVICE_COMMAND的IO控制命令，并把实际的mmc_CMD按照SDCMD_DESCRIPTOR(具体命令结构)和SFFDISK_DEVICE_COMMAND_DATA（InBuffer发送的格式要求）填充到DeviceIoControl的InBuffer区，最后发送，如果命令有输出，输出会缓存在DeviceIoControl的IpOutBuffer中。

#### 参考命令格式

> [**IOCTL_SFFDISK_DEVICE_COMMAND**](https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/sffdisk/ni-sffdisk-ioctl_sffdisk_device_command)
>
> (下图指示了通过IOCTL_SFFDISK_DEVICE_COMMAND请求提交的数据的布局)
>
> ![image-20210111214940048](..\pic\image-20210111214940048.png)
>
>  
>
> [**SFFDISK_DEVICE_COMMAND_DATA**](https://docs.microsoft.com/en-us/previous-versions/windows/hardware/drivers/ff538133(v=vs.85))
>
> [SFFDISK_DCMD](https://docs.microsoft.com/zh-cn/windows-hardware/drivers/ddi/sffdisk/ne-sffdisk-sffdisk_dcmd?redirectedfrom=MSDN)
>
> [**SDCMD_DESCRIPTOR**](https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/sddef/ns-sddef-_sdcmd_descriptor)

#### 实现代码（以获取ext_csd寄存器为例）

```C
int read_extcsd(HANDLE hDevice) {
	BOOL bRet = FALSE;
	
	/*
	 * DeviceIoControl Parameter initialization
	 */
	SFFDISK_DEVICE_COMMAND_DATA InBuffer; //DeviceIoContrl的InBuffer用SFFDISK_DEVICE_COMMAND_DATA填充
	DWORD sizeOfInBuffer = 0;
	UINT8 OutBuffer[512]; //DeviceIoContrl的输出
	LPDWORD ByTesReturned = 0;

	/*
	 * InBuffer（SFFDISK_DEVICE_COMMAND_DATA） Parameter initialization
	 */
	InBuffer.HeaderSize = sizeof(SFFDISK_DEVICE_COMMAND_DATA);
	InBuffer.Command = SFFDISK_DC_DEVICE_COMMAND;
	InBuffer.ProtocolArgumentSize = sizeof(SDCMD_DESCRIPTOR);

    /*
	 * InBuffer->Data（SDCMD_DESCRIPTOR） Parameter initialization
	 */
	SDCMD_DESCRIPTOR CMD8;
	CMD8.Cmd = 8;//CMD8
	CMD8.CmdClass = SDCC_STANDARD; // Indicates an SD card command from the standard command set. This command set includes command codes 0 to 63.
	CMD8.TransferDirection = SDTD_READ;
	CMD8.TransferType = SDTT_SINGLE_BLOCK;
	CMD8.ResponseType = SDRT_1;// find in JESD84-B51
	DWORD sizeofCMD8 = sizeof(CMD8);

	memcpy(InBuffer.Data, &CMD8, sizeofCMD8); //save in Data
	
	InBuffer.DeviceDataBufferSize = sizeofCMD8 ;

	sizeOfInBuffer = sizeof(InBuffer) + sizeof(sizeofCMD8) + 512;
    
    /*
	 * DeviceIoControl
	 */
	bRet = DeviceIoControl(hDevice, IOCTL_SFFDISK_DEVICE_COMMAND, &InBuffer, sizeOfInBuffer, OutBuffer, sizeof(OutBuffer), ByTesReturned, NULL);
	if(bRet == 0)
	{
		return 0;
	}
	else {
		printf("DeviceIoControl CMD8 OK\n");
	}
	return 1;
}
```

### 目前的问题

#### ERROR:拒绝访问

![error5](..\pic\error5.PNG)

#### DeviceIoctl运行IOCTL_DISK_GET_DRIVE_GEOMETRY

![iotest1](.\pic\iotest1.JPG)

在CreateFile中权限已经设置为： GENERIC_READ | GENERIC_WRITE

