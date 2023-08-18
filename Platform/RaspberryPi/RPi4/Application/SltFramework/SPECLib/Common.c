#include "Common.h"
EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *gSimpleFileSystem;  //����FAT�ļ�ϵͳ�ľ��
EFI_FILE_PROTOCOL *gFileRoot;		//�����ļ��ľ��
//Name: LocateFileIO
//Input: 
//Output: *gFileRoot
//Descriptor:
EFI_STATUS LocateFileRoot(void)
{
	EFI_STATUS  Status = 0;
	
	 Status = gBS->LocateProtocol(
            &gEfiSimpleFileSystemProtocolGuid,
            NULL,
            (VOID**)&gSimpleFileSystem //ʵ�ʳ������ò���������Ԥ��������ӿ� 2019-06-10 19:51:02 luobing
    );
    if (EFI_ERROR(Status)) {
     //δ�ҵ�EFI_SIMPLE_FILE_SYSTEM_PROTOCOL
        return Status;
    }
    Status = gSimpleFileSystem->OpenVolume(gSimpleFileSystem, &gFileRoot);
    Print(L"--ethanzhang gFileRoot %x,Status %d\n",gFileRoot,Status);
    return Status;
}