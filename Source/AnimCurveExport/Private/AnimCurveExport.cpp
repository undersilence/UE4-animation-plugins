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
#include "UI/AnimSequenceToolWidget.h"
#include "Widgets/Layout/SScrollBox.h"

static const FName AnimCurveExportTabName("AnimSequenceToolkits");
bool FAnimSequenceToolkitsModule::bInDebug = true;

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
	
	// {
	// 	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
	// 	ToolbarExtender->AddToolBarExtension(
	// 		"Settings", EExtensionHook::After, PluginCommands,
	// 		FToolBarExtensionDelegate::CreateRaw(
	// 			this, &FAnimSequenceToolkitsModule::AddToolbarExtension));
	//
	// 	LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(
	// 		ToolbarExtender);
	// }

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

// void FAnimSequenceToolkitsModule::OnAnimSequenceSelected(
// 	const FAssetData& SelectedAsset)
// {
// 	UAnimSequence* Sequence = Cast<UAnimSequence>(SelectedAsset.GetAsset());
// 	if (!Sequence)
// 	{
// 		return;
// 	}
// 	UE_LOG(LogTemp, Log, TEXT("Get Current Selection: %s"),
// 		*(Sequence->GetName()));
// 	// AnimSequences.Add(Sequence);
// 	TargetAnimSequence = Sequence;
// 	// TargetSkeleton = Sequence->GetSkeleton();
// }
//
//
// TSharedRef<SWidget> FAnimSequenceToolkitsModule::ConstructSequenceBrowser()
// {
// 	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
//
// 	// Configure filter for asset picker
// 	FARFilter Filter;
// 	Filter.bRecursiveClasses = true;
// 	Filter.ClassNames.Add(UAnimSequence::StaticClass()->GetFName());
// 	// Filter.ClassNames.Add(USoundWave::StaticClass()->GetFName());
//
// 	FAssetPickerConfig Config;
// 	Config.Filter = Filter;
// 	Config.InitialAssetViewType = EAssetViewType::Column;
// 	Config.bAddFilterUI = true;
// 	Config.bShowPathInColumnView = true;
// 	Config.bSortByPathInColumnView = true;
// 	Config.GetCurrentSelectionDelegates.Add(&GetCurrentSelectionDelegate);
//
// 	// Configure response to click
// 	Config.OnAssetSelected = FOnAssetSelected::CreateRaw(
// 		this, &FAnimSequenceToolkitsModule::OnAnimSequenceSelected);
// 	Config.bFocusSearchBoxWhenOpened = false;
// 	Config.DefaultFilterMenuExpansion = EAssetTypeCategories::Animation;
// 	Config.SaveSettingsName = "SequenceBrowser";
//
// 	SequencePickerWidget = ContentBrowserModule.Get().CreateAssetPicker(Config);
// 	return SNew(SBorder)[SequencePickerWidget.ToSharedRef()];
// }
//
// // TSharedRef<SWidget> FAnimSequenceToolkitsModule::ConstructBatchOperation()
// // {
// // 	
// // }
//
//
// TSharedRef<SWidget> FAnimSequenceToolkitsModule::ConstructBonePickerWidget()
// {
// 	return SNew(SBorder)
// 	       [
// 		       SAssignNew(BonePickerWidget, SVerticalBox)
// 		       + SVerticalBox::Slot()
// 		         .AutoHeight()
// 		         .VAlign(VAlign_Center)
// 		       [
// 			       SNew(STextBlock)
// 			       .Text(FText::FromString("Target Bone Name"))
// 			       .Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
// 		       ]
// 		       + SVerticalBox::Slot()
// 		         .AutoHeight()
// 		         .VAlign(VAlign_Center)
// 		         .Padding(5, 0, 0, 0)
// 		       [
// 			       SNew(SEditableTextBox)
// 			       .MinDesiredWidth(50)
// 			       .Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
// 			       .Text_Lambda([this]() -> FText { return TargetBoneText; })
// 			       .OnTextCommitted_Lambda([this](const FText& InText, ETextCommit::Type CommitInfo) { TargetBoneText = InText; })
// 		       ]
// 	       ];
// }
//
// TSharedRef<SWidget>
// FAnimSequenceToolkitsModule::ConstructAnimSequenceListWidget()
// {
// 	auto MakeAnimSequenceTableRow =
// 		[](UAnimSequence* InSeq,
// 		const TSharedRef<STableViewBase>& Owner) -> TSharedRef<ITableRow>
// 	{
// 		return SNew(STableRow<UAnimSequence*>, Owner)
// 		       .Padding(FMargin(16, 4, 16, 4))
// 		       [
// 			       SNew(STextBlock)
// 			       .Text(FText::FromString(InSeq->GetName()))
// 		       ];
// 	};
//
// 	auto ScrollBar = SNew(SScrollBar);
// 	auto HeaderRowWidget =
// 		SNew(SHeaderRow)
// 		+ SHeaderRow::Column("Current AnimSequence Selections")
// 		.FillWidth(0.80f)
// 		[
// 			SNew(STextBlock)
// 			.Text(FText::FromString("Selected AnimSequences"))
// 			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
// 		];
//
// 	return SNew(SBorder)
// 	       .BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
// 	       [
// 		       SNew(SVerticalBox)
// 		       + SVerticalBox::Slot()
// 		       .FillHeight(1.0f)
// 		       [
// 			       SNew(SBorder)
// 			       .BorderImage(FEditorStyle::GetBrush("MessageLog.ListBorder"))
// 			       [
// 				       SNew(SScrollBox)
// 				       + SScrollBox::Slot()
// 				       [
// 					       SAssignNew(SequencesListView, SListView<UAnimSequence*>)
// 					       .ItemHeight(40)
// 					       .ListItemsSource(&AnimSequences)
// 					       .OnGenerateRow_Lambda(MakeAnimSequenceTableRow)
// 					       .HeaderRow(HeaderRowWidget)
// 				       ]
// 			       ]
// 		       ]
// 		       + SVerticalBox::Slot()
// 		         .AutoHeight()
// 		         .VAlign(VAlign_Bottom)
// 		       [
// 			       SNew(SButton)
// 			       .HAlign(HAlign_Center)
// 			       .Text(FText::FromString("Add from Browser"))
// 			       .OnClicked_Raw(this, &FAnimSequenceToolkitsModule::OnSequencesAdd)
// 		       ]
// 		       + SVerticalBox::Slot()
// 		         .AutoHeight()
// 		         .VAlign(VAlign_Bottom)
// 		       [
// 			       SNew(SButton)
// 			       .HAlign(HAlign_Center)
// 			       .Text(FText::FromString("Remove Selected"))
// 			       .OnClicked_Raw(this, &FAnimSequenceToolkitsModule::OnSequencesClear)
// 		       ]
// 	       ];
// }
//
// TSharedRef<SWidget> FAnimSequenceToolkitsModule::ConstructPathPickerWidget()
// {
// 	auto MakePathPickerWidget = [&, this]()
// 	{
// 		const auto& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
// 		FPathPickerConfig PathPickerConfig;
// 		PathPickerConfig.OnPathSelected = FOnPathSelected::CreateLambda([this](const FString& NewPath) { ExportPath = NewPath; });
// 		PathPickerConfig.DefaultPath = "/Game";
// 		return ContentBrowserModule.Get().CreatePathPicker(PathPickerConfig);
// 	};
//
// 	return SNew(SBorder)
// 	       [
// 		       SAssignNew(PathPickerWidget, SVerticalBox)
// 		       + SVerticalBox::Slot()
// 		       .AutoHeight()
// 		       [
// 			       SNew(STextBlock)
// 			       .Text(FText::FromString("Output Path"))
// 			       .Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
// 		       ]
// 		       + SVerticalBox::Slot()
// 		       .AutoHeight()
// 		       [
// 			       SNew(SComboButton)
// 			       .OnGetMenuContent_Lambda(MakePathPickerWidget)
// 			       .ButtonContent()
// 			       [
// 				       SNew(STextBlock)
// 				       .Text_Lambda([this]() { return FText::FromString(ExportPath); })
// 			       ]
// 		       ]
// 	       ];
// 	// return PathPickerWidget.ToSharedRef();
// }
//
// TSharedRef<SWidget>
// FAnimSequenceToolkitsModule::ConstructCurveExtractorWidget()
// {
// 	return SNew(SVerticalBox)
// 	       + SVerticalBox::Slot()
// 	         .AutoHeight()
// 	         .VAlign(VAlign_Top)
// 	       [
// 		       SNew(STextBlock)
// 		       .Text(FText::FromString("Animation Curve Extractor"))
// 		       .Font(FCoreStyle::GetDefaultFontStyle("Regular", 20))
// 	       ]
// 	       + SVerticalBox::Slot()
// 	       [
// 		       ConstructAnimSequenceListWidget()
// 	       ]
// 	       + SVerticalBox::Slot()
// 	         .AutoHeight()
// 	         .VAlign(VAlign_Bottom)
// 	       [
// 		       ConstructBonePickerWidget()
// 	       ]
// 	       + SVerticalBox::Slot()
// 	         .AutoHeight()
// 	         .VAlign(VAlign_Bottom)
// 	       [
// 		       ConstructPathPickerWidget()
// 	       ]
// 	       + SVerticalBox::Slot()
// 	         .AutoHeight()
// 	         .VAlign(VAlign_Bottom)
// 	       [
// 		       SNew(SButton)
// 				.HAlign(HAlign_Center)
// 				.Text(FText::FromString("Export Curve"))
// 				.OnClicked_Lambda([this]()
// 		                    {
// 			                    for (auto Seq : AnimSequences)
// 			                    {
// 				                    SaveBonesCurves(Seq, TargetBoneText.ToString(), ExportPath);
// 			                    }
// 			                    return FReply::Handled();
// 		                    })
// 	       ]
// 	       + SVerticalBox::Slot()
// 	         .AutoHeight()
// 	         .VAlign(VAlign_Bottom)
// 	       [
// 		       SNew(SButton)
// 		       .HAlign(HAlign_Center)
// 		       .Text(FText::FromString("Set Footsteps"))
// 		       .OnClicked_Lambda([this]()
// 		       {
// 			       for (auto Seq : AnimSequences)
// 			       {
// 				       // HARDCODE: set LeftHand as Pivot
// 				       MarkFootstepsFor1PAnimation(Seq, "LeftHand");
// 			       }
// 			       return FReply::Handled();
// 		       })
// 	       ];
// }

TSharedRef<SDockTab> FAnimSequenceToolkitsModule::OnSpawnPluginTab(
	const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
	       .TabRole(ETabRole::NomadTab)
	       [

				SNew(SAnimSequenceToolWidget)
		       // SNew(SBorder)
		       // [
			      //  SNew(SHorizontalBox)
			      //  + SHorizontalBox::Slot()
			      //    .FillWidth(1)
			      //    .Padding(2, 0, 2, 0)
			      //  [
				     //   ConstructCurveExtractorWidget()
			      //  ]
			      //  + SHorizontalBox::Slot()
			      //  .FillWidth(2)
			      //  [
				     //   ConstructSequenceBrowser()
			      //  ]
			      //  // ] + SHorizontalBox::Slot().AutoWidth()[
			      //  // 	ConstructBoneTreeList()
			      //  // ]
		       // ]
	       ];
}

// FReply FAnimSequenceToolkitsModule::OnSequencesAdd()
// {
// 	TArray<FAssetData> SelectedAssets = GetCurrentSelectionDelegate.Execute();
// 	for (FAssetData& d : SelectedAssets)
// 	{
// 		UAnimSequence* Anim = Cast<UAnimSequence>(d.GetAsset());
// 		if (Anim && AnimSequences.Find(Anim) == INDEX_NONE)
// 		{
// 			AnimSequences.Add(Anim);
// 		}
// 	}
// 	SequencesListView->RequestListRefresh();
// 	return FReply::Handled();
// }
//
// FReply FAnimSequenceToolkitsModule::OnSequencesClear()
// {
// 	auto SelectedItems = SequencesListView->GetSelectedItems();
// 	for (auto& Item : SelectedItems)
// 	{
// 		AnimSequences.Remove(Item);
// 	}
// 	SequencesListView->RequestListRefresh();
// 	// AnimSequences.Reset();
// 	return FReply::Handled();
// }

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
