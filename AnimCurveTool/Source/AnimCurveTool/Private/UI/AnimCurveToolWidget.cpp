#include "AnimCurveToolWidget.h"

#include "AnimationBlueprintLibrary.h"
#include "DesktopPlatformModule.h"
#include "EditorDirectories.h"
#include "PropertyEditing.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Util/AnimCurveUtils.h"
#include "EngineUtils.h"
#include "IDesktopPlatform.h"
#include "Dom/JsonObject.h"
#include "Framework/Application/SlateApplication.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/FileHelper.h"
#include "Misc/MessageDialog.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Widgets/Text/STextBlock.h"

DEFINE_LOG_CATEGORY_STATIC(LogAnimCurveTool, Log, All);

#define LOCTEXT_NAMESPACE "FAnimCurveToolModule"

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
        "FilterSetting",
        "CheckSetting"
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

#define REGISTER_ANIM_NAME_CHECK(Check_Type) \
    CheckRegistryTable.Emplace(Check_Type, &FAnimRuleFilter::MatchAnimName<Check_Type>)


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

    CheckSetting = UAnimCheckSettings::Get();
    CheckSetting->m_ParentWidget = this;

    REGISTER_ANIM_NAME_CHECK(EAnimFilterType::Least_One);
    REGISTER_ANIM_NAME_CHECK(EAnimFilterType::Have_All);
    REGISTER_ANIM_NAME_CHECK(EAnimFilterType::Not_Any);

    REGISTER_ANIM_NAME_CHECK(EAnimFilterType::End_With);
    REGISTER_ANIM_NAME_CHECK(EAnimFilterType::Not_End);
    REGISTER_ANIM_NAME_CHECK(EAnimFilterType::Start_With);
    REGISTER_ANIM_NAME_CHECK(EAnimFilterType::Not_Start);
}

TSharedRef<SWidget> SAnimCurveToolWidget::Content()
{
    AnimCurveSettingView = CreateSettingView<UAnimCurveSettings>(TEXT("Resources"), UAnimCurveSettings::Get());
    FootstepSettingView = CreateSettingView<UFootstepSettings>(TEXT("Resources"), UFootstepSettings::Get());
    SequenceSelectionView = CreateSettingView<UAnimSequenceSelection>(TEXT("Resources"), UAnimSequenceSelection::Get());
    JsonSettingView = CreateSettingView<UAnimJsonSettings>(TEXT("Resources"), UAnimJsonSettings::Get());
    CheckSettingView = CreateSettingView<UAnimCheckSettings>(TEXT("Resources"), UAnimCheckSettings::Get());

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

                + SVerticalBox::Slot()
                  .AutoHeight()
                  .Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
                [
                    SNew(SHorizontalBox) + SHorizontalBox::Slot().VAlign(VAlign_Top)
                    [
                        CheckSettingView->AsShared()
                    ]
                ]
            ]
        ]
    ];

    SVerticalBox::FSlot& ButtonChannel1 = SVerticalBox::Slot()
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
						.Text(LOCTEXT("Camera_Animation", "Check Animation"))
					// .ButtonColorAndOpacity(FLinearColor(0.2f, 1.0f, 0.2f))
						.OnClicked(this, &SAnimCurveToolWidget::OnSubmitCheckAnimation)
        ]

    ];

    SVerticalBox::FSlot& ButtonChannel2 = SVerticalBox::Slot()
                                          .AutoHeight()
                                          .Padding(2.0f, 1.0f)
                                          .VAlign(VAlign_Bottom)[

        SNew(SHorizontalBox) + SHorizontalBox::Slot()
                               .AutoWidth()
                               .HAlign(HAlign_Center)
                               .VAlign(VAlign_Center)
                               .Padding(4, 0, 0, 0)
        [
            SNew(SButton)
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
             .Text(LOCTEXT("Load_Json", "Load from Json"))
             .OnClicked(this, &SAnimCurveToolWidget::OnSubmitLoadJson)
        ]

        + SHorizontalBox::Slot()
          .AutoWidth()
          .HAlign(HAlign_Center)
          .VAlign(VAlign_Center)
          .Padding(4, 0, 0, 0)
        [
            SNew(SButton)
             .Text(LOCTEXT("Open_Document", "Open Document"))
             .ButtonColorAndOpacity(FLinearColor(0.2f, 1.0f, 0.2f))
             .OnClicked(this, &SAnimCurveToolWidget::OnDocumentButtonClick)
        ]
    ];

    TSharedRef<SWidget> MainContent = SNew(SVerticalBox) + PropertyPannel + ButtonChannel1 + ButtonChannel2;

    return MainContent;
}

void SAnimCurveToolWidget::ProcessAnimSequencesFilter()
{
    // if (SequenceSelection->IsImportFromJson)
    // {
    //     LoadFromAnimJson(SequenceSelection->AnimJsonPath);
    // }

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
        if (!FAnimCurveUtils::MarkFootstepsFor1PAnimation(Seq, FootstepSetting->TrackBoneNames,
                                                          FootstepSetting->IsEnableDebug))
        {
            SequenceSelection->ErrorSequences.Add(Seq);
            UE_LOG(LogAnimCurveTool, Log, TEXT("[%s] may not be suitable for footstep recognition."), *Seq->GetName());
        }
    }
    return FReply::Handled();
}

FReply SAnimCurveToolWidget::OnSubmitExtractCurves()
{
    ProcessAnimSequencesFilter();
    for (auto Seq : SequenceSelection->AnimationSequences)
    {
        uint32 SaveFlags = 0;
        SaveFlags |= AnimCurveSetting->IsExtractPositionXYZ ? 0xf0 : 0x00;
        SaveFlags |= AnimCurveSetting->IsExtractRotationXYZ ? 0x0f : 0x00;
        if (!FAnimCurveUtils::SaveBonesCurves(Seq, AnimCurveSetting->TargetBoneName,
                                              AnimCurveSetting->ExportDirectoryPath.Path, SaveFlags))
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
    FAnimCurveUtils::GetAnimAssets(JsonSetting->SearchPath.Path, AnimSequences);

    if (JsonSetting->bEnableNameConventionFilter)
    {
        for (auto& Filter : JsonSetting->AnimRuleFilters)
        {
            AnimSequences.RemoveAll(
                [&](auto Seq) -> bool
                {
                    auto IsRequired = CheckRegistryTable[Filter.FilterType](Filter.Keywords, Seq->GetName());
                    // if(!IsRequired)
                    // {
                    //     UE_LOG(LogAnimCurveTool, Log, TEXT("Remove %s, due to rule %s"), *Seq->GetName(), *Filter.Keywords[0]);
                    // }
                    return !IsRequired;
                });
        }
    }

    if (JsonSetting->bEnableVariableCurvesFilter)
    {
        for (auto& CurveName : JsonSetting->VariableCurvesNames)
        {
            AnimSequences.RemoveAll(
                [&](auto Seq) -> bool
                {
                    return !UAnimationBlueprintLibrary::DoesCurveExist(Seq, *CurveName, ERawCurveTrackTypes::RCT_MAX);
                });
        }
    }

    TArray<TSharedPtr<FJsonValue>> SequencesRefs;
    for (const auto Seq : AnimSequences)
    {
        FString PackagePath; // = FString::Format(TEXT("{0}'{1}'"), {TEXT("AnimSequence"), Seq->GetPathName()});
        if (FPackageName::TryConvertFilenameToLongPackageName(Seq->GetPathName(), PackagePath))
        {
            SequencesRefs.Push(MakeShared<FJsonValueString>(PackagePath));
        }
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
            FPaths::ConvertRelativePathToFull(JsonSetting->ExportPath.FilePath);
        if (FFileHelper::SaveStringToFile(OutputString, *AbsPath,
                                          FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(),
                                          FILEWRITE_None))
        {
            UE_LOG(LogAnimCurveTool, Log, TEXT("Save Json to '%s' Success!"), *AbsPath);
        }
    }

    return FReply::Handled();
}

FReply SAnimCurveToolWidget::OnDocumentButtonClick()
{
    FPlatformProcess::LaunchURL(
        TEXT("https://iwiki.woa.com/pages/viewpage.action?pageId=2006664229"), nullptr, nullptr);
    return FReply::Handled();
}

FReply SAnimCurveToolWidget::OnSubmitLoadJson()
{
    // Prompt the user for the filenames
    TArray<FString> OpenFilenames;
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    bool bOpened = false;
    int32 FilterIndex = -1;

    if (DesktopPlatform)
    {
        const void* ParentWindowWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);

        bOpened = DesktopPlatform->OpenFileDialog(
            ParentWindowWindowHandle,
            LOCTEXT("ImportDialogTitle", "Import").ToString(),
            FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_IMPORT),
            TEXT(""),
            "AnimSequence list JSON (*.json)|*.json",
            EFileDialogFlags::Multiple,
            OpenFilenames,
            FilterIndex // file type dependent
        );
    }
    if (bOpened)
    {
        FString JsonPath_Succ, JsonPath_Fail;
        for (auto Filename : OpenFilenames)
        {
            if (!LoadFromAnimJson(Filename))
            {
                JsonPath_Fail.Append(Filename).Append("\n");
                UE_LOG(LogAnimCurveTool, Warning, TEXT("Load Json error from %s, file not found or illegal Json file"),
                       *Filename);
            }
        }

        if (JsonPath_Fail.Len())
        {
            FMessageDialog::Open(EAppMsgType::Ok,
                                 FText::Format(
                                     LOCTEXT("Load_Json_Error", "Load Json Complete, but get error when load: \n {0}"),
                                     FText::FromString(JsonPath_Fail)));
        }
    }
    return FReply::Handled();
}

FReply SAnimCurveToolWidget::OnSubmitCheckAnimation()
{
    if (CheckSetting->bCheckIfCameraRootAtOrigin)
    {
        CheckCameraRootAtOrigin();
    }
    if (CheckSetting->bCheckIfSingleFrameAnim)
    {
        CheckSingleFrameAnimation();
    }
    return FReply::Handled();
}

void SAnimCurveToolWidget::CheckCameraRootAtOrigin()
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
}

void SAnimCurveToolWidget::CheckSingleFrameAnimation()
{
    ProcessAnimSequencesFilter();
    SequenceSelection->ErrorSequences.Empty();
    for (auto Seq : SequenceSelection->AnimationSequences)
    {
        if (Seq->GetNumberOfFrames() == 1)
        {
            UE_LOG(LogAnimCurveTool, Warning, TEXT("[%s] only contains one frame!"), *Seq->GetName());
            SequenceSelection->ErrorSequences.Push(Seq);
        }
    }
}

bool SAnimCurveToolWidget::LoadFromAnimJson(const FString& JsonName)
{
    FString FileContents;
    if (!FFileHelper::LoadFileToString(FileContents, *JsonName))
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

#undef LOCTEXT_NAMESPACE
