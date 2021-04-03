//
//  kern_alc.cpp
//  AppleALC
//
//  Copyright © 2016-2017 vit9696. All rights reserved.
//

#include <Headers/kern_api.hpp>
#include <Headers/kern_devinfo.hpp>
#include <Headers/plugin_start.hpp>
#include <IOKit/IOService.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <mach/vm_map.h>

#include "kern_alc.hpp"
#include "kern_resources.hpp"

// Only used in apple-driven callbacks
AlcEnabler* AlcEnabler::callbackAlc = nullptr;

static const char * kextPath0[] { "/System/Library/Extensions/AppleHDA.kext/Contents/MacOS/AppleHDA", };
const size_t KextIdAppleHDA = 0;
KernelPatcher::KextInfo ADDPR(kextList)[] {
	{ "com.apple.driver.AppleHDA", kextPath0, 1, {false, true}, {true}, KernelPatcher::KextInfo::Unloaded },
};

void AlcEnabler::init() {
	callbackAlc = this;

	lilu.onKextLoadForce(ADDPR(kextList), arrsize(ADDPR(kextList)),
	[](void *user, KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
		static_cast<AlcEnabler *>(user)->processKext(patcher, index, address, size);
	}, this);
}

void AlcEnabler::deinit() {

}

void AlcEnabler::layoutLoadCallback(uint32_t requestTag, kern_return_t result, const void *resourceData, uint32_t resourceDataLength, void *context) {
	DBGLOG("alc", "layoutLoadCallback %u %d %d %u %d", requestTag, result, resourceData != nullptr, resourceDataLength, context != nullptr);
	callbackAlc->updateResource(Resource::Layout, result, resourceData, resourceDataLength);
	DBGLOG("alc", "layoutLoadCallback done %u %d %d %u %d", requestTag, result, resourceData != nullptr, resourceDataLength, context != nullptr);
	FunctionCast(layoutLoadCallback, callbackAlc->orgLayoutLoadCallback)(requestTag, result, resourceData, resourceDataLength, context);
}

void AlcEnabler::platformLoadCallback(uint32_t requestTag, kern_return_t result, const void *resourceData, uint32_t resourceDataLength, void *context) {
	DBGLOG("alc", "platformLoadCallback %u %d %d %u %d", requestTag, result, resourceData != nullptr, resourceDataLength, context != nullptr);
	callbackAlc->updateResource(Resource::Platform, result, resourceData, resourceDataLength);
	DBGLOG("alc", "platformLoadCallback done %u %d %d %u %d", requestTag, result, resourceData != nullptr, resourceDataLength, context != nullptr);
	FunctionCast(platformLoadCallback, callbackAlc->orgPlatformLoadCallback)(requestTag, result, resourceData, resourceDataLength, context);
}

void AlcEnabler::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	size_t kextIndex = 0;

	while (kextIndex < arrsize(ADDPR(kextList))) {
		if (ADDPR(kextList)[kextIndex].loadIndex == index)
			break;
		kextIndex++;
	}

	if (kextIndex == arrsize(ADDPR(kextList)))
		return;

	if (kextIndex == KextIdAppleHDA) {
		KernelPatcher::RouteRequest requests[] {
			KernelPatcher::RouteRequest("__ZN14AppleHDADriver18layoutLoadCallbackEjiPKvjPv", layoutLoadCallback, orgLayoutLoadCallback),
			KernelPatcher::RouteRequest("__ZN14AppleHDADriver20platformLoadCallbackEjiPKvjPv", platformLoadCallback, orgPlatformLoadCallback),
		};

		patcher.routeMultiple(index, requests, address, size);
	}
	
	// Ignore all the errors for other processors
	patcher.clearError();
}

void AlcEnabler::updateResource(Resource type, kern_return_t &result, const void * &resourceData, uint32_t &resourceDataLength) {
	DBGLOG("alc", "resource-request arrived %s", type == Resource::Platform ? "platform" : "layout");

	if (type == Resource::Platform) {
		resourceData = platformData;
		resourceDataLength = platformDataSize;
	} else {
		resourceData = layoutData;
		resourceDataLength = layoutDataSize;
	}
	result = kOSReturnSuccess;
}
