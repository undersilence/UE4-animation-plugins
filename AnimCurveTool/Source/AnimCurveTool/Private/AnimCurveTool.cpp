// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "AnimCurveTool.h"
#include "AnimCurveToolStyle.h"
#include "AnimCurveToolCommands.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "UI/AnimCurveToolWidget.h"
#include "UI/AnimRuleCustomization.h"

static const FName AnimCurveToolTabName("AnimCurveTool");

#define LOCTEXT_NAMESPACE "FAnimCurveToolModule"

void FAnimCurveToolModule::StartupModule()
{
    // This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

    FAnimCurveToolStyle::Initialize();
    FAnimCurveToolStyle::ReloadTextures();

    FAnimCurveToolCommands::Register();

    PluginCommands = MakeShareable(new FUICommandList);

    PluginCommands->MapAction(
        FAnimCurveToolCommands::Get().OpenPluginWindow,
        FExecuteAction::CreateRaw(this, &FAnimCurveToolModule::PluginButtonClicked),
        FCanExecuteAction());

    FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

    {
        TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
        MenuExtender->AddMenuExtension("WindowLayout", EExtensionHook::After, PluginCommands,
                                       FMenuExtensionDelegate::CreateRaw(
                                           this, &FAnimCurveToolModule::AddMenuExtension));

        LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
    }

    // {
    // 	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
    // 	ToolbarExtender->AddToolBarExtension("Settings", EExtensionHook::After, PluginCommands, FToolBarExtensionDelegate::CreateRaw(this, &FAnimCurveToolModule::AddToolbarExtension));
    // 	
    // 	LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
    // }

    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(AnimCurveToolTabName,
                                                      FOnSpawnTab::CreateRaw(
                                                          this, &FAnimCurveToolModule::OnSpawnPluginTab))
                            .SetDisplayName(LOCTEXT("FAnimCurveToolTabTitle", "AnimCurveTool"))
                            .SetMenuType(ETabSpawnerMenuType::Hidden);

    // FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
    // PropertyEditorModule.RegisterCustomClassLayout(FName("FAnimRuleFilter"), FOnGetDetailCustomizationInstance::CreateStatic(&FAnimRuleDetailCustom::MakeInstance));
    FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
    PropertyModule.RegisterCustomPropertyTypeLayout(FAnimRuleFilter::StaticStruct()->GetFName(),
                                                    FOnGetPropertyTypeCustomizationInstance::CreateStatic(
                                                        &FAnimRuleCustomization::MakeInstance)
    );
}


void FAnimCurveToolModule::ShutdownModule()
{
    // This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
    // we call this function before unloading the module.
    FAnimCurveToolStyle::Shutdown();

    FAnimCurveToolCommands::Unregister();

    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(AnimCurveToolTabName);

    FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
    PropertyEditorModule.UnregisterCustomClassLayout(FAnimRuleFilter::StaticStruct()->GetFName());
}

TSharedRef<SDockTab> FAnimCurveToolModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
    FText WidgetText = FText::Format(
        LOCTEXT("WindowWidgetText", "Add code to {0} in {1} to override this window's contents"),
        FText::FromString(TEXT("FAnimCurveToolModule::OnSpawnPluginTab")),
        FText::FromString(TEXT("AnimCurveTool.cpp"))
    );

    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(SAnimCurveToolWidget)
        ];
}

void FAnimCurveToolModule::PluginButtonClicked()
{
    FGlobalTabmanager::Get()->InvokeTab(AnimCurveToolTabName);
}

void FAnimCurveToolModule::AddMenuExtension(FMenuBuilder& Builder)
{
    Builder.AddMenuEntry(FAnimCurveToolCommands::Get().OpenPluginWindow);
}

void FAnimCurveToolModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
    Builder.AddToolBarButton(FAnimCurveToolCommands::Get().OpenPluginWindow);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAnimCurveToolModule, AnimCurveTool)
