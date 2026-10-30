#include "Platform.h"
#include <stdlib.h>

#if defined(_WIN32)
static std::filesystem::path getEnvPath(const wchar_t* name, const char* fallback)
{
	// _wgetenv evita a conversão para a página de código ANSI
	const wchar_t* value = _wgetenv(name);
	if(value == NULL || value[0] == L'\0')
		return fallback;
	return value;
}
#else
static std::filesystem::path getEnvPath(const char* name, const char* fallback)
{
	const char* value = getenv(name);
	if(value == NULL || value[0] == '\0')
		return fallback;
	return value;
}
#endif

std::filesystem::path pathFromUtf8(const std::string& str)
{
	return std::filesystem::path(std::u8string(str.begin(), str.end()));
}

std::string pathToUtf8(const std::filesystem::path& path)
{
	std::u8string str = path.u8string();
	return std::string(str.begin(), str.end());
}

static std::filesystem::path getDataDir()
{
	std::filesystem::path dir;
#if defined(_WIN32)
	dir = getEnvPath(L"APPDATA", ".");
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
	return dir;
}

std::string getDataFilePath()
{
	return pathToUtf8(getDataDir() / "NeuralNetWork.bin");
}

std::string getSettingsFilePath()
{
	return pathToUtf8(getDataDir() / "estado.txt");
}
