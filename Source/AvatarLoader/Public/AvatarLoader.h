// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "SWebBrowser.h"

class FToolBarBuilder;
class FMenuBuilder;

class FAvatarLoaderModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void PluginButtonClicked();

	void HandleUrlChanged(const FText &NewUrl);

	void HandleTokenReceived(const FString &Token, const FString &Environment);

	void GetSignedUrl();

	void DownloadFBXFile(const FString SignedUrl);

	void ImportFBXFile(const FString &FilePath, const FString &FolderPath);

	void HandleError(const FString &ErrorMessage);

	FString ReceivedToken;

	FString ReceivedEnvironment;

	TSharedPtr<SWebBrowser> WebBrowserWidget;

	TSharedPtr<SWindow> Window;

private:
	void RegisterMenus();

private:
	TSharedPtr<class FUICommandList> PluginCommands;
};
