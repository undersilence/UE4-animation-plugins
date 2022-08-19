// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "AnimBPTool.h"
#include "AnimBPToolStyle.h"
#include "AnimBPToolCommands.h"
#include "BlueprintEditorModule.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

#include "IAnimationBlueprintEditorModule.h"
#include "GraphEditorActions.h"
#include "K2Node_BreakStruct.h"
#include "K2Node_GetClassDefaults.h"
#include "K2Node_SetFieldsInStruct.h"
#include "EdGraph/EdGraph.h"

static const FName AnimBPToolTabName("AnimBPTool");

#define LOCTEXT_NAMESPACE "FAnimBPToolModule"

void FAnimBPToolModule::StartupModule()
{
    // This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

    FAnimBPToolStyle::Initialize();
    FAnimBPToolStyle::ReloadTextures();

    FAnimBPToolCommands::Register();

    PluginCommands = MakeShareable(new FUICommandList);

    PluginCommands->MapAction(
       FAnimBPToolCommands::Get().HidePins,
       FExecuteAction::CreateRaw(this, &FAnimBPToolModule::HideUnconnectedPins),
       FCanExecuteAction());

    AddAnimationBlueprintEditorToolbarExtender();
}

void FAnimBPToolModule::AddAnimationBlueprintEditorToolbarExtender()
{
    IAnimationBlueprintEditorModule& AnimationBlueprintEditorModule = FModuleManager::Get().LoadModuleChecked<
        IAnimationBlueprintEditorModule>("AnimationBlueprintEditor");
    auto& ToolbarExtenders = AnimationBlueprintEditorModule.GetAllAnimationBlueprintEditorToolbarExtenders();

    ToolbarExtenders.Add(
        IAnimationBlueprintEditorModule::FAnimationBlueprintEditorToolbarExtender::CreateRaw(
            this, &FAnimBPToolModule::GetAnimationBlueprintEditorToolbarExtender));
    AnimationBlueprintEditorExtenderHandle = ToolbarExtenders.Last().GetHandle();
}

void FAnimBPToolModule::RemoveAnimationBlueprintEditorToolbarExtender() const
{
    IAnimationBlueprintEditorModule* AnimationBlueprintEditorModule = FModuleManager::Get().GetModulePtr<
        IAnimationBlueprintEditorModule>("AnimationBlueprintEditor");
    if (AnimationBlueprintEditorModule)
    {
        typedef IAnimationBlueprintEditorModule::FAnimationBlueprintEditorToolbarExtender DelegateType;
        AnimationBlueprintEditorModule->GetAllAnimationBlueprintEditorToolbarExtenders().RemoveAll(
            [=](const DelegateType& In) { return In.GetHandle() == AnimationBlueprintEditorExtenderHandle; });
    }
}

TSharedRef<FExtender> FAnimBPToolModule::GetAnimationBlueprintEditorToolbarExtender(
    const TSharedRef<FUICommandList> CommandList, TSharedRef<IAnimationBlueprintEditor> InAnimationBlueprintEditor)
{
    TSharedRef<FExtender> Extender = MakeShareable(new FExtender);
    CommandList->Append(PluginCommands.ToSharedRef());
    if (InAnimationBlueprintEditor->GetBlueprintObj() && InAnimationBlueprintEditor->GetBlueprintObj()->BlueprintType !=
        BPTYPE_Interface)
    {
        AnimBPEditorPtr = InAnimationBlueprintEditor;
        Extender->AddToolBarExtension(
            "Asset",
            EExtensionHook::After,
            CommandList,
            FToolBarExtensionDelegate::CreateRaw(this, &FAnimBPToolModule::AddToolbarExtension));
    }

    return Extender;
}

void FAnimBPToolModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
    Builder.BeginSection("Graph");
    {
        Builder.AddToolBarButton(
            FAnimBPToolCommands::Get().HidePins,
            NAME_None,
            LOCTEXT("HideUnconnectedPins", "Hide Unconnected Pins"),
            LOCTEXT("HideUnconnectedPinsTooltip",
                    "Hide unconnected pins for K2Node_BreakStruct in blueprint"),
            FSlateIcon("AnimBPToolStyle", "AnimBPTool.HidePins"));
        
    }
    Builder.EndSection();
}

void FAnimBPToolModule::HideUnconnectedPins() const
{
    if(!AnimBPEditorPtr.IsValid()) return;
    const auto GraphPtr = AnimBPEditorPtr.Pin()->GetFocusedGraph();
    GEditor->BeginTransaction(LOCTEXT("HideAllUnconnectedPins", "Hide All Unconnected Pins"));
    GraphPtr->Modify();
    for (const auto Node : GraphPtr->Nodes)
    {
        if (Node && Node->IsA<UK2Node_BreakStruct>())
        {
            const auto Node_BreakStruct = Cast<UK2Node_BreakStruct>(Node);
            bool bWasChanged = false;
            for (auto& PropertyPin : Node_BreakStruct->ShowPinForProperties)
            {
                const auto NodePin = Node_BreakStruct->FindPin(
                    PropertyPin.PropertyName.ToString(), EGPD_Output);
                if (NodePin && NodePin->LinkedTo.Num() <= 0)
                {
                    bWasChanged = true;
                    PropertyPin.bShowPin = false;
                    NodePin->SetSavePinIfOrphaned(false);
                }
            }
            // Node_BreakStruct->Modify();
            if(bWasChanged) 
                Node_BreakStruct->ReconstructNode();
            // Node_BreakStruct->Modify();
        }
    }
    GEditor->EndTransaction();
    // auto SelectedNodes = AnimationBlueprintEditor->GetSelectedNodes();
    // for (auto Node : SelectedNodes)
    // {
    //     UE_LOG(LogTemp, Log, TEXT("Select Node %s.%s"), *Node->GetName(), *Node->GetDesc());
    // }
}


void FAnimBPToolModule::ShutdownModule()
{
    // This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
    // we call this function before unloading the module.
    RemoveAnimationBlueprintEditorToolbarExtender();
    FAnimBPToolStyle::Shutdown();
    FAnimBPToolCommands::Unregister();
}


#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAnimBPToolModule, AnimBPTool)
