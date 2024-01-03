// Copyright Epic Games, Inc. All Rights Reserved.

#include "AvatarLoaderCommands.h"

#define LOCTEXT_NAMESPACE "FAvatarLoaderModule"

void FAvatarLoaderCommands::RegisterCommands()
{
	UI_COMMAND(PluginAction, "AvatarLoader", "Execute AvatarLoader action", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
