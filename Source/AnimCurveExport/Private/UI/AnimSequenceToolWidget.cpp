﻿#include "AnimSequenceToolWidget.h"
#include "PropertyEditing.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "AnimSequenceUtils.h"
#include "EngineUtils.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogAnimSequenceUtils, Log, All);

#define LOCTEXT_NAMESPACE "SAnimSequenceTool"

////////////////////////////////////////////////////////////////////////////////////////////////////
// UAnimCurveSettings | Anim Curve settings
////////////////////////////////////////////////////////////////////////////////////////////////////

bool UAnimCurveSettings::IsInitialized = false;
UAnimCurveSettings* UAnimCurveSettings::DefaultSetting = nullptr;

void UAnimCurveSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

bool UFootstepSettings::IsInitialized = false;
UFootstepSettings* UFootstepSettings::DefaultSetting = nullptr;

void UFootstepSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

bool UAnimSequenceSelection::IsInitialized = false;
UAnimSequenceSelection* UAnimSequenceSelection::DefaultSetting = nullptr;

void UAnimSequenceSelection::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

template <typename T>
TSharedPtr<IDetailsView> SAnimSequenceToolWidget::CreateSettingView(FString Name, T* SettingViewObject)
{
	FPropertyEditorModule& EditModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs
		(
			/*bUpdateFromSelection=*/ false,
			/*bLockable=*/ false,
			/*bAllowSearch=*/ false,
			/*InNameAreaSettings=*/ FDetailsViewArgs::HideNameArea,
			/*bHideSelectionTip=*/ true,
			/*InNotifyHook=*/ nullptr,
			/*InSearchInitialKeyFocus=*/ false,
			/*InViewIdentifier=*/ FName(Name.GetCharArray().GetData())
			);
	// DetailsViewArgs.bShowAnimatedPropertiesOption = false;
	DetailsViewArgs.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Automatic;
	DetailsViewArgs.bShowOptions = false;

	TSharedPtr<IDetailsView> ObjectSettingView = EditModule.CreateDetailView(DetailsViewArgs);
	ObjectSettingView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateStatic(&SAnimSequenceToolWidget::IsPropertyVisible, true));
	ObjectSettingView->SetDisableCustomDetailLayouts(true);
	ObjectSettingView->SetObject(SettingViewObject);
	return ObjectSettingView;
}

void SAnimSequenceToolWidget::Construct(const FArguments& Args)
{
	Setup();

	ChildSlot[
		Content()
	];
}

bool SAnimSequenceToolWidget::IsPropertyVisible(const FPropertyAndParent& PropertyAndParent, bool bInShouldShowNonEditable)
{
	const FString VisualCategoryNames[] =
	{
		"SequenceSelection",
		"CurveSetting",
		"FootstepSetting",
	};

	FString PropertyCategoryName = PropertyAndParent.Property.GetMetaData("Category");

	int VisualCategoryNamesCount = sizeof(VisualCategoryNames) / sizeof(VisualCategoryNames[0]);
	for (int nameID = 0; nameID < VisualCategoryNamesCount; ++nameID)
	{
		if (PropertyCategoryName == VisualCategoryNames[nameID])
			return true;
	}

	return false;
}

void SAnimSequenceToolWidget::Setup()
{
	AnimCurveSetting = UAnimCurveSettings::Get();
	AnimCurveSetting->m_ParentWidget = this;

	FootstepSetting = UFootstepSettings::Get();
	FootstepSetting->m_ParentWidget = this;

	SequenceSelection = UAnimSequenceSelection::Get();
	SequenceSelection->m_ParentWidget = this;
}

TSharedRef<SWidget> SAnimSequenceToolWidget::Content()
{
	AnimCurveSettingView = CreateSettingView<UAnimCurveSettings>(TEXT("Resources"), UAnimCurveSettings::Get());
	FootstepSettingView = CreateSettingView<UFootstepSettings>(TEXT("Resources"), UFootstepSettings::Get());
	SequenceSelectionView = CreateSettingView<UAnimSequenceSelection>(TEXT("Resources"), UAnimSequenceSelection::Get());

	SVerticalBox::FSlot& PropertyPannel = SVerticalBox::Slot().Padding(2.0f, 1.0f)
	[
		SNew(SScrollBox) + SScrollBox::Slot()
		[
			SNew(SBorder).BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().VAlign(VAlign_Top)
				                      .AutoHeight()
				                      .Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				[
					SNew(SHorizontalBox) + SHorizontalBox::Slot().VAlign(VAlign_Top)
					[
						SequenceSelectionView->AsShared()
					]
				]

				+ SVerticalBox::Slot().VAlign(VAlign_Top)
				                      .AutoHeight()
				                      .Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				[
					SNew(SHorizontalBox) + SHorizontalBox::Slot().VAlign(VAlign_Top)
					[
						AnimCurveSettingView->AsShared()
					]
				]

				+ SVerticalBox::Slot()
				  .AutoHeight()
				  .Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				[
					SNew(SHorizontalBox) + SHorizontalBox::Slot().VAlign(VAlign_Top)
					[
						FootstepSettingView->AsShared()
					]
				]
			]
		]
	];

	SVerticalBox::FSlot& ButtonChannel = SVerticalBox::Slot()
	                                     .AutoHeight()
	                                     .Padding(2.0f, 1.0f)
	                                     .VAlign(VAlign_Bottom)
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		  .AutoWidth()
		  .HAlign(HAlign_Center)
		  .VAlign(VAlign_Center)
		  .Padding(4, 0, 0, 0)
		[
			SNew(SButton)
				.Text(LOCTEXT("Extract_Curves", "Extract Curves"))
				.OnClicked(this, &SAnimSequenceToolWidget::OnSubmitExtractCurves)
		]

		+ SHorizontalBox::Slot()
		  .AutoWidth()
		  .HAlign(HAlign_Center)
		  .VAlign(VAlign_Center)
		  .Padding(4, 0, 0, 0)
		[
			SNew(SButton)
				.Text(LOCTEXT("Mark_Footsteps", "Mark Footsteps"))
				.OnClicked(this, &SAnimSequenceToolWidget::OnSubmitMarkFootsteps)
		]

		//+ SHorizontalBox::Slot()
		//	.AutoWidth()
		//	.HAlign(HAlign_Center)
		//	.VAlign(VAlign_Center)
		//	.Padding(4, 0, 0, 0)
		//[
		//	SNew(SButton)
		//	.Text(LOCTEXT("Debug", "Debug"))
		//	.OnClicked(this, &SCharacterProcessWindow::OnDebugButtonClick)
		//]

		+ SHorizontalBox::Slot()
		  .AutoWidth()
		  .HAlign(HAlign_Center)
		  .VAlign(VAlign_Center)
		  .Padding(4, 0, 0, 0)
		[
			SNew(SButton)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Text(LOCTEXT("Load_From_Json", "Load From Json"))
			// .ButtonColorAndOpacity(FLinearColor(0.2f, 1.0f, 0.2f))
			// .OnClicked(this, &SCharacterProcessWindow::OnDocumentButtonClick)
		]
	];

	TSharedRef<SWidget> MainContent = SNew(SVerticalBox) + PropertyPannel + ButtonChannel;

	return MainContent;
}

void SAnimSequenceToolWidget::ProcessAnimSequencesFilter()
{
	if (SequenceSelection->IsImportFromJson)
	{
		LoadFromAnim1pJson(SequenceSelection->Anim1pJsonPath);
	}
	SequenceSelection->AnimationSequences.RemoveAll([](const auto Ptr)
	{
		return Ptr == nullptr;
	});
}


FReply SAnimSequenceToolWidget::OnSubmitMarkFootsteps()
{
	ProcessAnimSequencesFilter();
	for (auto Seq : SequenceSelection->AnimationSequences)
	{
		for (auto& BoneName : FootstepSetting->TrackBoneNames)
		{
			if (!AnimSequenceUtils::MarkFootstepsFor1PAnimation(Seq, BoneName, FootstepSetting->IsEnableDebug))
			{
				UE_LOG(LogAnimSequenceUtils, Log, TEXT("[%s->%s] may not be suitable for footstep recognition"), *Seq->GetName(), *BoneName);
			}
		}
	}
	return FReply::Handled();
}

FReply SAnimSequenceToolWidget::OnSubmitExtractCurves()
{
	ProcessAnimSequencesFilter();
	for (auto Seq : SequenceSelection->AnimationSequences)
	{
		if (!AnimSequenceUtils::SaveBonesCurves(Seq, AnimCurveSetting->TargetBoneName, AnimCurveSetting->ExportDirectoryPath.Path))
		{
			UE_LOG(LogAnimSequenceUtils, Log, TEXT("[%s->%s] target bone name not exists!"), *Seq->GetName(), *AnimCurveSetting->TargetBoneName);
		}
	}
	return FReply::Handled();
}

bool SAnimSequenceToolWidget::LoadFromAnim1pJson(const FFilePath& JsonName)
{
	FString FileContents;
	if (!FFileHelper::LoadFileToString(FileContents, *JsonName.FilePath))
	{
		return false;
	}

	TSharedPtr<FJsonObject> JsonObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(FileContents);
	if (!FJsonSerializer::Deserialize(Reader, JsonObj) || !JsonObj.IsValid())
	{
		return false;
	}

	TArray<FFilePath> AnimSequencePaths;
	
	for (auto GunTypeIter = JsonObj->Values.CreateConstIterator(); GunTypeIter; ++GunTypeIter)
	{
		UE_LOG(LogTemp, Log, TEXT("Parse GunType %s"), *GunTypeIter->Key);
		const auto GunTypeObj = GunTypeIter->Value->AsObject();
		for(auto GunNameIter = GunTypeObj->Values.CreateConstIterator(); GunNameIter; ++GunNameIter) {
			UE_LOG(LogTemp, Log, TEXT("Parse GunName %s"), *GunNameIter->Key);
			auto GunSequences = GunNameIter->Value->AsArray();
			for(const auto& GunSeq : GunSequences)
			{
				UE_LOG(LogTemp, Log, TEXT("Parse Gun AnimSequence %s"), *GunSeq->AsString());
				AnimSequencePaths.Add(FFilePath{GunSeq->AsString()});
			}
		} 
	}
	
	return AnimSequenceUtils::LoadAnimSequencesByNames(AnimSequencePaths);
}