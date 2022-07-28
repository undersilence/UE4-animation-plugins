#include "AnimCurveToolWidget.h"
#include "PropertyEditing.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "AnimCurveUtils.h"
#include "EngineUtils.h"
#include "Dom/JsonObject.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Tools/UEdMode.h"

DEFINE_LOG_CATEGORY_STATIC(LogAnimCurveTool, Log, All);

#define LOCTEXT_NAMESPACE "FAnimCurveToolModule"

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

bool UAnimJsonSettings::IsInitialized = false;
UAnimJsonSettings* UAnimJsonSettings::DefaultSetting = nullptr;

void UAnimJsonSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
}

template <typename T>
TSharedPtr<IDetailsView> SAnimCurveToolWidget::CreateSettingView(FString Name, T* SettingViewObject)
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
    ObjectSettingView->SetIsPropertyVisibleDelegate(
        FIsPropertyVisible::CreateStatic(&SAnimCurveToolWidget::IsPropertyVisible, true));
    ObjectSettingView->SetDisableCustomDetailLayouts(true);
    ObjectSettingView->SetObject(SettingViewObject);
    return ObjectSettingView;
}

void SAnimCurveToolWidget::Construct(const FArguments& Args)
{
    Setup();

    ChildSlot[
        Content()
    ];
}

bool SAnimCurveToolWidget::IsPropertyVisible(const FPropertyAndParent& PropertyAndParent, bool bInShouldShowNonEditable)
{
    const FString VisualCategoryNames[] =
    {
        "SequenceSelection",
        "CurveSetting",
        "FootstepSetting",
        "JsonGeneration"
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

void SAnimCurveToolWidget::Setup()
{
    AnimCurveSetting = UAnimCurveSettings::Get();
    AnimCurveSetting->m_ParentWidget = this;

    FootstepSetting = UFootstepSettings::Get();
    FootstepSetting->m_ParentWidget = this;

    SequenceSelection = UAnimSequenceSelection::Get();
    SequenceSelection->m_ParentWidget = this;

    JsonSetting = UAnimJsonSettings::Get();
    JsonSetting->m_ParentWidget = this;
}

TSharedRef<SWidget> SAnimCurveToolWidget::Content()
{
    AnimCurveSettingView = CreateSettingView<UAnimCurveSettings>(TEXT("Resources"), UAnimCurveSettings::Get());
    FootstepSettingView = CreateSettingView<UFootstepSettings>(TEXT("Resources"), UFootstepSettings::Get());
    SequenceSelectionView = CreateSettingView<UAnimSequenceSelection>(TEXT("Resources"), UAnimSequenceSelection::Get());
    JsonSettingView = CreateSettingView<UAnimJsonSettings>(TEXT("Resources"), UAnimJsonSettings::Get());

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

                + SVerticalBox::Slot()
                  .AutoHeight()
                  .Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
                [
                    SNew(SHorizontalBox) + SHorizontalBox::Slot().VAlign(VAlign_Top)
                    [
                        JsonSettingView->AsShared()
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
				.OnClicked(this, &SAnimCurveToolWidget::OnSubmitExtractCurves)
        ]

        + SHorizontalBox::Slot()
          .AutoWidth()
          .HAlign(HAlign_Center)
          .VAlign(VAlign_Center)
          .Padding(4, 0, 0, 0)
        [
            SNew(SButton)
				.Text(LOCTEXT("Mark_Footsteps", "Mark Footsteps"))
				.OnClicked(this, &SAnimCurveToolWidget::OnSubmitMarkFootsteps)
        ]

        //+ SHorizontalBox::Slot()
        //	.AutoWidth()
        //	.HAlign(HAlign_Center)
        //	.VAlign(VAlign_Center)
        //	.Padding(4, 0, 0, 0)
        //[
        //	SNew(SButton)
        //	.Text(LOCTEXT("Debug", "Debug"))
        //	.OnClicked(this, &SAnimCurveToolWidget::OnDebugButtonClick)
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
						.Text(LOCTEXT("Camera_Root_Check", "Check Camera_Root"))
					// .ButtonColorAndOpacity(FLinearColor(0.2f, 1.0f, 0.2f))
						.OnClicked(this, &SAnimCurveToolWidget::OnSubmitCheckCameraRoot)
        ]

        + SHorizontalBox::Slot()
          .AutoWidth()
          .HAlign(HAlign_Center)
          .VAlign(VAlign_Center)
          .Padding(4, 0, 0, 0)
        [
            SNew(SButton)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Text(LOCTEXT("Generate_Json", "Generate Json"))
			// .ButtonColorAndOpacity(FLinearColor(0.2f, 1.0f, 0.2f))
			    .OnClicked(this, &SAnimCurveToolWidget::OnSubmitGenerateJson)
        ]

        + SHorizontalBox::Slot()
          .AutoWidth()
          .HAlign(HAlign_Center)
          .VAlign(VAlign_Center)
          .Padding(4, 0, 0, 0)
        [
            SNew(SButton)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Text(LOCTEXT("Load_Json", "Load from Json"))
				.OnClicked(this, &SAnimCurveToolWidget::OnSubmitLoadJson)
        ]
    ];

    TSharedRef<SWidget> MainContent = SNew(SVerticalBox) + PropertyPannel + ButtonChannel;

    return MainContent;
}

void SAnimCurveToolWidget::ProcessAnimSequencesFilter()
{
    if (SequenceSelection->IsImportFromJson)
    {
        LoadFromAnimJson(SequenceSelection->AnimJsonPath);
    }
    SequenceSelection->AnimationSequences.RemoveAll([](const auto Ptr)
    {
        return Ptr == nullptr;
    });
}


FReply SAnimCurveToolWidget::OnSubmitMarkFootsteps()
{
    ProcessAnimSequencesFilter();
    SequenceSelection->ErrorSequences.Empty();
    for (auto Seq : SequenceSelection->AnimationSequences)
    {
        bool IsNoError = false;
        for (auto& BoneName : FootstepSetting->TrackBoneNames)
        {
            if (!FAnimCurveUtils::MarkFootstepsFor1PAnimation(Seq, BoneName, FootstepSetting->IsEnableDebug))
            {
                // UE_LOG(LogAnimCurveTool, Log, TEXT("[%s->%s] may not be suitable for footstep recognition, skipped."), *Seq->GetName(), *BoneName);
            }
            else
            {
                UE_LOG(LogAnimCurveTool, Log, TEXT("[%s->%s] attempt to mark footsteps success!"), *Seq->GetName(),
                       *BoneName);
                IsNoError = true;
                break;
            }
        }
        if (!IsNoError)
        {
            SequenceSelection->ErrorSequences.Add(Seq);
        }
    }
    return FReply::Handled();
}

FReply SAnimCurveToolWidget::OnSubmitExtractCurves()
{
    ProcessAnimSequencesFilter();
    for (auto Seq : SequenceSelection->AnimationSequences)
    {
        if (!FAnimCurveUtils::SaveBonesCurves(Seq, AnimCurveSetting->TargetBoneName,
                                              AnimCurveSetting->ExportDirectoryPath.Path))
        {
            UE_LOG(LogAnimCurveTool, Warning, TEXT("[%s->%s]: Error ocurrs when save bone curves!"), *Seq->GetName(),
                   *AnimCurveSetting->TargetBoneName);
        }
    }
    return FReply::Handled();
}

FReply SAnimCurveToolWidget::OnSubmitGenerateJson()
{
    TArray<UAnimSequence*> AnimSequences;
    FAnimCurveUtils::GetObjectsOfClass<UAnimSequence>(AnimSequences);
    AnimSequences.RemoveAll([&](auto Seq)
    {
        FString SeqName = Seq->GetName();
        bool IsRequired = true;

        for (auto& Key : JsonSetting->KeywordsMustHave)
        {
            IsRequired &= SeqName.Contains(Key);
            // UE_LOG(LogAnimCurveTool, Log, TEXT("Check if %s contains %s, %d"), *SeqName, *Key, IsRequired);
        }

        bool IsContainsAny = false;
        for (auto& Key : JsonSetting->IncludedKeywords)
        {
            IsContainsAny |= SeqName.Contains(Key);
        }
        IsRequired &= IsContainsAny;

        for (auto& Key : JsonSetting->ExcludedKeywords)
        {
            IsRequired &= !SeqName.Contains(Key);
        }

        for (auto& Key : JsonSetting->ExcludedPrefix)
        {
            IsRequired &= !SeqName.StartsWith(Key);
        }

        // UE_LOG(LogTemp, Log, TEXT("%s is ok"), *SeqName);
        return !IsRequired;
    });

    TArray<TSharedPtr<FJsonValue>> SequencesRefs;
    for (const auto Seq : AnimSequences)
    {
        auto RefString = FString::Format(TEXT("{0}'{1}'"), {TEXT("AnimSequence"), Seq->GetPathName()});
        SequencesRefs.Push(MakeShared<FJsonValueString>(RefString));
    }
    // Construct Json Object
    TSharedPtr<FJsonObject> JsonObj = MakeShared<FJsonObject>();
    JsonObj->SetArrayField(TEXT("AnimSequences"), SequencesRefs);

    typedef TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>> FPrettyJsonStringWriterFactory;
    typedef TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>> FPrettyJsonStringWriter;

    FString OutputString;
    TSharedRef<FPrettyJsonStringWriter> Writer = FPrettyJsonStringWriterFactory::Create(&OutputString);
    if (FJsonSerializer::Serialize(JsonObj.ToSharedRef(), Writer))
    {
        auto AbsPath =
            FPaths::ConvertRelativePathToFull(JsonSetting->ExportDirectoryPath.Path + TEXT("anim_list.json"));
        if (FFileHelper::SaveStringToFile(OutputString, *AbsPath,
                                          FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(),
                                          FILEWRITE_None))
        {
            UE_LOG(LogAnimCurveTool, Log, TEXT("Save Json to '%s' Success!"), *AbsPath);
        }
    }

    return FReply::Handled();
}

FReply SAnimCurveToolWidget::OnSubmitLoadJson()
{
    if (SequenceSelection->IsImportFromJson)
    {
        if (!LoadFromAnimJson(SequenceSelection->AnimJsonPath))
        {
            UE_LOG(LogAnimCurveTool, Warning, TEXT("Load Json error from %s, file not found or illegal Json file"),
                   *SequenceSelection->AnimJsonPath.FilePath)
        }
    } 
    return FReply::Handled();
}

FReply SAnimCurveToolWidget::OnSubmitCheckCameraRoot()
{
    ProcessAnimSequencesFilter();
    SequenceSelection->ErrorSequences.Empty();
    for (auto Seq : SequenceSelection->AnimationSequences)
    {
        TArray<FVector> PosKeys;
        TArray<FQuat> RotKeys;
        if (FAnimCurveUtils::GetBoneKeysByNameHelper(Seq, "Camera_Root", PosKeys, RotKeys, true))
        {
            float Eps = 1e-10;
            auto Sign = [=](float x) -> int { return (x > Eps) - (x < Eps); };

            if (Sign(PosKeys[0].X) != 0 || Sign(PosKeys[0].Y) != 0 || Sign(PosKeys[0].Z) != 0)
            {
                UE_LOG(LogAnimCurveTool, Warning, TEXT("[%s->%s] Camera_Root not start at Origin(0, 0, 0)!"),
                       *Seq->GetName(),
                       TEXT("Camera_Root"));
                SequenceSelection->ErrorSequences.Push(Seq);
            }
            else if (Sign(PosKeys.Last().X) != 0 || Sign(PosKeys.Last().Y) != 0 || Sign(PosKeys.Last().Z) != 0)
            {
                UE_LOG(LogAnimCurveTool, Warning, TEXT("[%s->%s] Camera_Root not end at Origin(0, 0, 0)!"),
                       *Seq->GetName(),
                       TEXT("Camera_Root"));
                SequenceSelection->ErrorSequences.Push(Seq);
            }
        }
    }
    return FReply::Handled();
}


bool SAnimCurveToolWidget::LoadFromAnimJson(const FFilePath& JsonName)
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

    TArray<FString> AnimSequencePaths;

    // for (auto GunTypeIter = JsonObj->Values.CreateConstIterator(); GunTypeIter; ++GunTypeIter)
    // {
    // 	UE_LOG(LogAnimCurveTool, Log, TEXT("Parse GunType %s"), *GunTypeIter->Key);
    // 	const auto GunTypeObj = GunTypeIter->Value->AsObject();
    // 	for (auto GunNameIter = GunTypeObj->Values.CreateConstIterator(); GunNameIter; ++GunNameIter)
    // 	{
    // 		UE_LOG(LogAnimCurveTool, Log, TEXT("Parse GunName %s"), *GunNameIter->Key);
    // 		auto GunSequences = GunNameIter->Value->AsArray();
    // 		for (const auto& GunSeq : GunSequences)
    // 		{
    // 			UE_LOG(LogAnimCurveTool, Log, TEXT("Parse Gun AnimSequence %s"), *GunSeq->AsString());
    // 			AnimSequencePaths.Add(GunSeq->AsString());
    // 		}
    // 	}
    // }

    auto AnimSequenceJsonArray = JsonObj->GetArrayField("AnimSequences");
    for (auto& SeqJson : AnimSequenceJsonArray)
    {
        AnimSequencePaths.Add(SeqJson->AsString());
    }

    return FAnimCurveUtils::LoadAnimSequencesByReference(AnimSequencePaths, SequenceSelection->AnimationSequences);
}
