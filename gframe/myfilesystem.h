#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <cwchar>
#include <algorithm>
#include <filesystem>
#include <functional>
#include <vector>

class FileSystem {
public:
	static void SafeFileName(wchar_t* wfile) {
		wchar_t* cursor = wfile;
		while ((cursor = std::wcspbrk(cursor, L"<>:\"/\\|?*")) != nullptr) {
			*cursor = L'_';
			cursor++;
		}
	}

	static bool IsFileExists(const std::filesystem::path& path) {
		std::error_code ec;
		return std::filesystem::is_regular_file(path, ec);
	}

	static bool IsDirExists(const std::filesystem::path& path) {
		std::error_code ec;
		return std::filesystem::is_directory(path, ec);
	}

	static bool MakeDir(const std::filesystem::path& path) {
		std::error_code ec;
		return std::filesystem::create_directory(path, ec);
	}

	static bool Rename(const std::filesystem::path& oldpath, const std::filesystem::path& newpath) {
		std::error_code ec;
		std::filesystem::rename(oldpath, newpath, ec);
		return !ec;
	}

	static bool DeleteDir(const std::filesystem::path& path) {
		std::error_code ec;
		std::filesystem::remove_all(path, ec);
		return !ec;
	}

	static bool RemoveFile(const std::filesystem::path& path) {
		std::error_code ec;
		return std::filesystem::remove(path, ec);
	}

	static void TraversalDir(const std::filesystem::path& path, const std::function<void(const char*, bool)>& cb) {
		std::error_code ec;
		for(auto it = std::filesystem::directory_iterator(path, ec); it != std::filesystem::directory_iterator{}; it.increment(ec)) {
			cb(it->path().filename().string().c_str(), it->is_directory(ec));
		}
	}

	static void TraversalDir(const std::filesystem::path& path, const std::function<void(const wchar_t*, bool)>& cb) {
		std::error_code ec;
		for(auto it = std::filesystem::directory_iterator(path, ec); it != std::filesystem::directory_iterator{}; it.increment(ec)) {
			cb(it->path().filename().wstring().c_str(), it->is_directory(ec));
		}
	}
};

#endif //FILESYSTEM_H
