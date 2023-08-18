#ifndef _COMMON_H
#define _COMMON_H
#include  <Uefi.h>
#include  <Library/UefiLib.h>
#include  <Protocol/SimpleFileSystem.h>
#include  <Library/BaseLib.h>
#include  <Library/UefiBootServicesTableLib.h>
typedef __SIZE_TYPE__ size_t;

EFI_STATUS LocateFileRoot(void);
char *
strcpy(char * __restrict s1, const char * __restrict s2);
void *
memset(void *s, int c, size_t n);
void *
memcpy(void * __restrict s1, const void * __restrict s2, size_t n);
#endif