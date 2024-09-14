﻿#include "Script/Library/FIVSNode_Sequence.h"

#include "Editor/FIVSEdNodeViewer.h"
#include "Kernel/FIVSRuntimeContext.h"

void FFIVSNodeStatement_Sequence::PreExecPin(FFIVSRuntimeContext& Context, FGuid ExecPin) const {
	FFIVSNodeStatement::PreExecPin(Context, ExecPin);
}

void FFIVSNodeStatement_Sequence::ExecPin(FFIVSRuntimeContext& Context, FGuid ExecPin) const {
	Context.Push_ExecPin(ExecOut);
}

UFIVSNode_Sequence::UFIVSNode_Sequence() {
	DisplayName = FText::FromString(TEXT("Sequence"));
	ExecIn = CreateDefaultPin(FIVS_PIN_EXEC_INPUT, TEXT("Exec"), FText::FromString(TEXT("Exec")));
	ExecOut.Add(CreateDefaultPin(FIVS_PIN_EXEC_OUTPUT, TEXT("Then1"), FText::FromString(TEXT("Then 1"))));
	ExecOut.Add(CreateDefaultPin(FIVS_PIN_EXEC_OUTPUT, TEXT("Then2"), FText::FromString(TEXT("Then 2"))));
}

void UFIVSNode_Sequence::GetNodeActions(TArray<FFIVSNodeAction>& Actions) const {
	FFIVSNodeAction action = FFIVSNodeAction{
		UFIVSNode_Sequence::StaticClass(),
		FText::FromString(TEXT("Sequence")),
		FText::FromString(TEXT("General")),
		FText::FromString(TEXT("Sequence")),
		{
			FIVS_PIN_EXEC_INPUT,
			FIVS_PIN_EXEC_OUTPUT,
		},
	};
	Actions.Add(action);
}

void UFIVSNode_Sequence::ExtendPinContextMenu(UFIVSPin* InPin, FMenuBuilder& MenuBuilder) {
	if (!ExecOut.Contains(InPin)) return;

	MenuBuilder.AddSeparator();
	MenuBuilder.AddMenuEntry(
			FText::FromString("Remove Pin"),
			FText(),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this, InPin]() {
				ExecOut.Remove(InPin);
				DeletePin(InPin);
				SetOutputNum(ExecOut.Num());
			})));
}

TSharedRef<SFIVSEdNodeViewer> UFIVSNode_Sequence::CreateNodeViewer(const TSharedRef<SFIVSEdGraphViewer>& GraphViewer, const FFIVSEdNodeStyle* Style) {
	return SNew(SFIVSEdFunctionNodeViewer, GraphViewer, this)
		.Style(Style)
		.Footer()[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot().HAlign(HAlign_Right).Padding(10)[
				SNew(SButton)
				.Text(FText::FromString(TEXT("Add Pin")))
				.OnClicked_Lambda([this]() {
					SetOutputNum(ExecOut.Num() + 1);

					return FReply::Handled();
				})
			]
		];
}

void UFIVSNode_Sequence::SerializeNodeProperties(const TSharedRef<FJsonObject>& Properties) const {
	Properties->SetNumberField(TEXT("PinCount"), ExecOut.Num());
}

void UFIVSNode_Sequence::DeserializeNodeProperties(const TSharedPtr<FJsonObject>& Properties) {
	SetOutputNum(Properties->GetNumberField(TEXT("PinCount")));
}

void UFIVSNode_Sequence::SetOutputNum(int32 OutputNum) {
	while (ExecOut.Num() > OutputNum) {
		DeletePin(ExecOut.Pop());
	}
	while (ExecOut.Num() < OutputNum) {
		int32 i = ExecOut.Num() + 1;
		ExecOut.Add(CreatePin(FIVS_PIN_EXEC_OUTPUT, FString::Printf(TEXT("Then%i"), i), FText::FromString(FString::Printf(TEXT("Then %i"), i))));
	}
	for (int i = 0; i < OutputNum; ++i) {
		ExecOut[i]->DisplayName = FText::FromString(FString::Printf(TEXT("Then %i"), i+1));
	}
}
