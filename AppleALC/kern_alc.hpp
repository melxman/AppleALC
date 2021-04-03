//
//  kern_alc.hpp
//  AppleALC
//
//  Copyright © 2016-2017 vit9696. All rights reserved.
//

#ifndef kern_alc_hpp
#define kern_alc_hpp

#include <Headers/kern_patcher.hpp>
#include <Headers/kern_devinfo.hpp>

class AlcEnabler {
public:
	void init();
	void deinit();

private:
	/**
	 *	The only allowed instance of this class
	 */
	static AlcEnabler* callbackAlc;
	
	/**
	 *  Patch AppleHDA or another kext if needed and prepare other patches
	 *
	 *  @param patcher KernelPatcher instance
	 *  @param index   kinfo handle
	 *  @param address kinfo load address
	 *  @param size    kinfo memory size
	 */
	void processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);

	/**
	 *  Hooked ResourceLoad callbacks returning correct layout/platform
	 */
	static void layoutLoadCallback(uint32_t requestTag, kern_return_t result, const void *resourceData, uint32_t resourceDataLength, void *context);
	static void platformLoadCallback(uint32_t requestTag, kern_return_t result, const void *resourceData, uint32_t resourceDataLength, void *context);

	/**
	 *  Trampolines for original method invocations
	 */
	mach_vm_address_t orgLayoutLoadCallback {0};
	mach_vm_address_t orgPlatformLoadCallback {0};

	/**
	 *  Supported resource types
	 */
	enum class Resource {
		Layout,
		Platform
	};

	/**
	 *  Update resource request parameters with hooked data if necessary
	 *
	 *  @param type               resource type
	 *  @param result             kOSReturnSuccess on resource update
	 *  @param resourceData       resource data reference
	 *  @param resourceDataLength resource data length reference
	 */
	void updateResource(Resource type, kern_return_t &result, const void * &resourceData, uint32_t &resourceDataLength);
};

#endif /* kern_alc_hpp */
