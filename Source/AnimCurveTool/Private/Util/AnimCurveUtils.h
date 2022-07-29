#pragma once

#include "CoreMinimal.h"
#include "AssetRegistryModule.h"
#include "Animation/AnimSequence.h"
#include "Curves/CurveVector.h"




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

// Stateless Util Set
class FAnimCurveUtils
{
public:

    struct FFootstepMarker
    {
        float Value; // height
        int Orientation; // -1: left, 1: right
        int Frame;
    };
    
    static void GetAnimAssets(FString const& BaseDir, TArray<UAnimSequence*>& OutArray);

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
        if constexpr (TFilter == EAnimFilterType::Have_All)
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
        if constexpr (TFilter == EAnimFilterType::Start_With || TFilter == EAnimFilterType::Not_Start)
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
        if constexpr (TFilter == EAnimFilterType::End_With || TFilter == EAnimFilterType::Not_End)
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
        return true;
    }

    static bool SetVariableCurveHelper(UAnimSequence* Seq, const FString& CurveName, FFloatCurve& InFloatCurve);

    static UCurveVector* CreateCurveVectorAsset(const FString& PackagePath, const FString& CurveName);

    static bool GetBoneKeysByNameHelper(UAnimSequence* Seq, FString const& BoneName, TArray<FVector>& OutPosKey,
                                        TArray<FQuat>& OutRotKey, bool bConvertCS = false);

    static bool MarkFootstepsFor1PAnimation(UAnimSequence* Seq, FString const& KeyBone = "LeftHand",
                                            bool bDebug = false);

    static bool SaveBonesCurves(UAnimSequence* AnimSequence, FString const& BoneName, const FString& SavePath);

    static bool LoadAnimSequencesByReference(const TArray<FString>& AnimSequencePaths,
                                             TArray<UAnimSequence*>& OutSequences);
};
