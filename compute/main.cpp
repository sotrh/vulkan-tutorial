#include <vulkan/vulkan.h>

#include <cstdio>
#include <cstdlib>

#define BAIL_ON_BAD_RESULT(result) \
	if (VK_SUCCESS != (result)) {\
		fprintf(stderr, "Failure at %u %s\n", __LINE__, __FILE__);\
		exit(-1);\
	}

VkResult vkGetBestTransferQueueNPH(VkPhysicalDevice physicalDevice, uint32_t* queueFamilyIndex) {
	uint32_t queueFamilyPropertiesCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertiesCount, 0);
	VkQueueFamilyProperties* const queueFamilyProperties = (VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties) * queueFamilyPropertiesCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertiesCount, queueFamilyProperties);

	uint32_t i;
	VkQueueFlags maskedFlags;

	// try for a queue with just a transfer bit
	for (i = 0; i < queueFamilyPropertiesCount; i++) {
		maskedFlags = (~VK_QUEUE_SPARSE_BINDING_BIT & queueFamilyProperties[i].queueFlags);

		if (!((VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT) & maskedFlags) &&
			(VK_QUEUE_TRANSFER_BIT & maskedFlags)) {
			*queueFamilyIndex = i;
			return VK_SUCCESS;
		}
	}

	// try compute only
	for (i = 0; i < queueFamilyPropertiesCount; i++) {
		maskedFlags = (~VK_QUEUE_SPARSE_BINDING_BIT & queueFamilyProperties[i].queueFlags);

		if (!(VK_QUEUE_GRAPHICS_BIT & maskedFlags) && (VK_QUEUE_COMPUTE_BIT & maskedFlags)) {
			*queueFamilyIndex = i;
			return VK_SUCCESS;
		}
	}

	// try anything
	for (i = 0; i < queueFamilyPropertiesCount; i++) {
		maskedFlags = (~VK_QUEUE_SPARSE_BINDING_BIT & queueFamilyProperties[i].queueFlags);

		if ((VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT) & maskedFlags) {
			*queueFamilyIndex = i;
			return VK_SUCCESS;
		}
	}

	return VK_ERROR_INITIALIZATION_FAILED;
}

VkResult vkGetBestComputeQueueNPH(VkPhysicalDevice physicalDevice, uint32_t* queueFamilyIndex) {
	uint32_t queueFamilyPropertiesCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertiesCount, 0);
	VkQueueFamilyProperties* const queueFamilyProperties = (VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties) * queueFamilyPropertiesCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertiesCount, queueFamilyProperties);

	// compute only
	for (uint32_t i = 0; i < queueFamilyPropertiesCount; i++) {
		const VkQueueFlags maskedFlags = (~(VK_QUEUE_TRANSFER_BIT | VK_QUEUE_SPARSE_BINDING_BIT) &
			queueFamilyProperties[i].queueFlags);

		if (!(VK_QUEUE_GRAPHICS_BIT & maskedFlags) && (VK_QUEUE_COMPUTE_BIT & maskedFlags)) {
			*queueFamilyIndex = i;
			return VK_SUCCESS;
		}
	}

	// anything else
	for (uint32_t i = 0; i < queueFamilyPropertiesCount; i++) {
		const VkQueueFlags maskedFlags = (~(VK_QUEUE_TRANSFER_BIT | VK_QUEUE_SPARSE_BINDING_BIT) &
			queueFamilyProperties[i].queueFlags);

		if (VK_QUEUE_COMPUTE_BIT & maskedFlags) {
			*queueFamilyIndex = i;
			return VK_SUCCESS;
		}
	}

	return VK_ERROR_INITIALIZATION_FAILED;
}

int main() {
	const VkApplicationInfo applicationInfo = {
		VK_STRUCTURE_TYPE_APPLICATION_INFO,
		0,
		"VKComputeSample",
		0,
		"",
		0,
		VK_MAKE_VERSION(1, 0, 9)
	};

	const VkInstanceCreateInfo instanceCreateInfo = {
		VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		0,
		0,
		&applicationInfo,
		0,
		0,
		0,
		0,
	};

	VkInstance instance;
	BAIL_ON_BAD_RESULT(vkCreateInstance(&instanceCreateInfo, 0, &instance));

	uint32_t physicalDeviceCount = 0;
	BAIL_ON_BAD_RESULT(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, 0));
	VkPhysicalDevice* const physicalDevices = (VkPhysicalDevice*)malloc(
		sizeof(VkPhysicalDevice) * physicalDeviceCount);
	BAIL_ON_BAD_RESULT(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices));

	for (uint32_t i = 0; i < physicalDeviceCount; i++) {
		uint32_t queueFamilyIndex = 0;
		BAIL_ON_BAD_RESULT(vkGetBestComputeQueueNPH(physicalDevices[i], &queueFamilyIndex));

		const float queuePriority = 1.0f;
		const VkDeviceQueueCreateInfo deviceQueueCreateInfo = {
			VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			0,
			0,
			queueFamilyIndex,
			1,
			&queuePriority
		};

		const VkDeviceCreateInfo deviceCreateInfo = {
			VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			0,
			0,
			1,
			&deviceQueueCreateInfo,
			0,
			0,
			0,
			0,
			0
		};

		VkDevice device;
		BAIL_ON_BAD_RESULT(vkCreateDevice(physicalDevices[i], &deviceCreateInfo, 0, &device));

		VkPhysicalDeviceMemoryProperties properties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevices[i], &properties);

		const int32_t bufferLength = 16384;
		const int32_t bufferSize = sizeof(int32_t) * bufferLength;

		// two buffers from one chunk of memory
		const VkDeviceSize memorySize = bufferSize * 2;

		// set memoryTypeIndex to an invalid entry in the properties.memoryTypes array
		uint32_t memoryTypeIndex = VK_MAX_MEMORY_TYPES;

		// for (uint32_t k = 0; k < properties.memoryTypeCount; k++) {
		// 	if ((VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT & properties.memoryTypes[k].propertyFlags)
		// 		&& (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT & ))
		// }
	}

    return 0;
}