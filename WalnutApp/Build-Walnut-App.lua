project "WalnutApp"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++20"
   targetdir "bin/%{cfg.buildcfg}"
   staticruntime "off"

   files { "src/**.h", "src/**.cpp" }

   includedirs
   {
      "../vendor/imgui",
      "../vendor/GLFW/include",

      "../Walnut/Source",
      "../Walnut/Platform/GUI",

      "../vendor/simavr",

      "%{IncludeDir.VulkanSDK}",
      "%{IncludeDir.glm}",
   }

    links
    {
        "Walnut",
        "simavr",
        "ImGui",
        "GLFW",
        "%{Library.Vulkan}",
    }

   targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
   objdir ("../bin-int/" .. outputdir .. "/%{prj.name}")

   filter "system:windows"
      systemversion "latest"
      defines { "WL_PLATFORM_WINDOWS" }

   filter "configurations:Debug"
      defines { "WL_DEBUG" }
      runtime "Debug"
      symbols "On"

   filter "configurations:Release"
      defines { "WL_RELEASE" }
      runtime "Release"
      optimize "On"
      symbols "On"

   filter "configurations:Dist"
      kind "WindowedApp"
      defines { "WL_DIST" }
      runtime "Release"
      optimize "On"
      symbols "Off"