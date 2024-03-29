﻿#include "AnimCurveUtils.h"

#include "AnimationBlueprintLibrary.h"
#include "AssetRegistryModule.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "PackageTools.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/AnimNotifies/AnimNotify_PlaySound.h"
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

    if (SaveFlags & 0xf0)
    {
        auto PosCurve_Asset = CreateCurveVectorAsset(PackagePath, CurveNamePrefix + "_Translation");
        if (SaveFlags & 0x80) PosCurve_Asset->FloatCurves[0] = PosCurve.FloatCurves[0];
        if (SaveFlags & 0x40) PosCurve_Asset->FloatCurves[1] = PosCurve.FloatCurves[1];
        if (SaveFlags & 0x20) PosCurve_Asset->FloatCurves[2] = PosCurve.FloatCurves[2];
        Packages.Add(PosCurve_Asset->GetOutermost());
    }

    if (SaveFlags & 0x0f)
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

float FAnimCurveUtils::CalcStdDevOfMarkers(TArray<FFootstepMarker> const& Markers, int32 TotalFrames)
{
    const float AverageStride = static_cast<float>(TotalFrames - 1) / Markers.Num();
    float StdDev = 0.0f;
    for (int i = 0; i < Markers.Num(); i++)
    {
        const int CurrPt = Markers[i].Frame;
        const int NextPt = Markers[(i + 1) % Markers.Num()].Frame;
        const int Stride = (NextPt + TotalFrames - 1 - CurrPt) % (TotalFrames - 1);
        StdDev += (Stride - AverageStride) * (Stride - AverageStride);
    }
    StdDev /= Markers.Num();
    return StdDev /= Markers.Num();
}

bool FAnimCurveUtils::MarkFootstepsFor1PAnimation(
    UAnimSequence* Seq, TArray<FString> KeyBones, bool bUseCurve /* = true */, bool bDebug /* = false */)
{
    // TArray<TArray<FFootstepMarker>> MarkersBuffer;
    float MinPenalty = 1e9;
    TArray<FFootstepMarker> BestMarkers;
    const auto TotalFrames = Seq->GetNumberOfFrames();
    FString BestKeyBone;

    for (auto const& KeyBone : KeyBones)
    {
        float Penalty = 0;
        TArray<FFootstepMarker> Markers;
        CaptureLocalMinimaMarksByBoneName(Seq, KeyBone, Markers, bDebug);
        if (Markers.Num() == 0)
        {
            // no markers
            continue;
        }

        // Rule#0, Even numbers is best
        Penalty += (Markers.Num() % 2) * 10000.0f;

        // Rule#1, stride uniformly
        Penalty += CalcStdDevOfMarkers(Markers, TotalFrames);
        UE_LOG(LogAnimCurveUtil, Log, TEXT("[%s->%s] Penalty: %f"), *Seq->GetName(), *KeyBone, Penalty);

        // Choose Best markers
        if (Penalty < MinPenalty)
        {
            MinPenalty = Penalty;
            BestKeyBone = KeyBone;
            Swap(BestMarkers, Markers);
        }
    }

    if (BestMarkers.Num() == 0)
    {
        // No Markers 
        return false;
    }

    if (BestMarkers.Num() > 1 && BestMarkers.Num() % 2 == 1)
    {
        // if still the number of steps is odd, remove one
        TArray<FFootstepMarker> ModifiedMarkers;
        MinPenalty = 1e9;
        for (int i = 0; i < BestMarkers.Num(); ++i)
        {
            TArray<FFootstepMarker> TempMarkers = BestMarkers;
            TempMarkers.RemoveAt(i);
            float Penalty = CalcStdDevOfMarkers(TempMarkers, Seq->GetNumberOfFrames());
            if (Penalty < MinPenalty)
            {
                MinPenalty = Penalty;
                Swap(ModifiedMarkers, TempMarkers);
            }
        }
        BestMarkers = ModifiedMarkers;
    }

    // Process True Footsteps
    auto Frame0 = BestMarkers[0].Frame;
    auto Frame1 = BestMarkers[1 % BestMarkers.Num()].Frame;
    if (Frame1 <= Frame0) Frame1 += (TotalFrames - 1);
    auto FrameMid = (Frame0 + (Frame1 - Frame0) / 2);
    auto Offset = (FrameMid - Frame1) / 2;
    for (auto& Marker : BestMarkers)
    {
        Marker.Frame += Offset;
        if(Marker.Frame < 0) Marker.Frame += TotalFrames - 1;
    }

    // Seq->Modify();
    // Normal cases;
    if(bUseCurve)
    {
        FFloatCurve FootstepsCurve;
        auto CurrStep = -1; // LocalMinims[I].Orientation
        for (auto const& Footstep : BestMarkers)
        {
            FootstepsCurve.UpdateOrAddKey(CurrStep, Seq->GetTimeAtFrame(Footstep.Frame));
            FootstepsCurve.UpdateOrAddKey(-CurrStep, Seq->GetTimeAtFrame(Footstep.Frame) + 0.001);
            CurrStep = -CurrStep;
        }
        SetVariableCurveHelper(Seq, "Footsteps_Curve", FootstepsCurve);
    } else
    {
        FName FootstepTrackName(TEXT("Footstep_Track"));
        FName FootstepNotifyName(TEXT("Footstep_Event"));
        UAnimationBlueprintLibrary::RemoveAnimationNotifyTrack(Seq, FootstepTrackName);
        UAnimationBlueprintLibrary::AddAnimationNotifyTrack(Seq, FootstepTrackName);
        // UAnimNotify* Notify = DuplicateObject(GetDefault<UAnimNotify>(), Seq, TEXT("Footstep_Event"));
        // FAnimNotifyEvent NotifyEvent;

        // Create Footstep track
        auto Skeleton = Seq->GetSkeleton();
        Skeleton->AddNewAnimationNotify(FootstepNotifyName);
        for(auto const& Footstep : BestMarkers)
        {
            CreateNewNotify(Seq, FootstepTrackName, FootstepNotifyName, Seq->GetTimeAtFrame(Footstep.Frame));
        }
    }
    Seq->PostEditChange();
    Seq->MarkPackageDirty();
    return true;
}

void FAnimCurveUtils::CreateNewNotify(UAnimSequence* Seq, FName TrackName, FName NotifyName, float StartTime)
{
    // Insert a new notify record and spawn the new notify object
    int32 NewNotifyIndex = Seq->Notifies.Add(FAnimNotifyEvent());
    FAnimNotifyEvent& NewEvent = Seq->Notifies[NewNotifyIndex];
    NewEvent.NotifyName = NotifyName;
    
    NewEvent.Link(Seq, StartTime);
    NewEvent.TriggerTimeOffset = GetTriggerTimeOffsetForType(Seq->CalculateOffsetForNotify(StartTime));
    NewEvent.TrackIndex = UAnimationBlueprintLibrary::GetTrackIndexForAnimationNotifyTrackName(Seq, TrackName);
    NewEvent.Notify = nullptr;
    NewEvent.NotifyStateClass = nullptr;

    Seq->PostEditChange();
    Seq->MarkPackageDirty();
}


void FAnimCurveUtils::CaptureLocalMinimaMarksByBoneName(UAnimSequence* Seq, FString const& BoneName,
                                                        TArray<FFootstepMarker>& FootstepMarkers, bool bDebug)
{
    FFloatCurve PosXCurve, PosZCurve, RotYCurve;
    // Get KeyBone translation and rotation keys
    TArray<FVector> PosKeys;
    TArray<FQuat> RotKeys;
    if (!GetBoneKeysByNameHelper(Seq, BoneName, PosKeys, RotKeys, true))
    {
        UE_LOG(LogAnimCurveUtil, Warning, TEXT("[%s] BoneName (%s) not exist, or Curves can not be extracted"),
               *Seq->GetName(), *BoneName);
        return;
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
    constexpr float Eps = 1e-6;
    // Ignore end frame, cause frame_start == frame_end
    PosCache.Pop(), RotCache.Pop();
    N = PosCache.Num(), M = RotCache.Num();

    for (int i = 0; i < N; ++i)
    {
        PrevPos = PosCache[(i - 1 + N) % N];
        CurrPos = PosCache[i];
        NextPos = PosCache[(i + 1) % N];

        if (CurrPos.Z + Eps <= PrevPos.Z && CurrPos.Z + Eps <= NextPos.Z)
        {
            FootstepMarkers.Push(FFootstepMarker{CurrPos.Z, (PrevPos.X > CurrPos.X) * 2 - 1, i});
        }
    }

    if (bDebug)
    {
        SetVariableCurveHelper(Seq, FString::Printf(TEXT("%s_PosX_Curve"), *BoneName), PosXCurve);
        SetVariableCurveHelper(Seq, FString::Printf(TEXT("%s_PosZ_Curve"), *BoneName), PosZCurve);
        SetVariableCurveHelper(Seq, FString::Printf(TEXT("%s_RotY_Curve"), *BoneName), RotYCurve);
    }

    /*if (FootstepMarkers.Num() > 2)
    {
        UE_LOG(LogAnimCurveUtil, Warning,
               TEXT("[%s->%s] mark footstep could fail when target Curves.Pos.Z contains more than 2 local minimas"),
               *Seq->GetName(), *BoneName);
    }
    else if (FootstepMarkers.Num() == 1)
    {
        UE_LOG(LogAnimCurveUtil, Warning, TEXT("[%s->%s] has only one footstep mark, please check it manually."),
               *Seq->GetName(), *BoneName);
    }*/
}
