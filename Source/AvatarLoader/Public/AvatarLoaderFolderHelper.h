#pragma once

#include "CoreMinimal.h"

class FFolderHelper
{
public:
    static bool CreateFolder(const FString &FolderPath);
    static bool DoesFolderExist(const FString &FolderPath);
};
