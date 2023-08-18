/** @file
 *
 *  Copyright 2023 Cix Technology (Shanghai) Co., Ltd. All Rights Reserved. *
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#include <SltCommon.h>

EFI_STATUS
EFIAPI
SltInitLoadOptionInfo(
  OUT  SLT_TESTCASE_INFO *SltIPTestCaseInfo
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *FilePath;
  SLT_TESTCASE_INFO         *IpInfo     = SltIPTestCaseInfo;
  CHAR16                    *FileName   = IpInfo->IPEfiFileName;

  FilePath = FileDevicePath (NULL, FileName);
  if (FilePath == NULL) {
    DEBUG ((DEBUG_ERROR, "Fail to allocate memory for %s file path. Unable to run.\n", FileName));
    return SLT_MALLOC_FAIL;
  }

  ZeroMem (&IpInfo->AuxCoreLoadEfiOption, sizeof (SLT_LOADEFI_OPTION));
  IpInfo->AuxCoreLoadEfiOption.Description = AllocateCopyPool (StrSize (IpInfo->IPName), IpInfo->IPName);
  IpInfo->AuxCoreLoadEfiOption.FilePath    = DuplicateDevicePath (FilePath);
  IpInfo->AuxCoreLoadEfiOption.ParameterData = (VOID *)IpInfo;
  IpInfo->AuxCoreLoadEfiOption.ParameterDataSize = sizeof (SLT_TESTCASE_INFO);

  FreePool (FilePath);

  return SLT_SUCCESS;
}

STATIC
EFI_STATUS
SLTConnectDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePathToConnect
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                Handle;
  EFI_HANDLE                PreviousHandle;
  EFI_DEVICE_PATH_PROTOCOL  *RemainingDevicePath;

  if (DevicePathToConnect == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  PreviousHandle = NULL;
  do {
    RemainingDevicePath = DevicePathToConnect;
    Status              = gBS->LocateDevicePath (&gEfiDevicePathProtocolGuid, &RemainingDevicePath, &Handle);
    if (!EFI_ERROR (Status)) {
      PreviousHandle = Handle;
      Status = gBS->ConnectController (Handle, NULL, RemainingDevicePath, FALSE);
        if (Status == EFI_NOT_FOUND) {
          Status = EFI_SUCCESS;
        }
      }
  } while (!EFI_ERROR (Status) && !IsDevicePathEnd (RemainingDevicePath));

  return Status;
}


/**
  Print the device path info.

  @param DevicePath           The device path need to print.
**/
STATIC
VOID
SltPrintDevicePath (
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  CHAR16  *Str;

  Str = ConvertDevicePathToText (DevicePath, FALSE, FALSE);
  DEBUG ((DEBUG_INFO, "%s", Str));
  if (Str != NULL) {
    FreePool (Str);
  }
}

STATIC
EFI_DEVICE_PATH_PROTOCOL  *
SltExpandFileDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *FilePath
  )
{
  EFI_STATUS                Status;
  UINT32                    Index;
  UINTN                     HandleCount;
  EFI_HANDLE                *Handles;
  EFI_BLOCK_IO_PROTOCOL     *BlockIo;
  UINT32                    MediaType;
  EFI_DEVICE_PATH_PROTOCOL  *FullPath;

  Status = gBS->LocateHandleBuffer (
                        ByProtocol,
                        &gEfiSimpleFileSystemProtocolGuid,
                        NULL,
                        &HandleCount,
                        &Handles
                      );
  if (EFI_ERROR (Status)) {
    HandleCount = 0;
    Handles     = NULL;
  }

  //
  // Enumerate all removable media devices followed by all fixed media devices,
  //   followed by media devices which don't layer on block io.
  //
  for (MediaType = 0; MediaType < 3; MediaType++) {
    for (Index = 0; Index < HandleCount; Index++) {
      Status = gBS->HandleProtocol (Handles[Index], &gEfiBlockIoProtocolGuid, (VOID *)&BlockIo);
      if (EFI_ERROR (Status)) {
        BlockIo = NULL;
      }

      if (((MediaType == 0) && (BlockIo != NULL) && BlockIo->Media->RemovableMedia) ||
          ((MediaType == 1) && (BlockIo != NULL) && !BlockIo->Media->RemovableMedia) ||
          ((MediaType == 2) && (BlockIo == NULL))
          )
      {
        FullPath = AppendDevicePath (DevicePathFromHandle (Handles[Index]), FilePath);
        break;
      }
    }

    if (FullPath != NULL) {
      break;
    }
  }

  if (Handles != NULL) {
    FreePool (Handles);
  }

  return FullPath;
}

STATIC
VOID *
SltGetFullFilePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *FilePath,
  OUT EFI_DEVICE_PATH_PROTOCOL  **FullPath,
  OUT UINT32                    *FileSize
  )
{
  VOID                      *FileBuffer;
  EFI_DEVICE_PATH_PROTOCOL  *CurFullPath;
  UINTN                     LocalFileSize;
  UINT32                    AuthenticationStatus;

  LocalFileSize = 0;
  FileBuffer    = NULL;
  CurFullPath   = *FullPath;

  if ((DevicePathType (FilePath) == MEDIA_DEVICE_PATH) &&
      (DevicePathSubType (FilePath) == MEDIA_FILEPATH_DP))
  {
    CurFullPath = SltExpandFileDevicePath (FilePath);
  }

  FileBuffer = GetFileBufferByFilePath (
                          TRUE,
                          CurFullPath,
                          &LocalFileSize,
                          &AuthenticationStatus
                        );

  DEBUG ((DEBUG_INFO, "\n[SLT] Expand "));
  SltPrintDevicePath (FilePath);
  DEBUG ((DEBUG_INFO, " -> "));
  SltPrintDevicePath (CurFullPath);
  DEBUG ((DEBUG_INFO, "\n\n"));

  *FullPath = CurFullPath;
  *FileSize = LocalFileSize;
  return FileBuffer;
}

EFI_STATUS
EFIAPI
SltStartEfi (
  OUT  SLT_TESTCASE_INFO *SltIPTestCaseInfo
  )
{
  EFI_STATUS                 Status         = SLT_SUCCESS;
  EFI_HANDLE                 ImageHandle    = NULL;
  UINT32                     FileSize       = 0;
  SLT_LOADEFI_OPTION         *LoadEfiOption = &SltIPTestCaseInfo->AuxCoreLoadEfiOption;
  EFI_DEVICE_PATH_PROTOCOL   *CurFullPath   = NULL;
  EFI_LOADED_IMAGE_PROTOCOL  *ImageInfo;
  VOID                       *FileBuffer;

  if (LoadEfiOption == NULL) {
    return SLT_INVALID_PARAMETER;
  }

  if (LoadEfiOption->FilePath == NULL) {
    return SLT_INVALID_PARAMETER;
  }

  DEBUG_CODE_BEGIN ();
  if (LoadEfiOption->Description == NULL) {
    DEBUG ((DEBUG_INFO, "[SLT] %a:%d: unknow IP Description\n", __FUNCTION__, __LINE__));
  } else {
    DEBUG ((DEBUG_INFO, "[SLT] %a:%d: %s\n", __FUNCTION__, __LINE__, LoadEfiOption->Description));
  }
  DEBUG_CODE_END ();

  SLTConnectDevicePath (LoadEfiOption->FilePath);

  FileBuffer  = SltGetFullFilePath (LoadEfiOption->FilePath, &CurFullPath, &FileSize);
  if (FileBuffer == NULL) {
    return SLT_FILEPATH_FAIL;
  }
  DEBUG ((DEBUG_INFO, "[SLT] %a --- %d FileSize = 0x%x\n", __FUNCTION__, __LINE__, FileSize));

  Status = gBS->LoadImage (
                  FALSE,
                  gImageHandle,
                  CurFullPath,
                  FileBuffer,
                  FileSize,
                  &ImageHandle
                  );
  FreePool (FileBuffer);

  if (EFI_ERROR (Status)) {
    //
    // With EFI_SECURITY_VIOLATION retval, the Image was loaded and an ImageHandle was created
    // with a valid EFI_LOADED_IMAGE_PROTOCOL, but the image can not be started right now.
    // If the caller doesn't have the option to defer the execution of an image, we should
    // unload image for the EFI_SECURITY_VIOLATION to avoid resource leak.
    //
    if (Status == EFI_SECURITY_VIOLATION) {
      gBS->UnloadImage (ImageHandle);
    }
  } else {
    Status = gBS->HandleProtocol (ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID **)&ImageInfo);
    ASSERT_EFI_ERROR (Status);

    ImageInfo->LoadOptionsSize = LoadEfiOption->ParameterDataSize;
    ImageInfo->LoadOptions     = LoadEfiOption->ParameterData;

    DEBUG ((DEBUG_INFO, "[SLT] %a --- %d Start Image.....\n", __FUNCTION__, __LINE__));
    Status = gBS->StartImage (
                              ImageHandle,
                              NULL,
                              NULL
                            );
    LoadEfiOption->Status = Status;
  }

  return Status;
}
