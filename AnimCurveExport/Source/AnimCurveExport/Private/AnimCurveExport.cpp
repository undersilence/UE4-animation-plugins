// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "AnimCurveExport.h"
#include "AnimationBlueprintLibrary.h"
#include "AnimCurveExportStyle.h"
#include "AnimCurveExportCommands.h"
#include "AssetRegistryModule.h"

#include "IContentBrowserSingleton.h"
#include "ContentBrowserModule.h"
#include "FileHelpers.h"

#include "LevelEditor.h"
#include "PackageTools.h"
#include "PropertyCustomizationHelpers.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

#include "Animation/AnimNodeBase.h"
#include "Curves/CurveVector.h"

static const FName AnimCurveExportTabName("AnimCurveExport");

#define LOCTEXT_NAMESPACE "FAnimCurveExportModule"

void FAnimCurveExportModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	FAnimCurveExportStyle::Initialize();
	FAnimCurveExportStyle::ReloadTextures();

	FAnimCurveExportCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FAnimCurveExportCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FAnimCurveExportModule::PluginButtonClicked),
		FCanExecuteAction());

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

	{
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddMenuExtension("WindowLayout", EExtensionHook::After, PluginCommands,
		                               FMenuExtensionDelegate::CreateRaw(
			                               this, &FAnimCurveExportModule::AddMenuExtension));

		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
	}

	{
		TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
		ToolbarExtender->AddToolBarExtension("Settings", EExtensionHook::After, PluginCommands,
		                                     FToolBarExtensionDelegate::CreateRaw(
			                                     this, &FAnimCurveExportModule::AddToolbarExtension));

		LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
	}

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(AnimCurveExportTabName,
	                                                  FOnSpawnTab::CreateRaw(
		                                                  this, &FAnimCurveExportModule::OnSpawnPluginTab))
	                        .SetDisplayName(LOCTEXT("FAnimCurveExportTabTitle", "AnimCurveExport"))
	                        .SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FAnimCurveExportModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FAnimCurveExportStyle::Shutdown();

	FAnimCurveExportCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(AnimCurveExportTabName);
}

// FReply FAnimCurveExportModule::AddFromContentBrowser()
// {
// 	TArray<FAssetData> SelectedAssets;
// 	FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser").Get().GetSelectedAssets(SelectedAssets);
// 	for (FAssetData& d : SelectedAssets)
// 	{
// 		UAnimSequence* Anim = Cast<UAnimSequence>(d.GetAsset());
// 		if (Anim && SelectedAnimGroup.Find(Anim) == INDEX_NONE)
// 		{
// 			SelectedAnimGroup.Add(Anim);
// 		}
// 	}
// 	return FReply::Handled();
// }

// void FAnimCurveExportModule::OnPickAnimation(FAssetData const& AssetData)
// {
// 	TargetAnimSequence = Cast<UAnimSequence>(AssetData.GetAsset());
// }

TSharedRef<SWidget> FAnimCurveExportModule::OnCreateAnimPicker()
{
	TArray<const UClass*> ClassFilters;
	ClassFilters.Add(UAnimSequence::StaticClass());
	FAssetData CurrentAssetData = FAssetData();

	auto AnimSequencePicker = PropertyCustomizationHelpers::MakeAssetPickerWithMenu(
		FAssetData(),
		true,
		ClassFilters,
		TArray<UFactory*>(),
		FOnShouldFilterAsset(),
		FOnAssetSelected::CreateLambda([this](FAssetData const& InAssetData)
		{
			TargetAnimSequence = Cast<UAnimSequence>(InAssetData.GetAsset());
		}),
		FSimpleDelegate()
	);

	return AnimSequencePicker;
}

UCurveVector* FAnimCurveExportModule::CreateCurveVectorAsset(const FString& PackagePath, const FString& CurveName)
{
	const auto PackageName = UPackageTools::SanitizePackageName(PackagePath + "/" + CurveName);
	const auto Package = CreatePackage(nullptr, *PackageName);
	EObjectFlags Flags = RF_Public | RF_Standalone | RF_Transactional;

	const auto NewObj = NewObject<UCurveVector>(Package, FName(*CurveName), Flags);
	if (NewObj)
	{
		FAssetRegistryModule::AssetCreated(NewObj);
		Package->MarkPackageDirty();
	}
	return NewObj;
}

bool FAnimCurveExportModule::SaveBonesCurves(const UAnimSequence* AnimSequence,
                                             TArray<FName> BoneNames,
                                             const FString& SavePath)
{
	TArray<FTransform> Poses{};
	TArray<UPackage*> Packages;
	TArray<UCurveVector*> BoneTranslationCurves;
	TArray<UCurveVector*> BoneRotationCurves;

	const auto AnimName = AnimSequence->GetName();
	const auto PackagePath = SavePath + "/" + AnimName;

	// Initialize Bone Packages
	for (auto const& BoneName : BoneNames)
	{
		const auto CurveNamePrefix = AnimName + "_" + BoneName.ToString();
		auto TranslationCurve = CreateCurveVectorAsset(PackagePath, CurveNamePrefix + "_Translation");
		auto RotationCurve = CreateCurveVectorAsset(PackagePath, CurveNamePrefix + "_Rotation");

		BoneTranslationCurves.Add(TranslationCurve);
		BoneRotationCurves.Add(RotationCurve);
		Packages.Add(TranslationCurve->GetOutermost());
		Packages.Add(RotationCurve->GetOutermost());
	}

	for (int i = 0; i < AnimSequence->GetRawNumberOfFrames(); ++i)
	{
		UAnimationBlueprintLibrary::GetBonePosesForFrame(AnimSequence, BoneNames, i, false, Poses);
		const auto Time = AnimSequence->GetTimeAtFrame(i);
		for (int j = 0; j < BoneNames.Num(); ++j)
		{
			const auto Translation = Poses[j].GetLocation();
			const auto EulerAngle = Poses[j].GetRotation().Euler();

			BoneTranslationCurves[j]->FloatCurves[0].UpdateOrAddKey(Time, Translation.X);
			BoneTranslationCurves[j]->FloatCurves[1].UpdateOrAddKey(Time, Translation.Y);
			BoneTranslationCurves[j]->FloatCurves[2].UpdateOrAddKey(Time, Translation.Z);

			BoneRotationCurves[j]->FloatCurves[0].UpdateOrAddKey(Time, EulerAngle.X, true);
			BoneRotationCurves[j]->FloatCurves[1].UpdateOrAddKey(Time, EulerAngle.Y, true);
			BoneRotationCurves[j]->FloatCurves[2].UpdateOrAddKey(Time, EulerAngle.Z, true);
		}
	}

	// Save Packages
	return UEditorLoadingAndSavingUtils::SavePackages(Packages, true);
}

/* Deprecated Old
void FAnimCurveExportModule::SaveCurveVector(const FString& PackagePath, const FString& CurveName,
const UCurveVector& CurveVector)
{
	auto PackageName = PackagePath + TEXT("/") + CurveName;
	auto Package = CreatePackage(nullptr, *PackageName);
	EObjectFlags Flags = RF_Public | RF_Standalone | RF_Transactional;
	// Additional Copy is required
	auto CurveVectorSaved = NewObject<UCurveVector>(Package, FName(*CurveName), Flags);

	CurveVector.FloatCurves[X] = CurveVector[X];
	
	// CreateAsset first
	FAssetRegistryModule::AssetCreated();
	Package->MarkPackageDirty();

	// SaveAsset
	const FString PackageFileName = FPackageName::LongPackageNameToFilename(
		PackageName, FPackageName::GetAssetPackageExtension());
	const bool bSuccess =
		UPackage::SavePackage(Package, nullptr,
		                      RF_Public | RF_Standalone | RF_Transactional, *PackageFileName, GError, nullptr, false,
		                      true, SAVE_NoError);

	if (!bSuccess)
	{
		UE_LOG(LogTemp, Error, TEXT("Package '%s' wasn't saved!"), *PackageName)
	}

	UE_LOG(LogTemp, Warning, TEXT("Package '%s' was successfully saved"), *PackageName)
}

void FAnimCurveExportModule::ExportVectorCurve(const FString& PackagePath, const FVectorCurve& VectorCurve)
{
	auto CurveName = VectorCurve.Name.DisplayName.ToString();
	auto PackageName = PackagePath + TEXT("/") + CurveName;
	auto Package = CreatePackage(nullptr, *PackageName);
	EObjectFlags Flags = RF_Public | RF_Standalone | RF_Transactional;
	auto CurveVector = NewObject<UCurveVector>(Package, FName(*CurveName), Flags);
	
	CurveVector->FloatCurves[0] = VectorCurve.FloatCurves[0];
	CurveVector->FloatCurves[1] = VectorCurve.FloatCurves[1];
	CurveVector->FloatCurves[2] = VectorCurve.FloatCurves[2];
	
	// CreateAsset first
	FAssetRegistryModule::AssetCreated(CurveVector);
	Package->MarkPackageDirty();

	// SaveAsset
	const FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	const bool bSuccess =
		UPackage::SavePackage(Package, nullptr,
			RF_Public | RF_Standalone | RF_Transactional, *PackageFileName, GError, nullptr, false, true, SAVE_NoError);
		
	if (!bSuccess)
	{
		UE_LOG(LogTemp, Error, TEXT("Package '%s' wasn't saved!"), *PackageName)
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Package '%s' was successfully saved"), *PackageName)
 }
*/

TSharedRef<SWidget> FAnimCurveExportModule::MakeCurveExtractor()
{
	TSharedRef<SWidget> BonePicker =
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		  .AutoWidth()
		  .VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString("Target bone name: "))
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(5, 0, 0, 0).VAlign(VAlign_Center)
		[
			SNew(SEditableTextBox)
			.MinDesiredWidth(50)
			.Text_Lambda([this]() { return TargetBoneText; })
			.OnTextCommitted_Lambda([this](const FText& InText, ETextCommit::Type CommitInfo)
			{
				TargetBoneText = InText;
			})
		];

	TSharedRef<SWidget> AnimPicker =
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(15, 0, 15, 20)
		[
			SNew(SComboButton).OnGetMenuContent_Raw(this, &FAnimCurveExportModule::OnCreateAnimPicker)
			                  .ButtonContent()[
				SNew(STextBlock).Text_Lambda([this]()
				{
					return FText::FromString(TargetAnimSequence == nullptr
						                         ? "Please choose target animation"
						                         : TargetAnimSequence->GetName());
				})
			]
		];

	const auto& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	FPathPickerConfig PathPickerConfig;
	PathPickerConfig.OnPathSelected = FOnPathSelected::CreateLambda([this](const FString& NewPath)
	{
		ExportPath = NewPath;
	});
	PathPickerConfig.DefaultPath = "/Game";

	TSharedRef<SWidget> ExportPathWidget =
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(STextBlock)
			.Text(FText::FromString("Output Path"))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 5)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().MaxWidth(200)
			[
				ContentBrowserModule.Get().CreatePathPicker(PathPickerConfig)
			]
		];

	TSharedRef<SWidget> CurveExtractor =
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(STextBlock)
		.Text(FText::FromString("Animation Curve Extractor"))
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
		]
		+ SVerticalBox::Slot().AutoHeight()[
			AnimPicker
		]
		+ SVerticalBox::Slot().AutoHeight()[
			BonePicker
		]
		+ SVerticalBox::Slot().AutoHeight()[
			ExportPathWidget
		]
		+ SVerticalBox::Slot().AutoHeight()[
			SNew(SButton)
			.Text(FText::FromString("Export Curve"))
			.OnClicked_Lambda([this]()
			{
				SaveBonesCurves(TargetAnimSequence, {*TargetBoneText.ToString()}, ExportPath);
				return FReply::Handled();
			})
		];

	return CurveExtractor;
}

TSharedRef<SDockTab> FAnimCurveExportModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			// Put your tab content here!
			SNew(SBorder)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				MakeCurveExtractor()
			]
		];
}

void FAnimCurveExportModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->InvokeTab(AnimCurveExportTabName);
}

void FAnimCurveExportModule::AddMenuExtension(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FAnimCurveExportCommands::Get().OpenPluginWindow);
}

void FAnimCurveExportModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(FAnimCurveExportCommands::Get().OpenPluginWindow);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAnimCurveExportModule, AnimCurveExport)
