#include "Platform.h"
#include <filesystem>
#include <stdlib.h>

static std::filesystem::path getEnvPath(const char* name, const char* fallback)
{
	const char* value = getenv(name);
	if(value == NULL || value[0] == '\0')
		return fallback;
	return value;
}

std::string getDataFilePath()
{
	std::filesystem::path dir;
#if defined(_WIN32)
	dir = getEnvPath("APPDATA", ".");
#elif defined(__APPLE__)
	dir = getEnvPath("HOME", ".") / "Library" / "Application Support";
#else
	dir = getEnvPath("XDG_DATA_HOME", "");
	if(dir.empty())
		dir = getEnvPath("HOME", ".") / ".local" / "share";
#endif
	dir /= "RedesNeurais";
	std::error_code error;
	std::filesystem::create_directories(dir, error);
	return (dir / "NeuralNetWork.bin").string();
}
