require("premake", ">=5.0.0-alpha15")

solution "Reweighting Firefly"
  location "build"
  configurations { "Debug", "Release" }
  platforms {"x64"}

project "Reweighting Firefly"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++17"
   characterset "MBCS"
   sysincludedirs {
     "vendor/glm",
     "vendor/tinyexr",
     "vendor/stb"
   }
   files {
     "main.cpp",
   }
   filter "configurations:Release"
     optimize "Speed"
