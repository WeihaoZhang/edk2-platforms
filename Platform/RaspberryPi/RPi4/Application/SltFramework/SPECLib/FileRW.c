

#include "FileRW.h"
#include <stdarg.h>
// #include <stdio.h>

extern EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *gSimpleFileSystem;  //操作FAT文件系统的句柄
extern EFI_FILE_PROTOCOL *gFileRoot;		//操作文件的句柄
static UINT64 __sflags(const char *mode);
FILE *
fopen(const char *filename, const char *mode) {
    Print(L"filename %a\n",filename);
    EFI_STATUS Status;
    FILE *file;
    CHAR16 format_data[200];
    CHAR16 *dst = format_data;
    const char  *AsciiStr =  filename;
    while (!('\0' == (*AsciiStr))) {
        *(dst++) = (CHAR16)*(AsciiStr++);
    }
    *(dst++) = '\0';
    Print(L"format_data %s\n",format_data);
    Status = OpenFile(&file, format_data, __sflags(mode));
    Print(L"fopen Status %d\n",Status);
    if (Status ==EFI_SUCCESS) {
        return file;
    }
    return NULL;
}

int
fclose(FILE *fp)
{
    if (fp != NULL) {
        fp->Close(fp);
    }
    return 0;
}

int
fflush(FILE *fp)
{
    fp = fp;
    return 0;
}

/*
 * Read at most n-1 characters from the given file.
 * Stop when a newline has been read, or the count runs out.
 * Return first argument, or NULL if no characters were read.
 */
char *
fgets(char *buf, int n, FILE *fp) {
//   ASSERT(buf != NULL);
//   ASSERT(fp != NULL);
  if ((fp == NULL) || (n <= 0)) {        /* sanity check */
    return (NULL);
  }
  UINT64 current_position;
  EFI_STATUS  Status = GetFilePosition(fp,&current_position);
  Print(L"fgets fp [%a]:(%d) %x,n %d\n",__FUNCTION__,__LINE__,fp,n);

  Print(L"GetFilePosition [%a]:(%d) %d\n",__FUNCTION__,__LINE__,current_position);
  Print(L"buf [%a]:(%d) %x\n",__FUNCTION__,__LINE__,buf);
  if (Status != EFI_SUCCESS) {
    Print(L"Status error [%a]:(%d)\n",__FUNCTION__,__LINE__);
  }
  Status = ReadFile(fp, (UINTN *)&n, buf);
  if (Status != EFI_SUCCESS) {
    Print(L"Status error [%a]:(%d)\n",__FUNCTION__,__LINE__);
  }
  n-=1;
  buf[n] = '\0';
  for (int i = 0; i < n; i ++) {
    if (buf[i] != '\n') {
        current_position += 1;
    } else {
        buf[i] = '\0';
        current_position += 1;
        break;
    }
  }
  Print(L"current_position [%a]:(%d) %d\n",__FUNCTION__,__LINE__,current_position);
  if (Status != EFI_SUCCESS) {
    Print(L"Status error [%a]:(%d)\n",__FUNCTION__,__LINE__);
  }
  UINT64 new_position;
  Status = GetFilePosition(fp,&new_position);
//   Print(L"current_position [%a]:(%d) %d",__FUNCTION__,__LINE__,current_position);

  Print(L"ReadFile new_position [%a]:(%d) %d\n",__FUNCTION__,__LINE__,new_position);
  Status = SetFilePosition(fp,current_position);
  if (Status != EFI_SUCCESS) {
    Print(L"Status error [%a]:(%d)\n",__FUNCTION__,__LINE__);
  }
  return NULL;
}
/*
 * Return the (stdio) flags for a given mode.  Store the flags
 * to be passed to an open() syscall through *optr.
 * Return 0 on error.
 */
static UINT64
__sflags(const char *mode)
{
  UINT64 ret;

//   ASSERT(mode != NULL);
    for (; *mode; mode++) {
        switch (*mode++) {

        case 'r': /* open for reading */
        ret = ret | EFI_FILE_MODE_READ;
        break;

        case 'w': /* open for writing */
        ret = ret | EFI_FILE_MODE_WRITE;
        break;

        default:  /* illegal mode */
        break;
        }
    }
    Print(L"__sflags ret %d\n",ret);
    return ret;
}


//Name: OpenFile
//Input: fileHandle,fileName,OpenMode
//Output: if success,file's handle is *fileHandle 
//Descriptor: OpenMode:EFI_FILE_MODE_READ,EFI_FILE_MODE_WRITE,EFI_FILE_MODE_CREATE
//生成各种属性的文件，本函数不提供，需要的话可以修改
EFI_STATUS OpenFile(EFI_FILE_PROTOCOL **fileHandle,CHAR16 * fileName,UINT64 OpenMode)
{
  EFI_STATUS  Status = 0;
  Status = gFileRoot ->Open(
            gFileRoot,     
            fileHandle,
            (CHAR16*)fileName, 
            OpenMode,
            0   //如果OpenMode为Create属性，则此参数有效，参照spec
            );
  
  return Status;
}
//Name: ReadFile
//Input: fileHandle,bufSize,buffer
//Output:read data to the buffer and the length of data is bufSize
//Descriptor:
EFI_STATUS ReadFile(EFI_FILE_PROTOCOL *fileHandle,UINTN *bufSize,VOID *buffer)
{
  EFI_STATUS Status = 0;
 Print(L"ReadFile [%a]:(%d) %x  %x %d\n",__FUNCTION__,__LINE__,fileHandle,buffer,*bufSize);
  Status = fileHandle->Read(fileHandle, bufSize, buffer);
 Print(L"ReadFile ok[%a]:(%d) %x  %x %d\n",__FUNCTION__,__LINE__,fileHandle,buffer,*bufSize);


  return Status;
}

//Name: WriteFile
//Input: fileHandle,bufSize,buffer
//Output:write data to the file,data in buffer and the length of data is bufSize
//Descriptor:
EFI_STATUS WriteFile(EFI_FILE_PROTOCOL *fileHandle,UINTN *bufSize,VOID *buffer)
{
  EFI_STATUS Status = 0;

  Status = fileHandle->Write(fileHandle, bufSize, buffer);

  return Status;
}
//Name: SetFilePosition
//Input: fileHandle,position
//Output: 
//Descriptor: if position is 0xffffffffffffffff,the current position will be set to the end of file.
EFI_STATUS SetFilePosition(EFI_FILE_PROTOCOL *fileHandle,UINT64 position)
{
  EFI_STATUS Status = 0;
  
  Status = fileHandle->SetPosition(fileHandle, position);

  return Status;
}
//Name: GetFilePosition
//Input: fileHandle,position
//Output: store in the var position
//Descriptor: if position is 0xffffffffffffffff,the current position will be set to the end of file.
EFI_STATUS GetFilePosition(EFI_FILE_PROTOCOL *fileHandle,UINT64 *position)
{
  EFI_STATUS Status = 0;
  
  Status = fileHandle->GetPosition(fileHandle, position);

  return Status;
}


int
printf(char const *fmt, ...)
{
  int ret;
  va_list ap;

  va_start(ap, fmt);
    CHAR16 format_data[200];
    CHAR16 *dst = format_data;
    const char  *AsciiStr =  fmt;
    while (!('\0' == (*AsciiStr))) {
        *(dst++) = (CHAR16)*(AsciiStr++);
    }
    *(dst++) = '\0';
  ret = Print(format_data, ap);
  va_end(ap);
  return (ret);
}


int
fprintf(FILE *fp, const char *fmt, ...)
{
  int ret;
  va_list ap;

  if(fp == NULL) {
    return -1;
  }
    CHAR16 format_data[200];
    CHAR16 *dst = format_data;
    const char  *AsciiStr =  fmt;
    while (!('\0' == (*AsciiStr))) {
        *(dst++) = (CHAR16)*(AsciiStr++);
    }
    *(dst++) = '\0';
  va_start(ap, fmt);
//   ret = vfprintf(fp, fmt, ap);
  EFI_STATUS Status = FileHandlePrintLine(fp,format_data,ap);
  va_end(ap);
  ret = Status;
  return (ret);
}