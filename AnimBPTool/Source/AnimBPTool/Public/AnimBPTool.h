// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class IAnimationBlueprintEditor;
class FUICommandList;
class FExtender;
class FToolBarBuilder;
class FMenuBuilder;

class FAnimBPToolModule : public IModuleInterface
{
public:
    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    void AddAnimationBlueprintEditorToolbarExtender();
    void RemoveAnimationBlueprintEditorToolbarExtender() const;
    virtual void ShutdownModule() override;

    /** This function will be bound to Command. */
    void HideUnconnectedPins() const;

private:
    void AddToolbarExtension(FToolBarBuilder& Builder);
    TSharedRef<FExtender> GetAnimationBlueprintEditorToolbarExtender(
        const TSharedRef<FUICommandList> CommandList, TSharedRef<IAnimationBlueprintEditor> InAnimationBlueprintEditor);

private:
    TSharedPtr<class FUICommandList> PluginCommands;
    FDelegateHandle AnimationBlueprintEditorExtenderHandle;
    TWeakPtr<IAnimationBlueprintEditor> AnimBPEditorPtr;
};
