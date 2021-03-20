## [CPU运行级别](https://blog.csdn.net/tian5753/article/details/80887470)

>  [Intel](http://baike.baidu.com/subview/2396/2396.htm)的CPU将特权级别分为4个级别：RING0,RING1,RING2,RING3。Windows只使用其中的两个级别RING0和RING3，RING0只给操作系统用，RING3谁都能用。如果普通应用程序企图执行RING0指令，则Windows会显示“非法指令”错误信息。

![image-20201228000918828](.\pic\image-20201228000918828.png)

​		操作系统（内核）的代码运行在最高运行级别ring0上，可以使用特权指令，控制中断、修改页表、访问设备等等。 应用程序的代码运行在最低运行级别上ring3上，不能做受控操作。如果要做，比如要访问磁盘，写文件，那就要通过执行系统调用（函数），执行系统调用的时候，CPU的运行级别会发生从ring3到ring0的切换，并跳转到系统调用对应的内核代码位置执行，这样内核就为你完成了设备访问，完成之后再从ring0返回ring3。这个过程也称作用户态和内核态的切换。  

## WDF

> WDM（Windows Driver Model）: Windows 2000/XP/2003下，须采用WDM驱动模型开发windows驱动，其开发包为：DDK。
>
> 但WDM开发难度较大，因此微软在WDM基础上进行改进，形成WDF(Windows Driver Foundation）框架，提供了面向对象和事件驱动的驱动程序开发框架，开发包为WDK。
>
> WDF包含两种类型，内核级(ring 0)：KMDF，.sys文件；用户级(ring 3)：UMDF，.dll文件。
>
> ![image-20201227221755006](.\pic\image-20201227221755006.png)
>
> WDF开发参考：
>
> Developing Drivers with the Windows Driver Foundation （Microsoft官方开发手册）
>
> [Windows 10 中的 WDF 驱动程序]

## 应用程序与驱动程序通信（r0与r3通信）流程

> 1. 获取设备句柄[CreateFile函数](https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea)  (fileapi.h)
>
>    ```
>    HANDLE CreateFile(
>        LPCTSTR lpFileName,                         // 文件名/设备路径 设备的名称
>        DWORD dwDesiredAccess,                      // 访问方式 READ WRITE
>        DWORD dwShareMode,                          // 共享方式
>        LPSECURITY_ATTRIBUTES lpSecurityAttributes, // 安全描述符指针
>        DWORD dwCreationDisposition,                // 创建方式
>        DWORD dwFlagsAndAttributes,                 // 文件属性及标志
>        HANDLE hTemplateFile                        // 模板文件的句柄
>    );
>    ```
>
>    Ring3层的CreateFile函数获取了设备句柄后，将使用DeviceIoControl函数向指定的设备驱动发送一个IO控制码，驱动程序通过这个控制码来完成特定的工作。
>
> 2. 发送IO控制码[DeviceIoControl](https://docs.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-deviceiocontrol)  (ioapiset.h)
>
>    ```
>    BOOL WINAPI DeviceIoControl(
>      _In_         HANDLE hDevice,       //CreateFile函数打开的设备句柄
>      _In_         DWORD dwIoControlCode,//自定义的控制码
>      _In_opt_     LPVOID lpInBuffer,    //输入缓冲区
>      _In_         DWORD nInBufferSize,  //输入缓冲区的大小
>      _Out_opt_    LPVOID lpOutBuffer,   //输出缓冲区
>      _In_         DWORD nOutBufferSize, //输出缓冲区的大小
>      _Out_opt_    LPDWORD lpBytesReturned, //实际返回的字节数，对应驱动程序中pIrp->IoStatus.Information。
>      _Inout_opt_  LPOVERLAPPED lpOverlapped //用于异步操作
>    );
>    ```
>
>    
>
> 3. 总结
>
>    ​      驱动程序和应用程序自定义好IO控制码 ：[IOCTL_CODE](https://docs.microsoft.com/en-us/previous-versions/windows/embedded/ms902086(v=msdn.10))
>
>    ​      驱动程序定义驱动设备名，符号链接名， 将符号链接名与设备对象名称关联 ，等待IO控制码（IoCreateDevice，IoCreateSymbolicLink）
>
>    ​      应用程序由符号链接名通过CreateFile函数获取到设备句柄DeviceHandle，再用DeviceIoControl通过这个设备句柄发送控制码给派遣函数。
>
>    

## 完整流程

> 1. 通过设备GUID获取实例路径：[SetUpDi系列函数 ](https://docs.microsoft.com/en-us/windows/win32/api/setupapi/nf-setupapi-setupdigetclassdevsw)(setupapi.h)
>
> 2. ![image-20201227235236057](.\pic\image-20201227235236057.png)
>
>    SetupDiGetClassDevs():返回GUID所指设备信息集的指针
>
>    >    The **SetupDiGetClassDevs** function returns a handle to a [device information set](https://docs.microsoft.com/en-us/windows-hardware/drivers/install/device-information-sets) that contains requested device information elements for a local computer.                           ——MSDN
>
>    ```
>    WINSETUPAPI HDEVINFO SetupDiGetClassDevs(
>      const GUID *ClassGuid,   //GUID
>      PCSTR      Enumerator,   
>      HWND       hwndParent,
>      DWORD      Flags 
>    );
>    ```
>
>    [SetupDiEnumDeviceInterfaces()](https://docs.microsoft.com/zh-cn/windows/win32/api/setupapi/nf-setupapi-setupdienumdeviceinterfaces?redirectedfrom=MSDN):返回SetupDiGetClassDevs()查询的指针的信息组
>
>    ```
>    WINSETUPAPI BOOL SetupDiEnumDeviceInterfaces(
>      HDEVINFO                  DeviceInfoSet,  //通常是SetupDiGetClassDevs返回的那个指针
>      PSP_DEVINFO_DATA          DeviceInfoData,
>      const GUID                *InterfaceClassGuid, //GUID
>      DWORD                     MemberIndex,  //设备序号
>      PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData
>    );
>    ```
>
>    [SetupDiGetInterfaceDeviceDetail](https://docs.microsoft.com/en-us/windows/win32/api/setupapi/nf-setupapi-setupdigetdeviceinterfacedetaila):返回一个设备接口的详细信息,包含可传递给CreateFile()的设备路径
>
>    ![image-20201227235308584](pic\image-20201227235308584.png)
>
>    ```
>    WINSETUPAPI BOOL SetupDiGetDeviceInterfaceDetailA(
>      HDEVINFO                           DeviceInfoSet,   //句柄
>      PSP_DEVICE_INTERFACE_DATA          DeviceInterfaceData,
>      PSP_DEVICE_INTERFACE_DETAIL_DATA_A DeviceInterfaceDetailData,  //接口详细信息，可从这里获取设备路径
>      DWORD                              DeviceInterfaceDetailDataSize,
>      PDWORD                             RequiredSize,
>      PSP_DEVINFO_DATA                   DeviceInfoData
>    );
>    ```
>
> 3. 通过实例路径打开设备，获取句柄（Handle）
>
> 4. 通过句柄访问设备，发送控制码
>
> 5. 关闭句柄集，结束进程

## [发送command命令](https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/sffdisk/ni-sffdisk-ioctl_sffdisk_device_command?redirectedfrom=MSDN)

> 命令结构：SFFDISK_DEVICE_COMMAND_DATA
>
> ```
> typedef struct _SFFDISK_DEVICE_COMMAND_DATA {
> USHORT       HeaderSize;
> USHORT       Reserved;
> SFFDISK_DCMD Command;
> USHORT       ProtocolArgumentSize;
> ULONG        DeviceDataBufferSize;
> ULONG_PTR    Information;
> UCHAR        Data[];
> } SFFDISK_DEVICE_COMMAND_DATA, *PSFFDISK_DEVICE_COMMAND_DATA;
> ```
>
> 执行命令：IOCTL_SFFDISK_DEVICE_COMMAND
>
> 调用DeviceIoControl
>
> ```
> bRet = DeviceIoControl (
>  (HANDLE)  hDevice, 
>  (DWORD)  dwIoControlCode, //IOCTL_SFFDISK_DEVICE_COMMAND
>  (PUCHAR)  lpInBuffer,   //SFFDISK_DEVICE_COMMAND_DATA
>  (DWORD)  nInBufferSize, //sizeof（SFFDISK_DEVICE_COMMAND_DATA）...
>  (PUCHAR)  lpOutBuffer,
>  (DWORD)  nOutBufferSize, 
>  (LPDWORD)  lpBytesReturned,
>  (LPOVERLAPPED)  lpOverlapped 
> );
> ```
>
> * 命令队列结构：SFFDISK_QUERY_DEVICE_PROTOCOL_DATA
> * [SD卡规范](https://docs.microsoft.com/en-us/windows-hardware/drivers/sd/)
> * [下载此文件](IO.zip)

https://social.msdn.microsoft.com/Forums/windowsdesktop/en-US/7baa5f07-90cd-4b91-90d4-f0655897479d/secure-digital-card-with-ioctlsffdiskdevicecommand-cmd56?forum=windowsgeneraldevelopmentissues