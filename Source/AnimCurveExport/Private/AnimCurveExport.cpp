// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "AnimCurveExport.h"
#include "AnimCurveExportCommands.h"
#include "AnimCurveExportStyle.h"
#include "AnimationBlueprintLibrary.h"
#include "AnimationEditorUtils.h"
#include "AnimationRuntime.h"
#include "AssetRegistryModule.h"

#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"

#include "AssetData.h"
#include "FileHelpers.h"
#include "ISkeletonEditorModule.h"
#include "PersonaModule.h"

#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "LevelEditor.h"
#include "PackageTools.h"
#include "PropertyCustomizationHelpers.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"

#include "Animation/AnimNodeBase.h"
#include "Curves/CurveVector.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Widgets/Layout/SScrollBox.h"

static const FName AnimCurveExportTabName("AnimSequenceToolkits");

#define LOCTEXT_NAMESPACE "FAnimSequenceToolkitsModule"

void FAnimSequenceToolkitsModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact
	// timing is specified in the .uplugin file per-module

	FAnimCurveExportStyle::Initialize();
	FAnimCurveExportStyle::ReloadTextures();

	FAnimCurveExportCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FAnimCurveExportCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(
			this, &FAnimSequenceToolkitsModule::PluginButtonClicked),
		FCanExecuteAction());

	FLevelEditorModule& LevelEditorModule =
		FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

	{
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddMenuExtension(
			"WindowLayout", EExtensionHook::After, PluginCommands,
			FMenuExtensionDelegate::CreateRaw(
				this, &FAnimSequenceToolkitsModule::AddMenuExtension));

		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
	}

	{
		TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
		ToolbarExtender->AddToolBarExtension(
			"Settings", EExtensionHook::After, PluginCommands,
			FToolBarExtensionDelegate::CreateRaw(
				this, &FAnimSequenceToolkitsModule::AddToolbarExtension));

		LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(
			ToolbarExtender);
	}

	FGlobalTabmanager::Get()
		->RegisterNomadTabSpawner(
			AnimCurveExportTabName,
			FOnSpawnTab::CreateRaw(
				this, &FAnimSequenceToolkitsModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FAnimCurveExportTabTitle", "AnimCurveExport"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FAnimSequenceToolkitsModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For
	// modules that support dynamic reloading, we call this function before
	// unloading the module.
	FAnimCurveExportStyle::Shutdown();

	FAnimCurveExportCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(AnimCurveExportTabName);
}

/* DEPRECATED
TSharedRef<SWidget> FAnimSequenceToolkitsModule::ConstructAnimationPicker()
{
	TArray<const UClass*> ClassFilters;
	ClassFilters.Add(UAnimSequence::StaticClass());
	FAssetData CurrentAssetData = FAssetData();

	auto AnimSequencePicker =
PropertyCustomizationHelpers::MakeAssetPickerWithMenu( 		FAssetData(), 		true,
		ClassFilters,
		TArray<UFactory*>(),
		FOnShouldFilterAsset(),
		FOnAssetSelected::CreateLambda([this](FAssetData const&
InAssetData)
		{
			TargetAnimSequence =
Cast<UAnimSequence>(InAssetData.GetAsset());
			// TargetSkeleton = TargetAnimSequence->GetSkeleton();
		}),
		FSimpleDelegate()
	);
	return AnimSequencePicker;
}
*/

void FAnimSequenceToolkitsModule::OnAnimSequenceSelected(
	const FAssetData& SelectedAsset)
{
	UAnimSequence* Sequence = Cast<UAnimSequence>(SelectedAsset.GetAsset());
	if (!Sequence)
	{
		return;
	}
	UE_LOG(LogTemp, Log, TEXT("Get Current Selection: %s"),
		*(Sequence->GetName()));
	// AnimSequences.Add(Sequence);
	TargetAnimSequence = Sequence;
	// TargetSkeleton = Sequence->GetSkeleton();
}

UCurveVector*
FAnimSequenceToolkitsModule::CreateCurveVectorAsset(const FString& PackagePath,
	const FString& CurveName)
{
	const auto PackageName =
		UPackageTools::SanitizePackageName(PackagePath + "/" + CurveName);
	const auto Package = CreatePackage(nullptr, *PackageName);
	EObjectFlags Flags = RF_Public | RF_Standalone | RF_Transactional;

	const auto NewObj =
		NewObject<UCurveVector>(Package, FName(*CurveName), Flags);
	if (NewObj)
	{
		FAssetRegistryModule::AssetCreated(NewObj);
		Package->MarkPackageDirty();
	}
	return NewObj;
}

bool FAnimSequenceToolkitsModule::GetBoneKeysByNameHelper(
	UAnimSequence* Seq, FString const& BoneName, TArray<FVector>& OutPosKey,
	TArray<FQuat>& OutRotKey, bool bConvertCS /* = false */)
{
	auto Skeleton = Seq->GetSkeleton();
	auto RefSkeleton = Skeleton->GetReferenceSkeleton();
	auto BoneIndex = RefSkeleton.FindRawBoneIndex(*BoneName);
	if (BoneIndex == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("BoneName: %s not exist"), *BoneName);
		return false;
	}
	// In BoneSpace, need Convert to ComponentSpace
	auto NbrOfFrames = Seq->GetNumberOfFrames();
	OutPosKey.Init(FVector::ZeroVector, NbrOfFrames);
	OutRotKey.Init(FQuat::Identity, NbrOfFrames);
	auto BoneInfos = RefSkeleton.GetRefBoneInfo();

	TArray<FName> BoneTraces;
	do
	{
		auto _BoneName = RefSkeleton.GetBoneName(BoneIndex);
		BoneTraces.Add(_BoneName);
		BoneIndex = BoneInfos[BoneIndex].ParentIndex;
	} while (bConvertCS && BoneIndex);

	TArray<FTransform> Poses;
	for (int i = 0; i < Seq->GetNumberOfFrames(); ++i)
	{
		UAnimationBlueprintLibrary::GetBonePosesForFrame(Seq, BoneTraces, i, false, Poses);
		FTransform FinalTransform = FTransform::Identity;
		for (int j = 0; j < BoneTraces.Num(); ++j)
		{
			FinalTransform = FinalTransform * Poses[j];
		}

		OutPosKey[i] = FinalTransform.GetLocation();
		OutRotKey[i] = FinalTransform.GetRotation();
	}

	/* NOT USE ANYMORE
	do
	{
		auto _BoneName = RefSkeleton.GetBoneName(BoneIndex);
		UE_LOG(LogTemp, Log, TEXT("Current Bone Hierarchy %d, %s"), BoneIndex, *_BoneName.ToString());

		auto TrackIndex = AnimTrackNames.IndexOfByKey(_BoneName); // Correspond BoneIndex in AnimTracks, Find by Unique Name
		auto Track = Seq->GetRawAnimationTrack(TrackIndex);
		auto CurrPosKey = Track.PosKeys;
		auto CurrRotKey = Track.RotKeys;
		uint32 x = 0, IncrementPos = CurrPosKey.Num() != 1;
		uint32 y = 0, IncrementRot = CurrRotKey.Num() != 1;
		// FAnimationRuntime::GetComponentSpaceTransformRefPose();
		for (int i = 0; i < OutPosKey.Num(); ++i)
		{
			// UE_LOG(LogTemp, Log, TEXT("BoneIndex: %d, Get Relative Z value %f."),
			// BoneIndex, CurrPosKey[x].Z);
			OutPosKey[i] += CurrPosKey[x];
			OutRotKey[i] *= CurrRotKey[y];
			x += IncrementPos;
			y += IncrementRot;
		}
		BoneIndex = BoneInfos[BoneIndex].ParentIndex;
	} while (bConvertCS && BoneIndex);
	*/

	return true;
}

void FAnimSequenceToolkitsModule::MarkFootstepsFor1PAnimation(
	UAnimSequence* Seq, FString const& KeyBone /* = "LeftHand" */)
{
	FFloatCurve FootstepsCurve, PosXCurve, PosZCurve, RotYCurve;
	// Get KeyBone translation and rotation keys
	TArray<FVector> PosKeys;
	TArray<FQuat> RotKeys;
	GetBoneKeysByNameHelper(Seq, KeyBone, PosKeys, RotKeys, true);

	// Total Nums of the Key
	// Strategy: Capture the local Z minimal point in LeftHand
	const auto N = PosKeys.Num();
	const auto M = RotKeys.Num();
	const auto FrameCount = Seq->GetNumberOfFrames();

	UE_LOG(LogTemp, Log, TEXT("key counts Pos: %d, Rot: %d"), N, M);
	FVector RotAvg(0.0f), PosAvg(0.0f);

	TArray<FVector> PosCache, RotCache;
	uint32 x = 0, PosIncrement = N == 1 ? 0 : 1;
	uint32 y = 0, RotIncrement = M == 1 ? 0 : 1;
	for (int i = 0; i < FrameCount; ++i)
	{
		auto Time = Seq->GetTimeAtFrame(i);

		FVector CurrPos = PosKeys[x];
		FVector CurrRot = RotKeys[y].Euler();

		// Coordinates Transform
		Swap(CurrPos.Y, CurrPos.Z);
		CurrPos.Z = -CurrPos.Z;

		x += PosIncrement;
		y += RotIncrement;

		PosCache.Add(CurrPos);
		RotCache.Add(CurrRot);
		PosAvg += CurrPos;
		RotAvg += CurrRot;

		PosXCurve.FloatCurve.UpdateOrAddKey(Time, CurrPos.X);
		PosZCurve.FloatCurve.UpdateOrAddKey(Time, CurrPos.Z);
		RotYCurve.FloatCurve.UpdateOrAddKey(Time, CurrRot.Y, true);
	}

	RotAvg /= Seq->GetNumberOfFrames();
	PosAvg /= Seq->GetNumberOfFrames();
	UE_LOG(LogTemp, Log, TEXT("Position Average: %f, Rotation Average: %f"),
		PosAvg.X, RotAvg.Y);

	FVector PrevPos, NextPos, CurrPos;
	FVector PrevRot, NextRot, CurrRot;

	TArray<float> LocalMinimals;
	TArray<int32> MinimalKeys;
	TArray<int32> Permutations;

	for (int i = 0; i < FrameCount; ++i)
	{
		PrevPos = PosCache[(i - 1 + N) % N];
		CurrPos = PosCache[i];
		NextPos = PosCache[(i + 1) % N];

		if (CurrPos.Z <= PrevPos.Z && CurrPos.Z <= NextPos.Z)
		{
			Permutations.Add(LocalMinimals.Num());
			MinimalKeys.Add(i);
			LocalMinimals.Add(CurrPos.Z);
		}

		/* ERROR
		auto Pos0 = PosCache[i];
		auto Pos1 = PosCache[i + 1];

		float Rate = ((PosAvg.X - Pos0.X) / (Pos1.X - Pos0.X));
		if (0 <= Rate && Rate <= 1)
		{
			auto Time0 = Seq->GetTimeAtFrame(i);
			auto Time1 = Seq->GetTimeAtFrame(i + 1);
			auto FootstepTime = FMath::Lerp(Time0, Time1, Rate);
			// 1 means right, -1 means left
			auto LegType = Pos0.X > Pos1.X ? 1 : -1;


			FootstepsCurve.UpdateOrAddKey(Pos0.X > Pos1.X ? 1 : -1, Time0);

			// if (Pos0.X > Pos1.X)
			// {
			// 	FootstepsCurve->UpdateOrAddKey(-1, FootstepTime - 0.001);
			// 	FootstepsCurve.UpdateOrAddKey(1, FootstepTime);
			// }
			// else
			// {
			// 	FootstepsCurve.UpdateOrAddKey(1, FootstepTime);
			// 	FootstepsCurve->UpdateOrAddKey(-1, FootstepTime + 0.001);
			// }
		}
		*/
	}

	Permutations.Sort([&](int i, int j)
	{
		return LocalMinimals[i] < LocalMinimals[j];
	});

	if (LocalMinimals.Num() >= 2)
	{
		auto I = Permutations[0], II = Permutations[1];
		FootstepsCurve.UpdateOrAddKey(LocalMinimals[I], Seq->GetTimeAtFrame(MinimalKeys[I]));
		FootstepsCurve.UpdateOrAddKey(LocalMinimals[II], Seq->GetTimeAtFrame(MinimalKeys[II]));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("%s: Footsteps Mark Algorithm failed when LeftHand.Z Curve has less than 2 local minimals"), *Seq->GetName());
	}

	for (auto It = FootstepsCurve.FloatCurve.GetKeyHandleIterator(); It; ++It)
	{
		const FKeyHandle KeyHandle = *It;
		FootstepsCurve.FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Constant);
	}

	SetVariableCurveHelper(Seq, "Footsteps_Curve", FootstepsCurve);
	SetVariableCurveHelper(Seq, FString::Printf(TEXT("%s_PosX_Curve"), *KeyBone), PosXCurve);
	SetVariableCurveHelper(Seq, FString::Printf(TEXT("%s_PosZ_Curve"), *KeyBone), PosZCurve);
	SetVariableCurveHelper(Seq, FString::Printf(TEXT("%s_RotY_Curve"), *KeyBone), RotYCurve);

	return;
}

bool FAnimSequenceToolkitsModule::SetVariableCurveHelper(
	UAnimSequence* Seq, const FString& CurveName, FFloatCurve& InFloatCurve)
{
	// Create Variable Curve
	USkeleton* Skeleton = Seq->GetSkeleton();
	FSmartName NewTrackName;
	// Add Skeleton Curve if not exists
	Skeleton->AddSmartNameAndModify(USkeleton::AnimCurveMappingName, *CurveName,
		NewTrackName);

	if (!Seq->RawCurveData.GetCurveData(NewTrackName.UID))
	{
		// Add Footsteps Track for animation sequence if not exists
		Seq->Modify(true);
		Seq->RawCurveData.AddCurveData(NewTrackName);
		Seq->MarkRawDataAsModified();
	}
	auto Curve = static_cast<FFloatCurve*>(Seq->RawCurveData.GetCurveData(NewTrackName.UID));
	Curve->FloatCurve = InFloatCurve.FloatCurve;
	return true;
}

bool FAnimSequenceToolkitsModule::SaveBonesCurves(UAnimSequence* AnimSequence, FString const& BoneName, const FString& SavePath)
{
	TArray<UPackage*> Packages;

	const auto AnimName = AnimSequence->GetName();
	const auto PackagePath = SavePath + "/" + AnimName;
	// Initialize Bone Packages
	const auto CurveNamePrefix = AnimName + "_" + BoneName;
	auto PosCurve = CreateCurveVectorAsset(PackagePath, CurveNamePrefix + "_Translation");
	auto RotCurve = CreateCurveVectorAsset(PackagePath, CurveNamePrefix + "_Rotation");

	Packages.Add(PosCurve->GetOutermost());
	Packages.Add(RotCurve->GetOutermost());

	TArray<FVector> PosKeys;
	TArray<FQuat> RotKeys;
	GetBoneKeysByNameHelper(AnimSequence, BoneName, PosKeys, RotKeys, true);

	for (int i = 0; i < AnimSequence->GetRawNumberOfFrames(); ++i)
	{
		const auto Time = AnimSequence->GetTimeAtFrame(i);
		const auto Translation = PosKeys[i];
		const auto EulerAngle = RotKeys[i].Euler();

		PosCurve->FloatCurves[0].UpdateOrAddKey(Time, Translation.X);
		PosCurve->FloatCurves[1].UpdateOrAddKey(Time, Translation.Y);
		PosCurve->FloatCurves[2].UpdateOrAddKey(Time, Translation.Z);

		RotCurve->FloatCurves[0].UpdateOrAddKey(Time, EulerAngle.X, true);
		RotCurve->FloatCurves[1].UpdateOrAddKey(Time, EulerAngle.Y, true);
		RotCurve->FloatCurves[2].UpdateOrAddKey(Time, EulerAngle.Z, true);
	}
	// MarkFootstepsFor1PAnimation(AnimSequence);
	// Save Packages
	return UEditorLoadingAndSavingUtils::SavePackages(Packages, true);
}

TSharedRef<SWidget> FAnimSequenceToolkitsModule::ConstructSequenceBrowser()
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	// Configure filter for asset picker
	FARFilter Filter;
	Filter.bRecursiveClasses = true;
	Filter.ClassNames.Add(UAnimSequence::StaticClass()->GetFName());
	// Filter.ClassNames.Add(USoundWave::StaticClass()->GetFName());

	FAssetPickerConfig Config;
	Config.Filter = Filter;
	Config.InitialAssetViewType = EAssetViewType::Column;
	Config.bAddFilterUI = true;
	Config.bShowPathInColumnView = true;
	Config.bSortByPathInColumnView = true;
	Config.GetCurrentSelectionDelegates.Add(&GetCurrentSelectionDelegate);

	// Configure response to click
	Config.OnAssetSelected = FOnAssetSelected::CreateRaw(
		this, &FAnimSequenceToolkitsModule::OnAnimSequenceSelected);
	Config.bFocusSearchBoxWhenOpened = false;
	Config.DefaultFilterMenuExpansion = EAssetTypeCategories::Animation;
	Config.SaveSettingsName = "SequenceBrowser";

	SequencePickerWidget = ContentBrowserModule.Get().CreateAssetPicker(Config);
	return SNew(SBorder)[SequencePickerWidget.ToSharedRef()];
}

TSharedRef<SWidget> FAnimSequenceToolkitsModule::ConstructBonePickerWidget()
{
	return SNew(SBorder)
	       [
		       SAssignNew(BonePickerWidget, SVerticalBox)
		       + SVerticalBox::Slot()
		         .AutoHeight()
		         .VAlign(VAlign_Center)
		       [
			       SNew(STextBlock)
			       .Text(FText::FromString("Target Bone Name"))
			       .Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
		       ]
		       + SVerticalBox::Slot()
		         .AutoHeight()
		         .VAlign(VAlign_Center)
		         .Padding(5, 0, 0, 0)
		       [
			       SNew(SEditableTextBox)
			       .MinDesiredWidth(50)
			       .Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
			       .Text_Lambda([this]() -> FText { return TargetBoneText; })
			       .OnTextCommitted_Lambda([this](const FText& InText, ETextCommit::Type CommitInfo) { TargetBoneText = InText; })
		       ]
	       ];
}

// TSharedRef<SWidget> FAnimSequenceToolkitsModule::ConstructBoneTreeList()
// {
// 	FSkeletonTreeArgs SkeletonTreeArgs;
// 	// SkeletonTreeArgs.OnSelectionChanged =
// FOnSkeletonTreeSelectionChanged::CreateSP(
// 	//  	this, &FAnimCurveExportModule::HandleSelectionChanged);
// 	ISkeletonEditorModule& SkeletonEditorModule =
// FModuleManager::GetModuleChecked<ISkeletonEditorModule>( 		"SkeletonEditor");
// 	TSharedPtr<ISkeletonTree> SkeletonTree =
// SkeletonEditorModule.CreateSkeletonTree( 		TargetAnimSequence->GetSkeleton(),
// SkeletonTreeArgs); 	return SkeletonTree.ToSharedRef();
// }

TSharedRef<SWidget>
FAnimSequenceToolkitsModule::ConstructAnimSequenceListWidget()
{
	auto MakeAnimSequenceTableRow =
		[](UAnimSequence* InSeq,
		const TSharedRef<STableViewBase>& Owner) -> TSharedRef<ITableRow>
	{
		return SNew(STableRow<UAnimSequence*>, Owner)
		       .Padding(FMargin(16, 4, 16, 4))
		       [
			       SNew(STextBlock)
			       .Text(FText::FromString(InSeq->GetName()))
		       ];
	};

	auto ScrollBar = SNew(SScrollBar);
	auto HeaderRowWidget =
		SNew(SHeaderRow)
		+ SHeaderRow::Column("Current AnimSequence Selections")
		.FillWidth(0.80f)
		[
			SNew(STextBlock)
			.Text(FText::FromString("Selected AnimSequences"))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
		];

	return SNew(SBorder)
	       .BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
	       [
		       SNew(SVerticalBox)
		       + SVerticalBox::Slot()
		       .FillHeight(1.0f)
		       [
			       SNew(SBorder)
			       .BorderImage(FEditorStyle::GetBrush("MessageLog.ListBorder"))
			       [
				       SNew(SScrollBox)
				       + SScrollBox::Slot()
				       [
					       SAssignNew(SequencesListView, SListView<UAnimSequence*>)
					       .ItemHeight(40)
					       .ListItemsSource(&AnimSequences)
					       .OnGenerateRow_Lambda(MakeAnimSequenceTableRow)
					       .HeaderRow(HeaderRowWidget)
				       ]
			       ]
		       ]
		       + SVerticalBox::Slot()
		         .AutoHeight()
		         .VAlign(VAlign_Bottom)
		       [
			       SNew(SButton)
			       .HAlign(HAlign_Center)
			       .Text(FText::FromString("Add from Browser"))
			       .OnClicked_Raw(this, &FAnimSequenceToolkitsModule::OnSequencesAdd)
		       ]
		       + SVerticalBox::Slot()
		         .AutoHeight()
		         .VAlign(VAlign_Bottom)
		       [
			       SNew(SButton)
			       .HAlign(HAlign_Center)
			       .Text(FText::FromString("Remove Selected"))
			       .OnClicked_Raw(this, &FAnimSequenceToolkitsModule::OnSequencesClear)
		       ]
	       ];
}

TSharedRef<SWidget> FAnimSequenceToolkitsModule::ConstructPathPickerWidget()
{
	auto MakePathPickerWidget = [&, this]()
	{
		const auto& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		FPathPickerConfig PathPickerConfig;
		PathPickerConfig.OnPathSelected = FOnPathSelected::CreateLambda([this](const FString& NewPath) { ExportPath = NewPath; });
		PathPickerConfig.DefaultPath = "/Game";
		return ContentBrowserModule.Get().CreatePathPicker(PathPickerConfig);
	};

	return SNew(SBorder)
	       [
		       SAssignNew(PathPickerWidget, SVerticalBox)
		       + SVerticalBox::Slot()
		       .AutoHeight()
		       [
			       SNew(STextBlock)
			       .Text(FText::FromString("Output Path"))
			       .Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
		       ]
		       + SVerticalBox::Slot()
		       .AutoHeight()
		       [
			       SNew(SComboButton)
			       .OnGetMenuContent_Lambda(MakePathPickerWidget)
			       .ButtonContent()
			       [
				       SNew(STextBlock)
				       .Text_Lambda([this]() { return FText::FromString(ExportPath); })
			       ]
		       ]
	       ];
	// return PathPickerWidget.ToSharedRef();
}

TSharedRef<SWidget>
FAnimSequenceToolkitsModule::ConstructCurveExtractorWidget()
{
	return SNew(SVerticalBox)
	       + SVerticalBox::Slot()
	         .AutoHeight()
	         .VAlign(VAlign_Top)
	       [
		       SNew(STextBlock)
		       .Text(FText::FromString("Animation Curve Extractor"))
		       .Font(FCoreStyle::GetDefaultFontStyle("Regular", 20))
	       ]
	       + SVerticalBox::Slot()
	       [
		       ConstructAnimSequenceListWidget()
	       ]
	       + SVerticalBox::Slot()
	         .AutoHeight()
	         .VAlign(VAlign_Bottom)
	       [
		       ConstructBonePickerWidget()
	       ]
	       + SVerticalBox::Slot()
	         .AutoHeight()
	         .VAlign(VAlign_Bottom)
	       [
		       ConstructPathPickerWidget()
	       ]
	       + SVerticalBox::Slot()
	         .AutoHeight()
	         .VAlign(VAlign_Bottom)
	       [
		       SNew(SButton)
				.HAlign(HAlign_Center)
				.Text(FText::FromString("Export Curve"))
				.OnClicked_Lambda([this]()
		                    {
			                    for (auto Seq : AnimSequences)
			                    {
				                    SaveBonesCurves(Seq, TargetBoneText.ToString(), ExportPath);
			                    }
			                    return FReply::Handled();
		                    })
	       ]
	       + SVerticalBox::Slot()
	         .AutoHeight()
	         .VAlign(VAlign_Bottom)
	       [
		       SNew(SButton)
		       .HAlign(HAlign_Center)
		       .Text(FText::FromString("Set Footsteps"))
		       .OnClicked_Lambda([this]()
		       {
			       for (auto Seq : AnimSequences)
			       {
				       // HARDCODE: set LeftHand as Pivot
				       MarkFootstepsFor1PAnimation(Seq, "LeftHand");
			       }
			       return FReply::Handled();
		       })
	       ];
}

TSharedRef<SDockTab> FAnimSequenceToolkitsModule::OnSpawnPluginTab(
	const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
	       .TabRole(ETabRole::NomadTab)
	       [
		       SNew(SBorder)
		       [
			       SNew(SHorizontalBox)
			       + SHorizontalBox::Slot()
			         .FillWidth(1)
			         .Padding(2, 0, 2, 0)
			       [
				       ConstructCurveExtractorWidget()
			       ]
			       + SHorizontalBox::Slot()
			       .FillWidth(2)
			       [
				       ConstructSequenceBrowser()
			       ]
			       // ] + SHorizontalBox::Slot().AutoWidth()[
			       // 	ConstructBoneTreeList()
			       // ]
		       ]
	       ];
}

FReply FAnimSequenceToolkitsModule::OnSequencesAdd()
{
	TArray<FAssetData> SelectedAssets = GetCurrentSelectionDelegate.Execute();
	for (FAssetData& d : SelectedAssets)
	{
		UAnimSequence* Anim = Cast<UAnimSequence>(d.GetAsset());
		if (Anim && AnimSequences.Find(Anim) == INDEX_NONE)
		{
			AnimSequences.Add(Anim);
		}
	}
	SequencesListView->RequestListRefresh();
	return FReply::Handled();
}

FReply FAnimSequenceToolkitsModule::OnSequencesClear()
{
	auto SelectedItems = SequencesListView->GetSelectedItems();
	for (auto& Item : SelectedItems)
	{
		AnimSequences.Remove(Item);
	}
	SequencesListView->RequestListRefresh();
	// AnimSequences.Reset();
	return FReply::Handled();
}

void FAnimSequenceToolkitsModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->InvokeTab(AnimCurveExportTabName);
}

void FAnimSequenceToolkitsModule::AddMenuExtension(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FAnimCurveExportCommands::Get().OpenPluginWindow);
}

void FAnimSequenceToolkitsModule::AddToolbarExtension(
	FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(FAnimCurveExportCommands::Get().OpenPluginWindow);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAnimSequenceToolkitsModule, AnimCurveExport)
