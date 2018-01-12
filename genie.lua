-- build script

solution "SmMalloc"
	language "C++"

	flags {
		"NoManifest",
		"ExtraWarnings",
		"FloatFast",
		"StaticRuntime",
	}

	debugdir( "." )

	defines {
		"_CRT_SECURE_NO_WARNINGS",
		"_HAS_EXCEPTIONS=0",
		"_SCL_SECURE=0",
		"_SECURE_SCL=0",
	}

	location ( "Build/" .. _ACTION )

	local config_list = {
		"Debug",
		"Release",
	}

	local platform_list = {
		"x32",
		"x64",
	}

	configurations(config_list)
	platforms(platform_list)

configuration "Debug"
	defines {
		"_DEBUG",
		"_CRTDBG_MAP_ALLOC"
	}
	flags { 
		"Symbols"
	}


configuration "Release"
	defines	{
		"NDEBUG",
	}
	flags {
		"Symbols",
		"OptimizeSpeed"
	}


configuration "x32"
	flags {
		"EnableSSE2",
	}
	defines {
		"WIN32",
	}

configuration "x64"
	defines {
		"WIN32",
	}

--  give each configuration/platform a unique output/target directory
for _, config in ipairs(config_list) do
	for _, plat in ipairs(platform_list) do
		configuration { config, plat }
		objdir( "Build/" .. _ACTION .. "/tmp/"  .. config  .. "-" .. plat )
		targetdir( "Bin/" .. _ACTION .. "/" .. config .. "-" .. plat )
	end
end


project "UnitTest++"
	kind "StaticLib"
	defines {
		"_CRT_SECURE_NO_WARNINGS"
	}

	files {
		"ThirdParty/UnitTest++/UnitTest++/**.cpp",
		"ThirdParty/UnitTest++/UnitTest++/**.h", 
	}

	if isPosix or isOSX then
		excludes { "ThirdParty/UnitTest++/UnitTest++/Win32/**.*" }
	else
		excludes { "ThirdParty/UnitTest++/UnitTest++/Posix/**.*" }
	end

project "SmMalloc"
	kind "StaticLib"
	defines {
		"_CRT_SECURE_NO_WARNINGS"
	}

	files {
		"SmMalloc/**.*",
	}

project "Tests"
	kind "ConsoleApp"

	flags {
		"NoPCH",
	}

	files {
		"Tests/**.*", 
	}

	includedirs {
		"SmMalloc/",
		"ThirdParty/UnitTest++/UnitTest++",
	}

	links {
		"UnitTest++",
		"SmMalloc",
	}


project "PerfTest"
	kind "ConsoleApp"

	flags {
		"NoPCH",
	}

	files {
		"PerfTest/*.cpp", 
		"PerfTest/*.h",
		"PerfTest/Hoard/Source/libhoard.cpp",
		"PerfTest/Hoard/Source/wintls.cpp",
		"PerfTest/rpmalloc/*.*",
		"PerfTest/dlmalloc/*.*",
		"PerfTest/ltalloc/*.cc",
		"PerfTest/ltalloc/*.h",
		"PerfTest/ltalloc/*.hpp",
	}


	includedirs {
		"SmMalloc/",
		"PerfTest/Hoard/Heap-Layers/",
		"PerfTest/Hoard/include/",
		"PerfTest/Hoard/include/util",
		"PerfTest/Hoard/include/hoard",
		"PerfTest/Hoard/include/superblocks",
	}

	links {
		"SmMalloc",
	}

