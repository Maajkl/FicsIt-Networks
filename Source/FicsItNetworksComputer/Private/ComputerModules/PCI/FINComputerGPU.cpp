﻿#include "ComputerModules/PCI/FINComputerGPU.h"

#include "FGPlayerController.h"
#include "FINComputerRCO.h"
#include "FINComputerSubsystem.h"
#include "SlateApplication.h"
#include "Buildables/FGBuildableWidgetSign.h"
#include "Graphics/FINScreenInterface.h"
#include "Net/UnrealNetwork.h"

AFINComputerGPU::AFINComputerGPU() {
	SetActorTickEnabled(true);
	PrimaryActorTick.SetTickFunctionEnable(true);
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	NetDormancy = DORM_Awake;
}

void AFINComputerGPU::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AFINComputerGPU, Screen);
}

void AFINComputerGPU::BeginPlay() {
	Super::BeginPlay();

	UpdateScreen();
}
UE_DISABLE_OPTIMIZATION_SHIP
void AFINComputerGPU::TickActor(float DeltaTime, ELevelTick TickType, FActorTickFunction& ThisTickFunction) {
	Super::TickActor(DeltaTime, TickType, ThisTickFunction);

	if (!bScreenSizeUpdated && Screen) {
		if (auto widget = Cast<IFINScreenInterface>(Screen.GetUnderlyingPtr())->GetWidget()) {
			auto rco = GetWorld()->GetFirstPlayerController<AFGPlayerController>()->GetRemoteCallObjectOfClass<UFINComputerRCO>();

			TScriptInterface<IFINScreenInterface> screen = GetScreenInterface();
			if (screen && screen->GetWidget()) {
				FVector2D size = screen->GetWidget()->GetCachedGeometry().GetLocalSize();
				if (!size.IsNearlyZero()) {
					bScreenSizeUpdated = true;
					rco->GPUUpdateScreenSize(this, size);
				}
			}
		}
	}

	// TODO: Continuesly check for Trace Validity and udate widget accordingly
}
UE_ENABLE_OPTIMIZATION_SHIP

void AFINComputerGPU::EndPlay(const EEndPlayReason::Type endPlayReason) {
	Super::EndPlay(endPlayReason);
	if (endPlayReason == EEndPlayReason::Destroyed) BindScreen(FFIRTrace());
}

void AFINComputerGPU::BindScreen(const FFIRTrace& screen) {
	if (screen.IsValidPtr()) check(screen->GetClass()->ImplementsInterface(UFINScreenInterface::StaticClass()))
	if (Screen == screen) return;

	Screen = screen;
	UpdateScreen();
}

FFIRTrace AFINComputerGPU::GetScreen() const {
	return Screen;
}

void AFINComputerGPU::RequestNewWidget() {
	if (!GetScreenInterface()) return;
	GetScreenInterface()->SetWidget(CreateWidget());
}

TScriptInterface<IFINScreenInterface> AFINComputerGPU::GetScreenInterface() {
	return Screen.GetUnderlyingPtr();
}

TSharedPtr<SWidget> AFINComputerGPU::CreateWidget() {
	return nullptr;
}

int AFINComputerGPU::MouseToInt(const FPointerEvent& MouseEvent) {
	int mouseEvent = 0;
	if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))	mouseEvent |= 0b0000000001;
	if (MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton))	mouseEvent |= 0b0000000010;
	if (MouseEvent.IsControlDown())								mouseEvent |= 0b0000000100;
	if (MouseEvent.IsShiftDown())								mouseEvent |= 0b0000001000;
	if (MouseEvent.IsAltDown())									mouseEvent |= 0b0000010000;
	if (MouseEvent.IsCommandDown())								mouseEvent |= 0b0000100000;
	return mouseEvent;
}

int AFINComputerGPU::InputToInt(const FInputEvent& KeyEvent) {
	int mouseEvent = 0;
	if (KeyEvent.IsControlDown())								mouseEvent |= 0b0000000100;
	if (KeyEvent.IsShiftDown())									mouseEvent |= 0b0000001000;
	if (KeyEvent.IsAltDown())									mouseEvent |= 0b0000010000;
	if (KeyEvent.IsCommandDown())								mouseEvent |= 0b0000100000;
	return mouseEvent;
}

void AFINComputerGPU::OnRep_Screen() {
	bScreenSizeUpdated = false;
	UpdateScreen();
}

void AFINComputerGPU::UpdateScreen() {
	FFIRTrace newScreen = Screen;
	Screen = FFIRTrace();
	if (OldScreenCache.IsValidPtr()) Cast<IFINScreenInterface>(OldScreenCache.Get())->BindGPU(FFIRTrace());
	FFIRTrace oldScreen = OldScreenCache;
	OldScreenCache = Screen = newScreen;
	
	if (Screen.IsValidPtr()) Cast<IFINScreenInterface>(Screen.Get())->BindGPU(Screen / this);

	if (HasAuthority()) netSig_ScreenBound(oldScreen);
}

void AFINComputerGPU::netFunc_bindScreen(FFIRTrace NewScreen) {
	if (IsValid(NewScreen.GetUnderlyingPtr())) {
		bScreenSizeUpdated = false;
	}
	if (AFGBuildableWidgetSign* BuildableWidget = Cast<AFGBuildableWidgetSign>(*NewScreen)) {
		UFINGPUWidgetSign* WidgetSign = AFINComputerSubsystem::GetComputerSubsystem(this)->AddGPUWidgetSign(this, BuildableWidget);
		BindScreen(NewScreen / WidgetSign);
		return;
	}
	if (Cast<IFINScreenInterface>(NewScreen.GetUnderlyingPtr())) {
		BindScreen(NewScreen);
		return;
	}
	BindScreen(FFIRTrace());
	AFINComputerSubsystem::GetComputerSubsystem(this)->DeleteGPUWidgetSign(this);
}

FVector2D AFINComputerGPU::netFunc_getScreenSize() {
	TScriptInterface<IFINScreenInterface> screen = GetScreenInterface();
	if (screen && screen->GetWidget()) {
		return screen->GetWidget()->GetCachedGeometry().GetLocalSize();
	} else {
		FScopeLock Lock(&MutexScreenSize);
		return LastScreenSize;
	}
}

void AFINComputerGPU::netSig_ScreenBound_Implementation(const FFIRTrace& oldScreen) {}

void UFINScreenWidget::OnNewWidget() {
	if (Container.IsValid()) {
		if (Screen && Cast<IFINScreenInterface>(Screen)->GetWidget().IsValid()) {
			Container->SetContent(Cast<IFINScreenInterface>(Screen)->GetWidget().ToSharedRef());
		} else {
			Container->SetContent(SNew(SBox));
		}
	}
}

void UFINScreenWidget::OnNewGPU() {
	if (Screen) {
		TScriptInterface<IFINGPUInterface> GPU = GetScreen()->GetGPU().GetUnderlyingPtr();
		if (GPU) {
			GPU->RequestNewWidget();
			return;
		}
	}
	if (Container.IsValid()) Container->SetContent(SNew(SBox));
}

void UFINScreenWidget::SetScreen(UObject* NewScreen) {
	Screen = NewScreen;
	OnNewGPU();
}

TScriptInterface<IFINScreenInterface> UFINScreenWidget::GetScreen() {
	return Screen;
}

void UFINScreenWidget::Focus() {
	if (this->Screen) {
		TSharedPtr<SWidget> widget = Cast<IFINScreenInterface>(this->Screen)->GetWidget();
		if (widget.IsValid()) {
			FSlateApplication::Get().SetKeyboardFocus(widget);
		}
	}
}

void UFINScreenWidget::ReleaseSlateResources(bool bReleaseChildren) {
	Super::ReleaseSlateResources(bReleaseChildren);

	Container.Reset();

	/*if (Screen) {
		IFINGPUInterface* GPU = Cast<IFINGPUInterface>(Cast<IFINScreenInterface>(Screen)->GetGPU());
		if (GPU) GPU->DropWidget();
		OnNewWidget();
	}
	Container.Reset();*/
}

TSharedRef<SWidget> UFINScreenWidget::RebuildWidget() {
	Container = SNew(SBox).HAlign(HAlign_Fill).VAlign(VAlign_Fill);
	OnNewWidget();
	return Container.ToSharedRef();
}

void UFINGPUWidgetSign::BindGPU(const FFIRTrace& gpu) {
	if (gpu.IsValidPtr()) check(gpu->GetClass()->ImplementsInterface(UFINGPUInterface::StaticClass()))
	if (GPU == gpu) return;
	
	FFIRTrace oldGPU = GPU;
	GPU = FFIRTrace();
	if (oldGPU.IsValidPtr()) Cast<IFINGPUInterface>(oldGPU.GetUnderlyingPtr())->BindScreen(FFIRTrace());

	GPU = gpu;
	if (gpu.IsValidPtr()) Cast<IFINGPUInterface>(gpu.GetUnderlyingPtr())->BindScreen(gpu / this);

	OnGPUUpdate.Broadcast();
}

FFIRTrace UFINGPUWidgetSign::GetGPU() const {
	return GPU;
}

void UFINGPUWidgetSign::SetWidget(TSharedPtr<SWidget> widget) {
	if (Widget == widget) return;

	Widget = widget;
	
	OnWidgetUpdate.Broadcast();
}

TSharedPtr<SWidget> UFINGPUWidgetSign::GetWidget() const {
	return Widget;
}
