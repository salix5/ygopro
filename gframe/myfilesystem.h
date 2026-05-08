#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <filesystem>
#include <functional>

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
		return std::filesystem::is_regular_file(path);
	}

	static bool IsDirExists(const std::filesystem::path& path) {
		return std::filesystem::is_directory(path);
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
		for (auto& entry : std::filesystem::directory_iterator(path, ec)) {
			cb(entry.path().filename().string().c_str(), entry.is_directory());
		}
	}

	static void TraversalDir(const std::filesystem::path& path, const std::function<void(const wchar_t*, bool)>& cb) {
		std::error_code ec;
		for (auto& entry : std::filesystem::directory_iterator(path, ec)) {
			cb(entry.path().filename().wstring().c_str(), entry.is_directory());
		}
	}
};

#endif //FILESYSTEM_H
