#pragma once

#include "Engine/EngineTypes.h"
#include "Layout/WidgetPath.h"
#include "Widgets/SWidget.h"

#include "AnimToolSettings.generated.h"

class UAnimSequence;

/*
     * @zellitu: the reason why I hardcoded these certain rules, well,
     * it just enough to cover all situations while use RegEx seems a
     * little bit over complex under this use case.
     */
UENUM()
enum class EAnimFilterType : uint8
{
    Not_Any,
    // <1
    Least_One,
    // >=1
    Have_All,
    // ==N
    Not_Start,
    // not starts with any one 
    Start_With,
    // starts with least one
    Not_End,
    End_With,
};

USTRUCT()
struct FAnimRuleFilter
{
    GENERATED_USTRUCT_BODY()
    
    UPROPERTY(EditAnywhere, Category= JsonGeneration)
    EAnimFilterType FilterType = {EAnimFilterType::Least_One};
    UPROPERTY(EditAnywhere, Category= JsonGeneration)
    TArray<FString> Keywords = {};
    
    template <EAnimFilterType TFilter>
   
    static bool MatchAnimName(TArray<FString> const& Keywords, FString const& Input)
    {
        if constexpr (TFilter == EAnimFilterType::Not_Any || TFilter == EAnimFilterType::Least_One)
        {
            // these are inverse pairs
            for (auto const& Key : Keywords)
            {
                if (Input.Contains(Key))
                {
                    return TFilter == EAnimFilterType::Least_One;
                }
            }
            return TFilter == EAnimFilterType::Not_Any;
        }
        else if constexpr (TFilter == EAnimFilterType::Have_All)
        {
            for (auto const& Key : Keywords)
            {
                if (!Input.Contains(Key))
                {
                    return false;
                }
            }
            return true;
        }
        else if constexpr (TFilter == EAnimFilterType::Start_With || TFilter == EAnimFilterType::Not_Start)
        {
            for (auto const& Key : Keywords)
            {
                if (Input.StartsWith(Key))
                {
                    return TFilter == EAnimFilterType::Start_With;
                }
            }
            return TFilter == EAnimFilterType::Not_Start;
        }
        else if constexpr (TFilter == EAnimFilterType::End_With || TFilter == EAnimFilterType::Not_End)
        {
            for (auto const& Key : Keywords)
            {
                if (Input.EndsWith(Key))
                {
                    return TFilter == EAnimFilterType::End_With;
                }
            }
            return TFilter == EAnimFilterType::Not_Start;
        }
        else return true;
    }
};

UCLASS()
class UAnimCheckSettings : public UObject
{
    GENERATED_BODY()

public:
    UAnimCheckSettings()
    {
    }

    static UAnimCheckSettings* Get()
    {
        if (!IsInitialized)
        {
            DefaultSetting = DuplicateObject(GetMutableDefault<UAnimCheckSettings>(), nullptr);
            DefaultSetting->AddToRoot();
            IsInitialized = true;
        }

        return DefaultSetting;
    }

    static void Destroy()
    {
        if (IsInitialized)
        {
            if (UObjectInitialized() && DefaultSetting)
            {
                DefaultSetting->RemoveFromRoot();
                DefaultSetting->MarkPendingKill();
            }

            DefaultSetting = nullptr;
            IsInitialized = false;
        }
    }

    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

private:
    static bool IsInitialized;
    static UAnimCheckSettings* DefaultSetting; // GCMISMATCH_SKIP_CHECK

public:
    UPROPERTY(EditAnywhere, Category = CheckSetting)
    bool bCheckIfCameraRootAtOrigin {true};
    
    UPROPERTY(EditAnywhere, Category = CheckSetting)
    bool bCheckIfSingleFrameAnim {true};
    
    SWidget* m_ParentWidget;
};



UCLASS()
class UAnimJsonSettings : public UObject
{
    GENERATED_BODY()

public:
    UAnimJsonSettings()
    {
    }

    static UAnimJsonSettings* Get()
    {
        if (!IsInitialized)
        {
            DefaultSetting = DuplicateObject(GetMutableDefault<UAnimJsonSettings>(), nullptr);
            DefaultSetting->AddToRoot();
            IsInitialized = true;
        }

        return DefaultSetting;
    }

    static void Destroy()
    {
        if (IsInitialized)
        {
            if (UObjectInitialized() && DefaultSetting)
            {
                DefaultSetting->RemoveFromRoot();
                DefaultSetting->MarkPendingKill();
            }

            DefaultSetting = nullptr;
            IsInitialized = false;
        }
    }

    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

private:
    static bool IsInitialized;
    static UAnimJsonSettings* DefaultSetting; // GCMISMATCH_SKIP_CHECK
    
public:

    UPROPERTY(EditAnywhere, Category = FilterSetting)
    bool bEnableNameConventionFilter {true};
    
    UPROPERTY(EditAnywhere, Category = FilterSetting, Meta=(EditCondition="bEnableNameConventionFilter ", EditConditionHides))
    TArray<FAnimRuleFilter> AnimRuleFilters {
        {{EAnimFilterType::Least_One}, {"_1P_", "_1p_", "_IP_"}},
        {{EAnimFilterType::Least_One}, {"Crouch", "Walk", "Run", "Sprint"}},
        {{EAnimFilterType::Not_Any}, {"Idle", "Offset", "Wounded", "Prone", "Dying", "Start", "Stop"}},
        {{EAnimFilterType::Not_Start}, {"BS_"}}
    };

    UPROPERTY(EditAnywhere, Category = FilterSetting)
    bool bEnableVariableCurvesFilter {false};
    
    UPROPERTY(EditAnywhere, Category = FilterSetting, Meta=(EditCondition="bEnableVariableCurvesFilter", EditConditionHides))
    TArray<FString> VariableCurvesNames {};
    
    UPROPERTY(EditAnywhere, Category = FilterSetting)
    FDirectoryPath SearchPath{FPaths::ProjectContentDir() };
    
    UPROPERTY(EditAnywhere, Category = FilterSetting)
    FFilePath ExportPath{FPaths::ProjectConfigDir() / "anim_list.json"};

    SWidget* m_ParentWidget;
};

UCLASS()
class UFootstepSettings : public UObject
{
    GENERATED_BODY()

public:
    UFootstepSettings()
    {
    }

    static UFootstepSettings* Get()
    {
        if (!IsInitialized)
        {
            DefaultSetting = DuplicateObject(GetMutableDefault<UFootstepSettings>(), nullptr);
            DefaultSetting->AddToRoot();
            IsInitialized = true;
        }

        return DefaultSetting;
    }

    static void Destroy()
    {
        if (IsInitialized)
        {
            if (UObjectInitialized() && DefaultSetting)
            {
                DefaultSetting->RemoveFromRoot();
                DefaultSetting->MarkPendingKill();
            }

            DefaultSetting = nullptr;
            IsInitialized = false;
        }
    }

    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

private:
    static bool IsInitialized;
    static UFootstepSettings* DefaultSetting; // GCMISMATCH_SKIP_CHECK

public:
    UPROPERTY(EditAnywhere, Category=FootstepSetting)
    TArray<FString> TrackBoneNames = {"LeftHand", "RightHand"};

    UPROPERTY(EditAnywhere, Category=FootstepSetting)
    bool bUseCurve = true;
    
    UPROPERTY(EditAnywhere, Category=FootstepSetting)
    bool IsEnableDebug = false;

    SWidget* m_ParentWidget;
};

UCLASS()
class UAnimCurveSettings : public UObject
{
    GENERATED_BODY()

public:
    UAnimCurveSettings()
    {
    }

    static UAnimCurveSettings* Get()
    {
        if (!IsInitialized)
        {
            DefaultSetting = DuplicateObject(GetMutableDefault<UAnimCurveSettings>(), nullptr);
            DefaultSetting->AddToRoot();
            IsInitialized = true;
        }

        return DefaultSetting;
    }

    static void Destroy()
    {
        if (IsInitialized)
        {
            if (UObjectInitialized() && DefaultSetting)
            {
                DefaultSetting->RemoveFromRoot();
                DefaultSetting->MarkPendingKill();
            }

            DefaultSetting = nullptr;
            IsInitialized = false;
        }
    }

    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
private:
    static bool IsInitialized;
    static UAnimCurveSettings* DefaultSetting; // GCMISMATCH_SKIP_CHECK

public:
    UPROPERTY(EditAnywhere, Category=CurveSetting)
    FString TargetBoneName = "LeftHand";

    UPROPERTY(EditAnywhere, Category=CurveSetting)
    FDirectoryPath ExportDirectoryPath{FPaths::ProjectContentDir()};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CurveSetting)
    bool IsExtractPositionXYZ = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CurveSetting)
    bool IsExtractRotationXYZ = true;

    SWidget* m_ParentWidget;
};

UCLASS()
class UAnimSequenceSelection : public UObject
{
    GENERATED_BODY()

public:
    UAnimSequenceSelection()
    {
    }

    static UAnimSequenceSelection* Get()
    {
        if (!IsInitialized)
        {
            DefaultSetting = DuplicateObject(GetMutableDefault<UAnimSequenceSelection>(), nullptr);
            DefaultSetting->AddToRoot();
            IsInitialized = true;
        }

        return DefaultSetting;
    }

    static void Destroy()
    {
        if (IsInitialized)
        {
            if (UObjectInitialized() && DefaultSetting)
            {
                DefaultSetting->RemoveFromRoot();
                DefaultSetting->MarkPendingKill();
            }

            DefaultSetting = nullptr;
            IsInitialized = false;
        }
    }

    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
private:
    static bool IsInitialized;
    static UAnimSequenceSelection* DefaultSetting; // GCMISMATCH_SKIP_CHECK

public:
    // UPROPERTY(EditAnywhere, Category=SequenceSelection)
    // bool IsImportFromJson = false;
    //
    // UPROPERTY(EditAnywhere, Category=SequenceSelection, Meta=(EditCondition="IsImportFromJson", EditConditionHides))
    // FFilePath AnimJsonPath;

    UPROPERTY(EditAnywhere, Category=SequenceSelection)
    TArray<UAnimSequence*> AnimationSequences;

    UPROPERTY(VisibleAnywhere, Category=SequenceSelection)
    TArray<UAnimSequence*> ErrorSequences;

    SWidget* m_ParentWidget;
};