-- WalnutExternal.lua

VULKAN_SDK = os.getenv("VULKAN_SDK")

IncludeDir = {}
IncludeDir["VulkanSDK"] = "%{VULKAN_SDK}/Include"
IncludeDir["glm"] = "../vendor/glm"
IncludeDir["spdlog"] = "../vendor/spdlog/include"

LibraryDir = {}
LibraryDir["VulkanSDK"] = "%{VULKAN_SDK}/Lib"

Library = {}
if os.host() == "windows" then
    Library["Vulkan"] = "%{LibraryDir.VulkanSDK}/vulkan-1.lib"
else
    Library["Vulkan"] = "vulkan"
end

group "Dependencies"
   include "vendor/imgui"
   include "vendor/GLFW"
   include "vendor/yaml-cpp"
   include "vendor/simavr"
group ""

group "Core"
    include "Walnut/Build-Walnut.lua"

    -- Optional modules
    if os.isfile("Walnut-Modules/Walnut-Networking/Build-Walnut-Networking.lua") then
        include "Walnut-Modules/Walnut-Networking/Build-Walnut-Networking.lua"
    end
group ""