// Copyright Epic Games, Inc. All Rights Reserved.

#include "AvatarLoader.h"
#include "AvatarLoaderStyle.h"
#include "AvatarLoaderCommands.h"
#include "AvatarLoaderFolderHelper.h"
#include "AvatarLoaderTexts.h"
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"
#include "SWebBrowser.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/FileHelper.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Factories/FbxImportUI.h"
#include "Factories/FbxFactory.h"
#include "AssetImportTask.h"
#include "AssetToolsModule.h"
#include "Json.h"
#include "JsonUtilities.h"

static const FName AvatarLoaderTabName(AvatarLoaderTexts::PluginTabName);

#define LOCTEXT_NAMESPACE "FAvatarLoaderModule"

void FAvatarLoaderModule::StartupModule()
{
	FAvatarLoaderStyle::Initialize();

	FAvatarLoaderStyle::ReloadTextures();

	FAvatarLoaderCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FAvatarLoaderCommands::Get().PluginAction,
		FExecuteAction::CreateRaw(this, &FAvatarLoaderModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FAvatarLoaderModule::RegisterMenus));
}

void FAvatarLoaderModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FAvatarLoaderStyle::Shutdown();

	FAvatarLoaderCommands::Unregister();
}

void FAvatarLoaderModule::PluginButtonClicked()
{
	ReceivedToken = FString();

	Window = SNew(SWindow)
				 .Title(AvatarLoaderTexts::WindowTitle)
				 .ClientSize(FVector2D(1000, 800));

	WebBrowserWidget = SNew(SWebBrowser)
						   .InitialURL(AvatarLoaderTexts::InitialUrl)
						   .ShowControls(true)
						   .OnUrlChanged_Lambda([this](const FText &NewUrl)
												{ HandleUrlChanged(NewUrl); });

	Window->SetContent(WebBrowserWidget.ToSharedRef());

	FSlateApplication::Get().AddWindow(Window.ToSharedRef());
}

void FAvatarLoaderModule::HandleUrlChanged(const FText &NewUrl)
{
	FString UrlString = NewUrl.ToString();

	if (!UrlString.Contains(FString::Printf(TEXT("%s/home/"), *AvatarLoaderTexts::InitialUrl))) 
	{
		return;
	}

	FString TokenPrefix = FString::Printf(TEXT("%s/home/"), *AvatarLoaderTexts::InitialUrl);
	int32 TokenIndex = UrlString.Find(TokenPrefix, ESearchCase::CaseSensitive, ESearchDir::FromStart);
	if (TokenIndex != INDEX_NONE)
	{
		FString RightPart = UrlString.Mid(TokenIndex + TokenPrefix.Len());

		int32 EnvTokenSeparatorIndex;
		if (RightPart.FindChar(TCHAR('/'), EnvTokenSeparatorIndex))
		{
			FString Environment = RightPart.Left(EnvTokenSeparatorIndex);

			FString Token = RightPart.Mid(EnvTokenSeparatorIndex + 1);

			HandleTokenReceived(Token, Environment);
		}
	}
}

void FAvatarLoaderModule::HandleTokenReceived(const FString &Token, const FString &Environment)
{
	ReceivedToken = Token;

	ReceivedEnvironment = Environment;

	GetSignedUrl();
}

void FAvatarLoaderModule::GetSignedUrl()
{
	if (ReceivedToken.IsEmpty())
	{
		HandleError(AvatarLoaderTexts::InvalidTokenError);
		return;
	}

	if (ReceivedEnvironment.IsEmpty())
	{
		HandleError(AvatarLoaderTexts::InvalidEnvironmentError);
		return;
	}

	FString Url = FString::Printf(TEXT("https://%s-avatars-api.xportal.com/api/avatars/avatar_3d/default/"), *ReceivedEnvironment);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb(TEXT("GET"));
	HttpRequest->SetURL(Url);

	HttpRequest->SetHeader(TEXT("Authorization"), ReceivedToken);
	HttpRequest->SetHeader(TEXT("Format"), TEXT("FBX"));

	HttpRequest->OnProcessRequestComplete().BindLambda([this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
													   {
        if (bWasSuccessful && Response.IsValid())
        {
            if (Response->GetResponseCode() == EHttpResponseCodes::Ok)
            {
                TSharedPtr<FJsonObject> JsonObject;
                TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
                if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
                {
                    FString SignedUrl = JsonObject->GetStringField("signed_url");
					if (SignedUrl.IsEmpty()) {
						HandleError(AvatarLoaderTexts::InvalidSignedUrlError);

						return;
					}

                    DownloadFBXFile(SignedUrl);
                }
				else
				{
					HandleError(AvatarLoaderTexts::InvalidJSONError);
				}
            }
            else
            {
				TSharedPtr<FJsonObject> JsonObject;
                TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
				if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
                {
                    FString ErrorText = JsonObject->GetStringField("error");

					HandleError(ErrorText);

					return;
                }
				else {
					HandleError(AvatarLoaderTexts::BadResponseError);
				}
            }
        }
        else
        {
            HandleError(AvatarLoaderTexts::SomethingWentWrongError);

			if (Response.IsValid())
			{
				UE_LOG(LogTemp, Warning, TEXT("Response code: %d"), static_cast<int32>(Response->GetResponseCode()));
			}
        } });

	HttpRequest->ProcessRequest();
}

void FAvatarLoaderModule::DownloadFBXFile(const FString SignedUrl)
{
	FString FbxFileURL = FString::Printf(TEXT("%s"), *SignedUrl);

	FString FolderPath = AvatarLoaderTexts::FolderPath;
	FFolderHelper::CreateFolder(FolderPath);

	FString FilePath = FPaths::Combine(FPaths::ProjectContentDir(), FolderPath, TEXT("avatar.fbx"));

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb(TEXT("GET"));
	HttpRequest->SetURL(FbxFileURL);

	HttpRequest->OnProcessRequestComplete().BindLambda([this, FilePath, FolderPath](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
													   {
        if (bWasSuccessful && Response.IsValid()) {
			if (Response->GetResponseCode() == EHttpResponseCodes::Ok) {
				TArray<uint8> ContentData;

				ContentData.Append((const uint8*)Response->GetContent().GetData(), Response->GetContentLength());

				if (FFileHelper::SaveArrayToFile(ContentData, *FilePath)) {
					UE_LOG(LogTemp, Warning, TEXT("FBX file downloaded and saved successfully: %s"), *FilePath);

					ImportFBXFile(FilePath, FolderPath);
				}
			} else {
				HandleError(AvatarLoaderTexts::BadResponseDownloadError);
			}
		} else {
			HandleError(AvatarLoaderTexts::RequestFailedDownloadError);
		} });

	HttpRequest->ProcessRequest();
}

void FAvatarLoaderModule::ImportFBXFile(const FString &FilePath, const FString &FolderPath)
{
	FString AssetPath = FString("/Game/") + FolderPath + TEXT("/") + FPaths::GetBaseFilename(FilePath);

	FAssetRegistryModule &AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().ScanPathsSynchronous({"/Game/"}, true);
	TArray<FAssetData> AssetData;
	AssetRegistryModule.Get().GetAssetsByPackageName(*AssetPath, AssetData);

	if (AssetData.Num() > 0)
	{
		WebBrowserWidget->LoadURL(FString::Printf(TEXT("%s/already-imported"), *AvatarLoaderTexts::InitialUrl));

		return;
	}

	UFbxFactory *Factory = NewObject<UFbxFactory>();
	Factory->AddToRoot();
	Factory->ImportUI->bIsObjImport = false;
	Factory->ImportUI->bImportAsSkeletal = true;
	Factory->ImportUI->bImportMesh = true;
	Factory->ImportUI->bAutomatedImportShouldDetectType = true;
	Factory->ImportUI->bImportMaterials = true;
	Factory->ImportUI->bImportTextures = true;
	Factory->ImportUI->SetMeshTypeToImport();
	Factory->ImportUI->bOverrideFullName = false;
	Factory->ImportUI->bCreatePhysicsAsset = true;
	Factory->ImportUI->bImportAnimations = true;
	Factory->ImportUI->FileAxisDirection = TEXT("-90");

	FAssetToolsModule &AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	TArray<FString> FilePaths;
	FilePaths.Add(FilePath);
	AssetToolsModule.Get().ImportAssets(FilePaths, FString("/Game/") + FolderPath, Factory);

	if (Window.IsValid())
	{
		Window->RequestDestroyWindow();
	}

	Factory->RemoveFromRoot();
}

void FAvatarLoaderModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu *Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection &Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FAvatarLoaderCommands::Get().PluginAction, PluginCommands);
		}
	}

	{
		UToolMenu *ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
		{
			FToolMenuSection &Section = ToolbarMenu->FindOrAddSection("PluginTools");
			{
				FToolMenuEntry &Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FAvatarLoaderCommands::Get().PluginAction));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

void FAvatarLoaderModule::HandleError(const FString &ErrorMessage)
{
	UE_LOG(LogTemp, Warning, TEXT("%s"), *ErrorMessage);

	WebBrowserWidget->LoadURL(FString::Printf(TEXT("%s/error-page/%s"), *AvatarLoaderTexts::InitialUrl, *ErrorMessage));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAvatarLoaderModule, AvatarLoader)