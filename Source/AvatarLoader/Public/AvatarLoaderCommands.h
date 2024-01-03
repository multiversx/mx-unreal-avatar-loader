// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "AvatarLoaderStyle.h"

class FAvatarLoaderCommands : public TCommands<FAvatarLoaderCommands>
{
public:

	FAvatarLoaderCommands()
		: TCommands<FAvatarLoaderCommands>(TEXT("AvatarLoader"), NSLOCTEXT("Contexts", "AvatarLoader", "AvatarLoader Plugin"), NAME_None, FAvatarLoaderStyle::GetStyleSetName())
	{
	}

	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > PluginAction;
};
