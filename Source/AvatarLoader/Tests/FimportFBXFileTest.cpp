#include "AvatarLoader.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Factories/FbxImportUI.h"

bool ImportFBXFileTest()
{
    FString PluginContentFolder = TEXT("Plugins/AvatarLoader/Content/FBXFile");

    FString FilePath = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), PluginContentFolder, TEXT("test.fbx")));

    FAvatarLoaderModule AvatarLoader;
    AvatarLoader.ImportFBXFile(FilePath, PluginContentFolder);

    // Check if the asset was imported successfully
    FString AssetPath = FString("/Game/") + PluginContentFolder + TEXT("/") + FPaths::GetBaseFilename(FilePath);
    FAssetRegistryModule &AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    AssetRegistryModule.Get().ScanPathsSynchronous({"/Game/"}, true);
    TArray<FAssetData> AssetData;
    AssetRegistryModule.Get().GetAssetsByPackageName(*AssetPath, AssetData);

    return AssetData.Num() > 0;
}

// Register the test with the Automation system
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FImportFBXFileTest, "Import.FBXFile.ImportFBXFile", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FImportFBXFileTest::RunTest(const FString &Parameters)
{
    TestTrue("FBX file was imported", ImportFBXFileTest());
    return true;
}
#endif
