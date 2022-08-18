#pragma once

#include "IDetailCustomization.h"
#include "IDetailsView.h"
#include "Animation/AnimSequence.h"

#include "Widgets/SCompoundWidget.h"
#include "AnimToolSettings.h"


class SAnimCurveToolWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SAnimCurveToolWidget)
        {
        }

    SLATE_END_ARGS()

public:
    SAnimCurveToolWidget()
    {
    }

    ~SAnimCurveToolWidget()
    {
    }

    void Construct(const FArguments& Args);

    template <class T>
    TSharedPtr<IDetailsView> CreateSettingView(FString Name, T* SettingViewObject);

private:
    static bool IsPropertyVisible(const FPropertyAndParent& PropertyAndParent, bool bInShouldShowNonEditable);

private:
    void Setup();

    TSharedRef<SWidget> Content();
    
    void ProcessAnimSequencesFilter();

    FReply OnSubmitMarkFootsteps();

    FReply OnSubmitExtractCurves();

    FReply OnSubmitGenerateJson();
    
    FReply OnDocumentButtonClick();

    FReply OnSubmitLoadJson();

    FReply OnSubmitCheckAnimation();
    void CheckCameraRootAtOrigin();
    void CheckSingleFrameAnimation();

    bool LoadFromAnimJson(const FString& JsonName);


private:
    UAnimCurveSettings* AnimCurveSetting;
    TSharedPtr<IDetailsView> AnimCurveSettingView;

    UFootstepSettings* FootstepSetting;
    TSharedPtr<IDetailsView> FootstepSettingView;

    UAnimSequenceSelection* SequenceSelection;
    TSharedPtr<IDetailsView> SequenceSelectionView;

    UAnimJsonSettings* JsonSetting;
    TSharedPtr<IDetailsView> JsonSettingView;
    
    UAnimCheckSettings* CheckSetting;
    TSharedPtr<IDetailsView> CheckSettingView;
    
    TMap<EAnimFilterType, TFunction<bool(TArray<FString> const &, FString const&)>> CheckRegistryTable;

public:
};
