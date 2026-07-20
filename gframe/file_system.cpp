#include <cwchar>
#include "file_system.h"

FILE* mywfopen(const wchar_t* filename, const char* mode) {
	char fname[1024]{};
	std::mbstate_t state{};
	std::wcsrtombs(fname, &filename, sizeof fname, &state);
	if (filename != nullptr)
		return nullptr;
	return std::fopen(fname, mode);
}

void FileSystem::SafeFileName(wchar_t* wfile) {
	wchar_t* cursor = wfile;
	while ((cursor = std::wcspbrk(cursor, L"<>:\"/\\|?*")) != nullptr) {
		*cursor = L'_';
		cursor++;
	}
}

bool FileSystem::IsFileExists(const std::filesystem::path& path) {
	std::error_code ec;
	return std::filesystem::is_regular_file(path, ec);
}

bool FileSystem::IsDirExists(const std::filesystem::path& path) {
	std::error_code ec;
	return std::filesystem::is_directory(path, ec);
}

bool FileSystem::MakeDir(const std::filesystem::path& path) {
	std::error_code ec;
	return std::filesystem::create_directory(path, ec);
}

bool FileSystem::Rename(const std::filesystem::path& oldpath, const std::filesystem::path& newpath) {
	std::error_code ec;
	std::filesystem::rename(oldpath, newpath, ec);
	return !ec;
}

bool FileSystem::DeleteDir(const std::filesystem::path& path) {
	std::error_code ec;
	std::filesystem::remove_all(path, ec);
	return !ec;
}

bool FileSystem::RemoveFile(const std::filesystem::path& path) {
	std::error_code ec;
	return std::filesystem::remove(path, ec);
}
