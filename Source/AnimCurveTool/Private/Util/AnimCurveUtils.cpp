#include "AnimCurveUtils.h"

#include "AnimationBlueprintLibrary.h"
#include "AssetRegistryModule.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "PackageTools.h"
#include "Kismet2/BlueprintEditorUtils.h"


DEFINE_LOG_CATEGORY_STATIC(LogAnimCurveUtil, Log, All);


void FAnimCurveUtils::GetAnimAssets(FString const& BaseDir, TArray<UAnimSequence*>& OutArray)
{
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
        "AssetRegistry");
    TArray<FAssetData> AssetData;

    FARFilter Filter;
    FString PackagePath, FailReason;
    if (!FPackageName::TryConvertFilenameToLongPackageName(BaseDir, PackagePath, &FailReason))
    {
        UE_LOG(LogAnimCurveUtil, Log, TEXT("Fail to search AnimSequence Assets, BaseDir error: %s"), *FailReason);
        return;
    }

    Filter.PackagePaths.Add(*PackagePath);
    Filter.ClassNames.Add(UAnimSequence::StaticClass()->GetFName());
    Filter.bRecursivePaths = true;

    AssetRegistryModule.Get().GetAssets(Filter, AssetData);
    for (int i = 0; i < AssetData.Num(); i++)
    {
        OutArray.Add(Cast<UAnimSequence>(AssetData[i].GetAsset()));
    }
}

bool FAnimCurveUtils::SetVariableCurveHelper(
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


bool FAnimCurveUtils::SaveBonesCurves(UAnimSequence* AnimSequence, FString const& BoneName, const FString& SaveDir,
                                      uint32 SaveFlags /* = 0xff */)
{
    if (!SaveFlags) return true;

    FVectorCurve PosCurve, RotCurve;
    
    TArray<FVector> PosKeys;
    TArray<FQuat> RotKeys;
    if (!GetBoneKeysByNameHelper(AnimSequence, BoneName, PosKeys, RotKeys, true))
    {
        UE_LOG(LogAnimCurveUtil, Error, TEXT("[%s->%s]: Bone not Exists!"), *AnimSequence->GetName(), *BoneName);
        return false;
    }

    for (int i = 0; i < AnimSequence->GetRawNumberOfFrames(); ++i)
    {
        const auto Time = AnimSequence->GetTimeAtFrame(i);
        const auto Translation = PosKeys[i];
        const auto EulerAngle = RotKeys[i].Euler();

        PosCurve.FloatCurves[0].UpdateOrAddKey(Time, Translation.X);
        PosCurve.FloatCurves[1].UpdateOrAddKey(Time, Translation.Y);
        PosCurve.FloatCurves[2].UpdateOrAddKey(Time, Translation.Z);
        
        RotCurve.FloatCurves[0].UpdateOrAddKey(Time, EulerAngle.X, true);
        RotCurve.FloatCurves[1].UpdateOrAddKey(Time, EulerAngle.Y, true);
        RotCurve.FloatCurves[2].UpdateOrAddKey(Time, EulerAngle.Z, true);
    }
    // MarkFootstepsFor1PAnimation(AnimSequence);
    // Save Packages
    
    TArray<UPackage*> Packages;
    const auto AnimName = AnimSequence->GetName();
    const auto RawPackagePath = SaveDir / AnimName;
    // Initialize Bone Packages
    FString PackagePath, FailReason;
    if (!FPackageName::TryConvertFilenameToLongPackageName(RawPackagePath, PackagePath, &FailReason))
    {
        UE_LOG(LogAnimCurveUtil, Error, TEXT("[%s->%s]: Create packages error, %s."), *AnimName, *BoneName, *FailReason)
        return false;
    }
    const auto CurveNamePrefix = AnimName + "_" + BoneName;

    if(SaveFlags & 0xf0)
    {
        auto PosCurve_Asset = CreateCurveVectorAsset(PackagePath, CurveNamePrefix + "_Translation");
        if (SaveFlags & 0x80) PosCurve_Asset->FloatCurves[0] = PosCurve.FloatCurves[0];
        if (SaveFlags & 0x40) PosCurve_Asset->FloatCurves[1] = PosCurve.FloatCurves[1];
        if (SaveFlags & 0x20) PosCurve_Asset->FloatCurves[2] = PosCurve.FloatCurves[2];
        Packages.Add(PosCurve_Asset->GetOutermost());
    }

    if(SaveFlags & 0x0f)
    {
        auto RotCurve_Asset = CreateCurveVectorAsset(PackagePath, CurveNamePrefix + "_Rotation");
        if (SaveFlags & 0x08) RotCurve_Asset->FloatCurves[0] = RotCurve.FloatCurves[0];
        if (SaveFlags & 0x04) RotCurve_Asset->FloatCurves[1] = RotCurve.FloatCurves[1];
        if (SaveFlags & 0x02) RotCurve_Asset->FloatCurves[2] = RotCurve.FloatCurves[2];
        Packages.Add(RotCurve_Asset->GetOutermost());
    }
    
    return UEditorLoadingAndSavingUtils::SavePackages(Packages, true);
}


bool FAnimCurveUtils::LoadAnimSequencesByReference(const TArray<FString>& AnimSequencePaths,
                                                   TArray<UAnimSequence*>& OutSequences)
{
    for (auto& Path : AnimSequencePaths)
    {
        auto Seq = LoadObject<UAnimSequence>(nullptr, *Path);
        if (Seq)
        {
            OutSequences.AddUnique(Seq);
        }
        else
        {
            UE_LOG(LogAnimCurveUtil, Log, TEXT("Load Failed from %s: %s"), *Seq->GetName(), *Path);
        }
    }
    return true;
}

UCurveVector* FAnimCurveUtils::CreateCurveVectorAsset(const FString& PackagePath,
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

bool FAnimCurveUtils::GetBoneKeysByNameHelper(
    UAnimSequence* Seq, FString const& BoneName, TArray<FVector>& OutPosKey,
    TArray<FQuat>& OutRotKey, bool bConvertCS /* = false */)
{
    auto Skeleton = Seq->GetSkeleton();
    auto RefSkeleton = Skeleton->GetReferenceSkeleton();
    auto BoneIndex = RefSkeleton.FindRawBoneIndex(*BoneName);
    if (BoneIndex == INDEX_NONE)
    {
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
    }
    while (bConvertCS && BoneIndex);

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

    return true;
}

bool FAnimCurveUtils::MarkFootstepsFor1PAnimation(
    UAnimSequence* Seq, FString const& KeyBone /* = "LeftHand" */, bool bDebug /* = false */)
{
    FFloatCurve FootstepsCurve, PosXCurve, PosZCurve, RotYCurve;
    // Get KeyBone translation and rotation keys
    TArray<FVector> PosKeys;
    TArray<FQuat> RotKeys;
    if (!GetBoneKeysByNameHelper(Seq, KeyBone, PosKeys, RotKeys, true))
    {
        UE_LOG(LogAnimCurveUtil, Warning, TEXT("[%s] BoneName (%s) not exist, or Curves can not be extracted"),
               *Seq->GetName(), *KeyBone);
        return false;
    }
    // Total Nums of the Key
    // Strategy: Capture the local Z minimal point in LeftHand
    auto N = PosKeys.Num();
    auto M = RotKeys.Num();
    auto FrameCount = Seq->GetNumberOfFrames();

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

    if (bDebug)
    {
        RotAvg /= Seq->GetNumberOfFrames();
        PosAvg /= Seq->GetNumberOfFrames();
        UE_LOG(LogAnimCurveUtil, Log, TEXT("key counts Pos: %d, Rot: %d"), N, M);
        UE_LOG(LogAnimCurveUtil, Log, TEXT("Position Average: %f, Rotation Average: %f"),
               PosAvg.X, RotAvg.Y);
    }

    FVector PrevPos, NextPos, CurrPos;
    FVector PrevRot, NextRot, CurrRot;

    TArray<FFootstepMarker> FootstepMarkers;
    TArray<int32> MinimalKeys;
    TArray<int32> Permutations;

    // Ignore end frame, cause frame_start == frame_end
    PosCache.Pop();
    RotCache.Pop();
    float Eps = 1e-6;
    N = PosCache.Num(), M = RotCache.Num();
    for (int i = 0; i < N; ++i)
    {
        PrevPos = PosCache[(i - 1 + N) % N];
        CurrPos = PosCache[i];
        NextPos = PosCache[(i + 1) % N];

        if (CurrPos.Z + Eps <= PrevPos.Z && CurrPos.Z + Eps <= NextPos.Z)
        {
            Permutations.Add(FootstepMarkers.Num());
            MinimalKeys.Add(i);
            FootstepMarkers.Push(FFootstepMarker{CurrPos.Z, (PrevPos.X > CurrPos.X) * 2 - 1, i});
        }
    }

    Permutations.Sort([&](int i, int j)
    {
        return FootstepMarkers[i].Value < FootstepMarkers[j].Value;
    });

    if (FootstepMarkers.Num() == 0)
    {
        UE_LOG(LogAnimCurveUtil, Warning, TEXT("[%s->%s] no footstep detected, skip."), *Seq->GetName(), *KeyBone);
        return false;
    }

    if (FootstepMarkers.Num() & 1)
    {
        UE_LOG(LogAnimCurveUtil, Warning, TEXT("[%s->%s] Odd footsteps detected, remove highest."), *Seq->GetName(),
               *KeyBone);
        FootstepMarkers.RemoveAt(Permutations.Last());
    }

    // Normal cases;
    // auto I = Permutations[0], J = Permutations[1];
    auto CurrStep = -1; // LocalMinims[I].Orientation
    for (auto& Footstep : FootstepMarkers)
    {
        FootstepsCurve.UpdateOrAddKey(CurrStep, Seq->GetTimeAtFrame(Footstep.Frame));
        FootstepsCurve.UpdateOrAddKey(-CurrStep, Seq->GetTimeAtFrame(Footstep.Frame) + 0.001);
        CurrStep = -CurrStep;
    }

    // FootstepsCurve.UpdateOrAddKey(-LocalMinims[J].Orientation, Seq->GetTimeAtFrame(MinimalKeys[J]) - 0.001);
    // FootstepsCurve.UpdateOrAddKey(LocalMinims[J].Orientation, Seq->GetTimeAtFrame(MinimalKeys[J]));
    //
    // if (LocalMinims[I].Orientation == LocalMinims[J].Orientation)
    // {
    // 	UE_LOG(LogAnimCurveUtil, Warning, TEXT("[%s->%s] Footstep Marks on the same side, please check it manually."), *Seq->GetName(), *KeyBone);
    // 	return false;
    // }

    SetVariableCurveHelper(Seq, "Footsteps_Curve", FootstepsCurve);
    Seq->MarkPackageDirty();
    if (bDebug)
    {
        SetVariableCurveHelper(Seq, FString::Printf(TEXT("%s_PosX_Curve"), *KeyBone), PosXCurve);
        SetVariableCurveHelper(Seq, FString::Printf(TEXT("%s_PosZ_Curve"), *KeyBone), PosZCurve);
        SetVariableCurveHelper(Seq, FString::Printf(TEXT("%s_RotY_Curve"), *KeyBone), RotYCurve);
    }

    if (FootstepMarkers.Num() > 2)
    {
        /* Check Potential Loops
        bool bLoop = true;
        for (int i = 0; i < LocalMinims.Num(); ++i)
        {
            auto I = Permutations[i], J = Permutations[(i + 2) % LocalMinims.Num()];
            if (abs(LocalMinims[I].Value - LocalMinims[J].Value) > Eps)
            {
                bLoop = false;
                break;
            }
        }
        if (bLoop)
        {
            UE_LOG(LogAnimCurveUtil, Warning, TEXT("[%s] Footstep Marks may contain potential loops."), *Seq->GetName());
            return false;
        }
        else
        {
            UE_LOG(LogAnimCurveUtil, Warning, TEXT("[%s] mark footstep could fail when target Curves.Pos.Z (of %s) contains more than 2 local minimas"), *Seq->GetName(), *KeyBone);
            return false;
        }
        */
        UE_LOG(LogAnimCurveUtil, Warning,
               TEXT("[%s->%s] mark footstep could fail when target Curves.Pos.Z contains more than 2 local minimas"),
               *Seq->GetName(), *KeyBone);
        return false;
    }
    else if (FootstepMarkers.Num() == 1)
    {
        UE_LOG(LogAnimCurveUtil, Warning, TEXT("[%s->%s] has only one footstep mark, please check it manually."),
               *Seq->GetName(), *KeyBone);
        return false;
    }
    return true;
}
