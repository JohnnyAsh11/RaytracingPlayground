#ifndef __FILEHELPER_H_
#define __FILEHELPER_H_

#define MAX_FILEPATH 512

inline std::wstring ToWideChar(const std::string& str) 
{ 
	return std::wstring(str.begin(), str.end()); 
}

inline std::string GetExecutableDir()
{
	std::string sExePath = ".\\";
	char dir[MAX_FILEPATH] = {};
	GetModuleFileNameA(NULL, dir, MAX_FILEPATH);

	// Replacing the actual executable file name with 
	// nothing so that the string functions as a filepath.
	char* lastSlash = strrchr(dir, '\\');
	if (lastSlash)
	{
		*lastSlash = 0;
		sExePath = dir;
	}

	return sExePath;
}

/// <summary>
/// Using a wchar string, updates the passed in file path according to the exe directory.
/// </summary>
inline std::wstring FromExeDir(const std::wstring& relativeFilePath)
{
	return ToWideChar(GetExecutableDir()) + L"\\" + relativeFilePath;
}

/// <summary>
/// Using a standard string, updates the passed in file path according to the exe directory.
/// </summary>
inline std::string FromExeDir(const std::string& relativeFilePath)
{
	return GetExecutableDir() + "\\" + relativeFilePath;
}


#endif //__FILEHELPER_H_
