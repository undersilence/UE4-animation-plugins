#include "AnimRuleCustomization.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"
#include "Widgets/Text/STextBlock.h"

void FAnimRuleCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle,
                                                   FDetailWidgetRow& HeaderRow,
                                                   IPropertyTypeCustomizationUtils& CustomizationUtils)
{
    FilterTypeHandle = PropertyHandle->GetChildHandle(TEXT("FilterType"));
    KeywordsHandle = PropertyHandle->GetChildHandle(TEXT("Keywords"));
    check(FilterTypeHandle.IsValid());
    check(KeywordsHandle.IsValid());

    HeaderRow
        .NameContent()
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            [
                FilterTypeHandle->CreatePropertyNameWidget()
            ]
        ]
        .ValueContent()
	    .MinDesiredWidth(100.0f * 2.0f)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                FilterTypeHandle->CreatePropertyValueWidget()
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                KeywordsHandle->CreatePropertyValueWidget()
            ]
        ];
    // PropertyHandle->MarkResetToDefaultCustomized();
}


void FAnimRuleCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle,
                                               IDetailChildrenBuilder& ChildBuilder,
                                               IPropertyTypeCustomizationUtils& CustomizationUtils)
{

    auto& Builder = ChildBuilder.GetParentCategory().GetParentLayout();
    KeywordsHandle =  PropertyHandle->GetChildHandle(TEXT("Keywords"));
    check(KeywordsHandle.IsValid());
    
    auto KeywordArrayHandle = KeywordsHandle->AsArray();
    check(KeywordArrayHandle.IsValid());
    
    uint32 NbrOfKeywords = 0;
    KeywordArrayHandle->GetNumElements(NbrOfKeywords);
    for(uint32 i = 0; i < NbrOfKeywords; ++i)
    {
        ChildBuilder.AddProperty(KeywordArrayHandle->GetElement(i));
    }

    auto OnKeywordsChanged = FSimpleDelegate::CreateLambda([&Builder]
    {
        Builder.ForceRefreshDetails();
        // UE_LOG(LogTemp, Log, TEXT("OnKeywordsChanged!"));
    });
    KeywordArrayHandle->SetOnNumElementsChanged(OnKeywordsChanged);
    KeywordsHandle->NotifyPostChange();
}
