// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "AnimCurveExportStyle.h"

class FAnimCurveExportCommands : public TCommands<FAnimCurveExportCommands>
{
public:

	FAnimCurveExportCommands()
		: TCommands<FAnimCurveExportCommands>(TEXT("AnimCurveExport"), NSLOCTEXT("Contexts", "AnimCurveExport", "AnimCurveExport Plugin"), NAME_None, FAnimCurveExportStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};