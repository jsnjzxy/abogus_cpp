#include "abogus.h"

#include <cstring>
#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <mutex>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#include <dispatch/dispatch.h>
#include <pthread.h>
#elif defined(__linux__)
#include <unistd.h>
#include <sys/wait.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

#include <nlohmann/json.hpp>

// 版本信息
#define ABOGUS_VERSION_MAJOR 1
#define ABOGUS_VERSION_MINOR 0
#define ABOGUS_VERSION_PATCH 0
#define ABOGUS_VERSION_STRING "1.0.0"

namespace {

// ============================================================================
// 平台抽象层
// ============================================================================

#if defined(_WIN32)
#define POPEN _popen
#define PCLOSE _pclose
#else
#define POPEN popen
#define PCLOSE pclose
#endif

// 获取可执行文件所在目录
std::string get_executable_dir() {
#if defined(__APPLE__)
    char buf[4096];
    uint32_t size = sizeof(buf);
    if (_NSGetExecutablePath(buf, &size) == 0) {
        return std::filesystem::path(buf).parent_path().string();
    }
#elif defined(__linux__)
    char buf[4096];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len > 0) {
        buf[len] = '\0';
        return std::filesystem::path(buf).parent_path().string();
    }
#elif defined(_WIN32)
    char buf[MAX_PATH];
    if (GetModuleFileNameA(nullptr, buf, MAX_PATH) > 0) {
        return std::filesystem::path(buf).parent_path().string();
    }
#endif
    return "";
}

// 查找 get_abogus 可执行文件目录
std::string find_abogus_dir() {
    // 1. 优先使用环境变量
    if (const char* env = std::getenv("ABOGUS_JS_PATH")) {
        std::string p(env);
        if (std::filesystem::exists(p + "/get_abogus")) {
            return p;
        }
    }

    // 2. 相对于当前工作目录搜索
    std::filesystem::path cwd = std::filesystem::current_path();
    const char* cwd_rels[] = {
        "js",
        "abogus_js",
        "abogus_cpp/js",
        "3rdparty/abogus_cpp/js",
        "../js",
        "../abogus_js",
        "../../js",
        "../../abogus_js"
    };
    for (const auto* rel : cwd_rels) {
        auto p = (cwd / rel).string();
        if (std::filesystem::exists(p + "/get_abogus")) {
            return p;
        }
    }

    // 3. 相对于可执行文件目录搜索
    std::string exe_dir = get_executable_dir();
    if (!exe_dir.empty()) {
        const char* exe_rels[] = {
            "js",
            "abogus_js",
            "../Resources/js",           // macOS app bundle
            "../Resources/abogus_js"
        };
        for (const auto* rel : exe_rels) {
            auto p = (std::filesystem::path(exe_dir) / rel).string();
            if (std::filesystem::exists(p + "/get_abogus")) {
                return p;
            }
        }
    }

    return "";
}

// ============================================================================
// 可执行文件调用（使用 stdin/stdout 管道通信）
// ============================================================================

#if defined(_WIN32)

std::string run_executable_with_stdin(const std::string& execPath,
                                       const std::string& jsonInput) {
    std::string tmpPath = std::filesystem::temp_directory_path().string() +
                          "\\abogus_" +
                          std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
                          ".json";

    {
        std::ofstream tmpFile(tmpPath, std::ios::binary);
        if (!tmpFile) return "";
        tmpFile << jsonInput;
    }

    std::string resultCmd = "\"" + execPath + "\" \"" + tmpPath + "\" 2>nul";
    FILE* resultPipe = _popen(resultCmd.c_str(), "r");
    if (!resultPipe) {
        std::remove(tmpPath.c_str());
        return "";
    }

    std::string result;
    char buf[256];
    while (fgets(buf, sizeof(buf), resultPipe)) {
        result += buf;
    }
    _pclose(resultPipe);
    std::remove(tmpPath.c_str());

    while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
        result.pop_back();
    }
    return result;
}

#else

std::string run_executable_with_stdin(const std::string& execPath,
                                       const std::string& jsonInput) {
    int stdin_pipe[2];
    int stdout_pipe[2];

    if (pipe(stdin_pipe) < 0 || pipe(stdout_pipe) < 0) {
        return "";
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        return "";
    }

    if (pid == 0) {
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);

        dup2(stdin_pipe[0], STDIN_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);

        close(stdin_pipe[0]);
        close(stdout_pipe[1]);

        execl(execPath.c_str(), execPath.c_str(), nullptr);
        _exit(127);
    }

    close(stdin_pipe[0]);
    close(stdout_pipe[1]);

    write(stdin_pipe[1], jsonInput.c_str(), jsonInput.size());
    close(stdin_pipe[1]);

    std::string result;
    char buf[256];
    ssize_t n;
    while ((n = read(stdout_pipe[0], buf, sizeof(buf))) > 0) {
        result.append(buf, n);
    }
    close(stdout_pipe[0]);

    int status;
    waitpid(pid, &status, 0);

    while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
        result.pop_back();
    }

    return result;
}

#endif

// ============================================================================
// 高层封装
// ============================================================================

void print_abogus_hint() {
    std::filesystem::path cwd = std::filesystem::current_path();
    std::cerr << "[abogus] Lookup hint:\n"
              << "  Current directory: " << cwd.string() << "\n";

    std::string exe_dir = get_executable_dir();
    if (!exe_dir.empty()) {
        std::cerr << "  Executable directory: " << exe_dir << "\n";
    }

    std::cerr << "Solutions:\n"
              << "  1. Build get_abogus: cd js && npm install && ./build.sh\n"
              << "  2. Set environment: export ABOGUS_JS_PATH=/path/to/js\n";
}

std::string get_abogus_via_executable(const char* userAgent, const char* params) {
    std::string dir = find_abogus_dir();
    if (dir.empty()) {
        print_abogus_hint();
        return "";
    }

    std::string execPath = dir + "/get_abogus";
    if (!std::filesystem::exists(execPath)) {
        std::cerr << "[abogus] Error: " << execPath << " not found\n";
        return "";
    }

    nlohmann::json j;
    j["params"] = params ? std::string(params) : "";
    j["user_agent"] = userAgent ? std::string(userAgent) : "";
    std::string jsonStr = j.dump();

    std::string result = run_executable_with_stdin(execPath, jsonStr);

    if (result.empty() || result.substr(0, 5) == "ERROR") {
        std::cerr << "[abogus] Execution failed: " << result << "\n";
        return "";
    }

    return result;
}

} // anonymous namespace

// ============================================================================
// C 接口实现
// ============================================================================

static char* get_abogus_impl(const char* userAgent, const char* params) {
    try {
        std::string sigResult = get_abogus_via_executable(userAgent, params);
        if (sigResult.empty()) {
            return nullptr;
        }

        char* result = new char[sigResult.length() + 1];
        std::strcpy(result, sigResult.c_str());
        return result;
    } catch (const std::exception& e) {
        std::cerr << "[abogus] Exception: " << e.what() << "\n";
        return nullptr;
    } catch (...) {
        return nullptr;
    }
}

extern "C" {

const char* abogus_version(void) {
    return ABOGUS_VERSION_STRING;
}

char* get_abogus(const char* userAgent, const char* params) {
    if (!userAgent || !params) {
        return nullptr;
    }

#if defined(__APPLE__)
    if (!pthread_main_np()) {
        __block char* result = nullptr;
        dispatch_sync(dispatch_get_main_queue(), ^{
            result = get_abogus_impl(userAgent, params);
        });
        return result;
    }
#endif

    return get_abogus_impl(userAgent, params);
}

void free_abogus(char* ptr) {
    delete[] ptr;
}

} // extern "C"
