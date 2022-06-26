#ifdef _WIN32
#include <d3d11.h>
#include <d3d12.h>
#endif

#include <gl/GL.h>
#include <vulkan/vulkan.h>

#define XR_USE_GRAPHICS_API_VULKAN
#define XR_USE_GRAPHICS_API_OPENGL
#ifdef _WIN32
#define XR_USE_GRAPHICS_API_D3D11
#define XR_USE_GRAPHICS_API_D3D12
#define XR_USE_PLATFORM_WIN32
#endif

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <xrew.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <vector>
#include <iostream>
#include <cstring>
#include <fstream>
#include <sstream>

nlohmann::json document;

bool has_opengl = false;
bool has_d3d11 = false;
bool has_d3d12 = false;
bool has_vulkan = false;
bool has_vulkan_2 = false;

bool IsSupported(const std::vector<XrExtensionProperties>& extensionProperties, const char* extensionName)
{
	return std::find_if(extensionProperties.cbegin(), extensionProperties.cend(), [extensionName](const XrExtensionProperties& extensionProperties)
		{
			return 0 == strcmp(extensionProperties.extensionName, extensionName);
		}) != std::cend(extensionProperties);
}

void JsonAddVersionObject(nlohmann::json& json, XrVersion version, bool has_patch = false)
{
	json["major"] = XR_VERSION_MAJOR(version);
	json["minor"] = XR_VERSION_MINOR(version);
	if(has_patch)
		json["patch"] = XR_VERSION_PATCH(version);
}

std::string LuidToString(LUID _luid)
{
	union luid_array
	{
		uint8_t bytes[VK_LUID_SIZE];
		LUID luid;
	};

	luid_array luid;
	luid.luid = _luid;

	std::stringstream output;

	for(size_t i = 0;i < VK_LUID_SIZE; ++i)
	{
		output << std::hex << luid.bytes[i];
		if (i < VK_LUID_SIZE - 1)
			output << ":";
	}

	return output.str();
}

std::string FeatureLevelToString(D3D_FEATURE_LEVEL level)
{
	switch(level)
	{
	case D3D_FEATURE_LEVEL_1_0_CORE:
		return "D3D_FEATURE_LEVEL_1_0_CORE";
	case D3D_FEATURE_LEVEL_9_1:
		return "D3D_FEATURE_LEVEL_9_1";
	case D3D_FEATURE_LEVEL_9_2:
		return "D3D_FEATURE_LEVEL_9_2";
	case D3D_FEATURE_LEVEL_9_3:
		return "D3D_FEATURE_LEVEL_9_3";
	case D3D_FEATURE_LEVEL_10_0:
		return "D3D_FEATURE_LEVEL_10_0";
	case D3D_FEATURE_LEVEL_10_1:
		return "D3D_FEATURE_LEVEL_10_1";
	case D3D_FEATURE_LEVEL_11_0:
		return "D3D_FEATURE_LEVEL_11_0";
	case D3D_FEATURE_LEVEL_11_1:
		return "D3D_FEATURE_LEVEL_11_1";
	case D3D_FEATURE_LEVEL_12_0:
		return "D3D_FEATURE_LEVEL_12_0";
	case D3D_FEATURE_LEVEL_12_1:
		return "D3D_FEATURE_LEVEL_12_1";
	default:
		return "";
	}
}

void QuerySystemProperties(XrInstance instance, XrSystemId  systemId, nlohmann::json & system)
{
	if (systemId == 0) return;

	XrSystemProperties systemProperties{ XR_TYPE_SYSTEM_PROPERTIES, nullptr, systemId };
	if (XR_SUCCESS == xrGetSystemProperties(instance, systemId, &systemProperties))
	{
		system["systemName"] = systemProperties.systemName;
		system["vendorId"] = systemProperties.vendorId;
		system["trackingProperties"]["orientationTracking"] = systemProperties.trackingProperties.orientationTracking == XR_TRUE;
		system["trackingProperties"]["positionTracking"] = systemProperties.trackingProperties.positionTracking == XR_TRUE;
		system["graphicsProperties"]["maxLayerCount"] = systemProperties.graphicsProperties.maxLayerCount;
		system["graphicsProperties"]["maxSwapchainImageWidth"] = systemProperties.graphicsProperties.maxSwapchainImageWidth;
		system["graphicsProperties"]["maxSwapchainImageHeight"] = systemProperties.graphicsProperties.maxSwapchainImageHeight;

		std::cout << "system name " << systemProperties.systemName << "\n";
		std::cout << "vendor id " << systemProperties.vendorId << "\n";
		std::cout << "support orientation tracking " << systemProperties.trackingProperties.orientationTracking << "\n";
		std::cout << "support position tracking " << systemProperties.trackingProperties.positionTracking << "\n";

		if(has_opengl)
		{
			XrGraphicsRequirementsOpenGLKHR graphicsRequirementsOpenGL{ XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR };
			if(XR_SUCCEEDED(xrGetOpenGLGraphicsRequirementsKHR(instance, systemId, &graphicsRequirementsOpenGL)))
			{
				auto& OpenGL = system["GraphicsRequirements"]["OpenGL"];
				JsonAddVersionObject(OpenGL["minApiVersionSupported"], graphicsRequirementsOpenGL.minApiVersionSupported);
				JsonAddVersionObject(OpenGL["maxApiVersionSupported"], graphicsRequirementsOpenGL.maxApiVersionSupported);
			}
		}

		if(has_d3d11)
		{
			XrGraphicsRequirementsD3D11KHR graphicsRequirementsD3D11{ XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR };
			if (XR_SUCCEEDED(xrGetD3D11GraphicsRequirementsKHR(instance, systemId, &graphicsRequirementsD3D11)))
			{
				auto& D3D11 = system["GraphicsRequirements"]["D3D11"];
				//TODO this is just for this computer and is not interesting for a report
				//D3D11["adapterLuid"] = LuidToString(graphicsRequirementsD3D11.adapterLuid);
				D3D11["minFeatureLevel"] = FeatureLevelToString(graphicsRequirementsD3D11.minFeatureLevel);
			}
		}

		if(has_d3d12)
		{
			XrGraphicsRequirementsD3D12KHR graphicsRequirementsD3D12{ XR_TYPE_GRAPHICS_REQUIREMENTS_D3D12_KHR };
			if(XR_SUCCEEDED(xrGetD3D12GraphicsRequirementsKHR(instance, systemId, &graphicsRequirementsD3D12)))
			{
				auto& D3D12 = system["GraphicsRequirements"]["D3D12"];
				//TODO this is just for this computer and is not interesting for a report
				//D3D12["adapterLuid"] = LuidToString(graphicsRequirementsD3D12.adapterLuid);
				D3D12["minFeatureLevel"] = FeatureLevelToString(graphicsRequirementsD3D12.minFeatureLevel);
			}
		}

		if(has_vulkan)
		{
			XrGraphicsRequirementsVulkanKHR graphicsRequirementsVulkan{ XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR };
			if(XR_SUCCEEDED(xrGetVulkanGraphicsRequirementsKHR(instance, systemId, &graphicsRequirementsVulkan)))
			{
				auto& Vulkan = system["GraphicsRequirements"]["Vulkan"];
				JsonAddVersionObject(Vulkan["minApiVersionSupported"], graphicsRequirementsVulkan.minApiVersionSupported);
				JsonAddVersionObject(Vulkan["maxApiVersionSupported"], graphicsRequirementsVulkan.maxApiVersionSupported);
			}
		}

		//Note, there's no difference between Vulkan and Vulkan2 at this stage, it's just semantics. The graphics bindings structures are different tho.
		if(has_vulkan_2)
		{
			XrGraphicsRequirementsVulkan2KHR graphicsRequirementsVulkan2{ XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN2_KHR };
			if(XR_SUCCEEDED(xrGetVulkanGraphicsRequirements2KHR(instance, systemId, &graphicsRequirementsVulkan2)))
			{
				auto& Vulkan2 = system["GraphicsRequirements"]["Vulkan2"];
				JsonAddVersionObject(Vulkan2["minApiVersionSupported"], graphicsRequirementsVulkan2.minApiVersionSupported);
				JsonAddVersionObject(Vulkan2["maxApiVersionSupported"], graphicsRequirementsVulkan2.maxApiVersionSupported);
			}
		}
	}
}


int main()
{
	uint32_t nbLayers = 0;
	xrEnumerateApiLayerProperties(0, &nbLayers, nullptr);
	std::vector<XrApiLayerProperties> apiLayerList(nbLayers, { XR_TYPE_API_LAYER_PROPERTIES });
	xrEnumerateApiLayerProperties(apiLayerList.size(), &nbLayers, apiLayerList.data());


	uint32_t nbExtensions = 0;
	xrEnumerateInstanceExtensionProperties(nullptr, nbExtensions, &nbExtensions, nullptr);
	std::vector<XrExtensionProperties> extensionList(nbExtensions, { XR_TYPE_EXTENSION_PROPERTIES });
	xrEnumerateInstanceExtensionProperties(nullptr, nbExtensions, &nbExtensions, extensionList.data());

	std::map<std::string, std::string> ExtensionProvidedBy;
	for (const auto& layerProperties : apiLayerList)
	{
		const auto layerName = layerProperties.layerName;

		uint32_t nbLayerExtensions = 0;
		xrEnumerateInstanceExtensionProperties(layerName, nbLayerExtensions, &nbLayerExtensions, nullptr);
		std::vector<XrExtensionProperties> layerExtenesions(nbLayerExtensions, { XR_TYPE_EXTENSION_PROPERTIES });
		xrEnumerateInstanceExtensionProperties(layerName, nbLayerExtensions, &nbLayerExtensions, layerExtenesions.data());

		for(const auto& extensionProp : layerExtenesions)
			ExtensionProvidedBy[extensionProp.extensionName] = layerName;

		extensionList.insert(extensionList.end(), layerExtenesions.begin(), layerExtenesions.end());
	}


	auto& openxr_report = document["OpenXR"];

	std::cout << "Api Layers :\n";
	for (const auto& apiLayerProperties : apiLayerList)
	{
		std::cout << apiLayerProperties.layerName << " version " << apiLayerProperties.layerVersion << "\n";
		nlohmann::json layer;
		layer["name"] = apiLayerProperties.layerName;
		layer["version"] = (int)apiLayerProperties.layerVersion;
		layer["description"] = apiLayerProperties.description;
		openxr_report["layer_list"].push_back(layer);
	}

	std::cout << "All extensions (including layer ones) :\n";
	for (const auto& extensionProperties : extensionList)
	{
		std::cout << extensionProperties.extensionName << " version " << extensionProperties.extensionVersion;
		nlohmann::json extension;
		extension["name"] = extensionProperties.extensionName;
		extension["version"] = (int)extensionProperties.extensionVersion;

		if(const auto iterator = ExtensionProvidedBy.find(extensionProperties.extensionName); iterator != ExtensionProvidedBy.end())
		{
			auto& [extensionName, layerName] = *iterator;
			extension["providedBy"] = layerName;

			std::cout << " is provided by " << layerName;
		}

		std::cout << "\n";

		openxr_report["extension_list"].push_back(extension);
	}

	std::vector<const char*> enabledExtensions;
	if (IsSupported(extensionList, XR_KHR_VULKAN_ENABLE_EXTENSION_NAME))
	{
		enabledExtensions.push_back(XR_KHR_VULKAN_ENABLE_EXTENSION_NAME);
		has_vulkan = true;
	}
	if (IsSupported(extensionList, XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME))
	{
		enabledExtensions.push_back(XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME);
		has_vulkan_2 = true;
	}
	if (IsSupported(extensionList, XR_KHR_D3D11_ENABLE_EXTENSION_NAME))
	{
		enabledExtensions.push_back(XR_KHR_D3D11_ENABLE_EXTENSION_NAME);
		has_d3d11 = true;
	}
	if (IsSupported(extensionList, XR_KHR_D3D12_ENABLE_EXTENSION_NAME))
	{
		enabledExtensions.push_back(XR_KHR_D3D12_ENABLE_EXTENSION_NAME);
		has_d3d12 = true;
	}
	if (IsSupported(extensionList, XR_KHR_OPENGL_ENABLE_EXTENSION_NAME))
	{
		enabledExtensions.push_back(XR_KHR_OPENGL_ENABLE_EXTENSION_NAME);
		has_opengl = true;
	}


	openxr_report["version"] = "1.0.0";
	XrInstance instance = XR_NULL_HANDLE;
	XrInstanceCreateInfo instanceCreateInfo{ XR_TYPE_INSTANCE_CREATE_INFO };
	instanceCreateInfo.applicationInfo.apiVersion = XR_MAKE_VERSION(1, 0, 0);
	strcpy(instanceCreateInfo.applicationInfo.applicationName, "OpenXR-info");
	instanceCreateInfo.enabledExtensionNames = enabledExtensions.data();
	instanceCreateInfo.enabledExtensionCount = (uint32_t)enabledExtensions.size();
	if (XR_SUCCEEDED(xrCreateInstance(&instanceCreateInfo, &instance)))
	{

		XrSystemGetInfo systemGetInfoHandheld{ XR_TYPE_SYSTEM_GET_INFO, nullptr, XR_FORM_FACTOR_HANDHELD_DISPLAY };
		XrSystemId systemIdHandheld = 0;
		switch (xrGetSystem(instance, &systemGetInfoHandheld, &systemIdHandheld))
		{
		default:
			std::cout << "Unspecified error getting handheld system...\n";
			break;
		case XR_ERROR_FORM_FACTOR_UNAVAILABLE:
			std::cout << "Handled XR system is currently unavailable.\n";
			break;
		case XR_ERROR_FORM_FACTOR_UNSUPPORTED:
			std::cout << "XR instance do not support handheld mode.\n";
			break;
		case XR_SUCCESS:
			std::cout << "XR instance do support a handheld XR system\n";
			break;
		}

		XrSystemGetInfo systemGetInfoHMD{ XR_TYPE_SYSTEM_GET_INFO, nullptr, XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY };
		XrSystemId systemIdHMD = 0;
		switch (xrGetSystem(instance, &systemGetInfoHMD, &systemIdHMD))
		{
		default:
			std::cout << "Unspecified error getting HMD system...\n";
			break;
		case XR_ERROR_FORM_FACTOR_UNAVAILABLE:
			std::cout << "HMD XR system is currently unavailable.\n";
			break;
		case XR_ERROR_FORM_FACTOR_UNSUPPORTED:
			std::cout << "XR instance do not support HMD mode.\n";
			break;
		case XR_SUCCESS:
			std::cout << "XR instance do support a HMD XR system\n";
			break;
		}

		xrewInit(instance);

		QuerySystemProperties(instance, systemIdHandheld, openxr_report["XrSystem"]["handheldSystem"]);
		QuerySystemProperties(instance, systemIdHMD, openxr_report["XrSystem"]["hmdSystem"]);

		xrDestroyInstance(instance);
	}
	else
	{
		std::cout << "Cannot create instance. Will not query XrSystems\n";
	}

	const auto outputFileName = "report.json";

	//Dump the json document to a file on disk.
	{
		std::ofstream output_stream;
		output_stream.open(outputFileName);
		output_stream << document.dump(2);
	}

	std::cout << "See report file in : " << outputFileName << std::endl;
	return 0;
}