#include <cwchar>
#include "file_system.h"

namespace FileUtils {
	FILE* mywfopen(const wchar_t* filename, const char* mode) {
		char fname[1024]{};
		std::mbstate_t state{};
		std::wcsrtombs(fname, &filename, sizeof fname, &state);
		if (filename != nullptr)
			return nullptr;
		return std::fopen(fname, mode);
	}

	void SafeFileName(wchar_t* wfile) {
		wchar_t* cursor = wfile;
		while ((cursor = std::wcspbrk(cursor, L"<>:\"/\\|?*")) != nullptr) {
			*cursor = L'_';
			cursor++;
		}
	}

	bool IsFileExists(const std::filesystem::path& path) {
		std::error_code ec;
		return std::filesystem::is_regular_file(path, ec);
	}

	bool IsDirExists(const std::filesystem::path& path) {
		std::error_code ec;
		return std::filesystem::is_directory(path, ec);
	}

	bool MakeDir(const std::filesystem::path& path) {
		std::error_code ec;
		return std::filesystem::create_directory(path, ec);
	}

	bool Rename(const std::filesystem::path& oldpath, const std::filesystem::path& newpath) {
		std::error_code ec;
		std::filesystem::rename(oldpath, newpath, ec);
		return !ec;
	}

	bool DeleteDir(const std::filesystem::path& path) {
		std::error_code ec;
		std::filesystem::remove_all(path, ec);
		return !ec;
	}

	bool RemoveFile(const std::filesystem::path& path) {
		std::error_code ec;
		return std::filesystem::remove(path, ec);
	}
}
