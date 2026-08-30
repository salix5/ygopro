#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <filesystem>
#include <type_traits>

namespace FileUtils {
	FILE* mywfopen(const wchar_t* filename, const char* mode);
	void SafeFileName(wchar_t* wfile);
	bool IsFileExists(const std::filesystem::path& path);
	bool IsDirExists(const std::filesystem::path& path);
	bool MakeDir(const std::filesystem::path& path);
	bool Rename(const std::filesystem::path& oldpath, const std::filesystem::path& newpath);
	bool DeleteDir(const std::filesystem::path& path);
	bool RemoveFile(const std::filesystem::path& path);
	template<typename Callback>
	void TraversalDir(const std::filesystem::path& path, Callback&& cb) {
		static_assert(std::is_invocable_v<Callback, const std::filesystem::path&, bool>, "Callback must be invocable with (const std::filesystem::path&, bool)");
		std::error_code ec;
		for(auto it = std::filesystem::directory_iterator(path, ec); it != std::filesystem::directory_iterator{}; it.increment(ec)) {
			cb(it->path(), it->is_directory(ec));
		}
	}
}

#endif //FILESYSTEM_H
