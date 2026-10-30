#include "Console.h"
#include "../gui/Platform.h"
#if defined(_WIN32)
// fora dos arquivos que incluem o raylib, que tem nomes iguais aos do Windows
#include <windows.h>
#include <shellapi.h>
#endif

std::vector<std::string> console::arguments(int argc, char *argv[])
{
	std::vector<std::string> result;
#if defined(_WIN32)
	SetConsoleOutputCP(CP_UTF8);
	int count;
	LPWSTR *wide = CommandLineToArgvW(GetCommandLineW(), &count);
	if(wide != NULL)
	{
		for(int i = 0; i < count; i++)
			result.push_back(pathToUtf8(std::filesystem::path(wide[i])));
		LocalFree(wide);
		return result;
	}
#endif
	for(int i = 0; i < argc; i++)
		result.push_back(argv[i]);
	return result;
}
