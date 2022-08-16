#pragma once

#include "CoreMinimal.h"
#include "AssetRegistryModule.h"
#include "Animation/AnimSequence.h"
#include "Curves/CurveVector.h"

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

    static bool SetVariableCurveHelper(UAnimSequence* Seq, const FString& CurveName, FFloatCurve& InFloatCurve);

    static UCurveVector* CreateCurveVectorAsset(const FString& PackagePath, const FString& CurveName);

    static bool GetBoneKeysByNameHelper(UAnimSequence* Seq, FString const& BoneName, TArray<FVector>& OutPosKey,
                                        TArray<FQuat>& OutRotKey, bool bConvertCS = false);

    static float CalcStdDevOfMarkers(TArray<FFootstepMarker> const & Array, int32 N);
    
    // Give optional bone names for capture, return best matches.
    static bool MarkFootstepsFor1PAnimation(UAnimSequence* Seq, TArray<FString> KeyBones = {"LeftHand", "RightHand"},
                                            bool bDebug = false);

    // Give bone name for capture, return possible marks.
    static void CaptureFootstepMarksByBoneName(UAnimSequence* Seq, FString const& BoneNames,
                                               TArray<FFootstepMarker>& Markers, bool bDebug = false);
 
    // SaveFlags 0b_____________0000__________0000
    //              translation.xyzw rotation.xyzw
    static bool SaveBonesCurves(UAnimSequence* AnimSequence, FString const& BoneName, const FString& SavePath, uint32 SaveFlags = 0xff);

    static bool LoadAnimSequencesByReference(const TArray<FString>& AnimSequencePaths,
                                             TArray<UAnimSequence*>& OutSequences);
};
