#include "AvatarLoaderFolderHelper.h"
#include "Misc/Paths.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "HAL/PlatformFilemanager.h"

bool FFolderHelper::CreateFolder(const FString &FolderPath)
{
    IPlatformFile &PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

    FString FullPath = FPaths::Combine(FPaths::ProjectContentDir(), FolderPath);

    if (!PlatformFile.DirectoryExists(*FullPath))
    {
        return PlatformFile.CreateDirectoryTree(*FullPath);
    }

    return true;
}

bool FFolderHelper::DoesFolderExist(const FString &FolderPath)
{
    IPlatformFile &PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

    FString FullPath = FPaths::Combine(FPaths::ProjectContentDir(), FolderPath);

    return PlatformFile.DirectoryExists(*FullPath);
}
