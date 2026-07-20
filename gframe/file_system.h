#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <filesystem>
#include <type_traits>

FILE* mywfopen(const wchar_t* filename, const char* mode);

class FileSystem {
public:
	static void SafeFileName(wchar_t* wfile);
	static bool IsFileExists(const std::filesystem::path& path);
	static bool IsDirExists(const std::filesystem::path& path);
	static bool MakeDir(const std::filesystem::path& path);
	static bool Rename(const std::filesystem::path& oldpath, const std::filesystem::path& newpath);
	static bool DeleteDir(const std::filesystem::path& path);
	static bool RemoveFile(const std::filesystem::path& path);
	template<typename Callback>
	static void TraversalDir(const std::filesystem::path& path, Callback&& cb) {
		static_assert(std::is_invocable_v<Callback, const std::filesystem::path&, bool>, "Callback must be invocable with (const std::filesystem::path&, bool)");
		std::error_code ec;
		for(auto it = std::filesystem::directory_iterator(path, ec); it != std::filesystem::directory_iterator{}; it.increment(ec)) {
			cb(it->path(), it->is_directory(ec));
		}
	}
};

#endif //FILESYSTEM_H
