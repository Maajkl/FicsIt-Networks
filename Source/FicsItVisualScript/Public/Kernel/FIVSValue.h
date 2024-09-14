﻿#pragma once

#include "Network/FINAnyNetworkValue.h"
#include "FIVSValue.generated.h"

USTRUCT()
struct FFIVSValue {
	GENERATED_BODY()
public:
	UPROPERTY(SaveGame)
	bool bIsLValue = false;

	UPROPERTY(SaveGame)
	FFINAnyNetworkValue RValue;

	UPROPERTY(SaveGame)
	FGuid LValuePin;

	FFIVSValue() = default;
	FFIVSValue(const FFIVSValue& other) : bIsLValue(other.bIsLValue), LValuePin(other.LValuePin) {
		if (!bIsLValue) {
			RValue = other.RValue;
		}
	}
	FFIVSValue(const FFIVSValue&& other) : bIsLValue(other.bIsLValue), LValuePin(other.LValuePin), RValue(other.RValue) {}
	FFIVSValue& operator=(const FFIVSValue& other) {
		bIsLValue = other.bIsLValue;
		LValuePin = other.LValuePin;
		if (!bIsLValue) {
			RValue = other.RValue;
		}

		return *this;
	}
	FFIVSValue& operator=(const FFIVSValue&& other) {
		bIsLValue = other.bIsLValue;
		LValuePin = other.LValuePin;
		RValue = other.RValue;
		return *this;
	}
	
	static FFIVSValue MakeRValue(FFINAnyNetworkValue InValue) {
		FFIVSValue value;
		value.bIsLValue = false;
		value.RValue = InValue;
		return value;
	}

	static FFIVSValue MakeLValue(FGuid Pin, FFINAnyNetworkValue InValue) {
		FFIVSValue value;
		value.bIsLValue = true;
		value.RValue = InValue;
		value.LValuePin = Pin;
		return value;
	}
};