/*
 * Copyright (c) 2010-2022 OTClient <https://github.com/edubart/otclient>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "application.h"
#include "asyncdispatcher.h"
#include <csignal>
#include <framework/core/configmanager.h>
#include <framework/core/eventdispatcher.h>
#include <framework/core/modulemanager.h>
#include <framework/core/resourcemanager.h>
#include <framework/luaengine/luainterface.h>
#include <framework/platform/crashhandler.h>
#include <framework/platform/platform.h>

#include <gitinfo.h>

#define ADD_QUOTES_HELPER(s) #s
#define ADD_QUOTES(s) ADD_QUOTES_HELPER(s)

#include <locale>

#ifdef FRAMEWORK_NET
#include <framework/net/connection.h>
#endif

void exitSignalHandler(int sig)
{
    static bool signaled = false;
    switch (sig) {
        case SIGTERM:
        case SIGINT:
            if (!signaled && !g_app.isStopping() && !g_app.isTerminated()) {
                signaled = true;
                g_dispatcher.addEvent([ObjectPtr = &g_app] { ObjectPtr->close(); });
            }
            break;
    }
}

void Application::initFileMap(
#if ENCRYPTION_PACKED == 1
    const char*
#else
    const char *applicationPath
#endif
) {
#if ENCRYPTION_PACKED == 1
    g_app.MODULE_FILE_COUNT = 1;
    
    uintmax_t moduleRTsize = 0;
    try {
        moduleRTsize = std::filesystem::file_size(g_resources.getWorkDir() + "modules.zip");
        std::cout << "modules size : " << moduleRTsize << std::endl;
    } catch(...) {}
    g_app.MODULE_SIZE = moduleRTsize;
#else
    g_app.FILE_SIZE = std::filesystem::file_size(applicationPath);
    g_app.MODULE_SIZE = 0;
    g_app.MODULE_FILE_COUNT = 0;
    std::vector<std::string> excludedExtensions = { XorStr(".lua"),XorStr(".otui"),XorStr(".otmod") };
    for (const auto& entry : std::filesystem::recursive_directory_iterator("./")) {
        std::string ext = entry.path().extension().string();
        if (std::find(excludedExtensions.begin(), excludedExtensions.end(), ext) != excludedExtensions.end())
        {
            g_app.MODULE_FILE_COUNT++;
            g_app.MODULE_SIZE += entry.file_size();
        }
    }
#endif
}

void Application::init(std::vector<std::string>& args)
{
    // capture exit signals
    signal(SIGTERM, exitSignalHandler);
    signal(SIGINT, exitSignalHandler);

#ifdef CRASH_HANDLER
    installCrashHandler();
#endif

    // setup locale
    std::locale::global(std::locale());

    // process args encoding
    g_platform.processArgs(args);

    g_asyncDispatcher.init();

    std::string startupOptions;
    for (uint32_t i = 1; i < args.size(); ++i) {
        const auto& arg = args[i];
        startupOptions += " ";
        startupOptions += arg;
    }
    if (startupOptions.length() > 0)
        g_logger.info(stdext::format("Startup options: %s", startupOptions));

    m_startupOptions = startupOptions;

    // initialize configs
    g_configs.init();

    // initialize resources
    g_resources.init(args[0].data());

    // initialize lua
    g_lua.init();
    registerLuaFunctions();
}

void Application::deinit()
{
    g_lua.callGlobalField("g_app", "onTerminate");

    // run modules unload events
    g_modules.unloadModules();
    g_modules.clear();

    // release remaining lua object references
    g_lua.collectGarbage();

    // poll remaining events
    poll();

    g_asyncDispatcher.terminate();

    // disable dispatcher events
    g_dispatcher.shutdown();
}

void Application::terminate()
{
#ifdef FRAMEWORK_NET
    // terminate network
    Connection::terminate();
#endif

    // release configs
    g_configs.terminate();

    // release resources
    g_resources.terminate();

    // terminate script environment
    g_lua.terminate();

    m_terminated = true;

    signal(SIGTERM, SIG_DFL);
    signal(SIGINT, SIG_DFL);
}

void Application::poll()
{
#ifdef FRAMEWORK_NET
    Connection::poll();
#endif

    g_dispatcher.poll();

    // poll connection again to flush pending write
#ifdef FRAMEWORK_NET
    Connection::poll();
#endif
}

void Application::exit()
{
    g_lua.callGlobalField<bool>("g_app", "onExit");
    m_stopping = true;
}

void Application::close()
{
    if (!g_lua.callGlobalField<bool>("g_app", "onClose"))
        exit();
}

std::string Application::getOs()
{
#if defined(WIN32)
    return "windows";
#elif defined(__APPLE__)
    return "mac";
#elif __linux
    return "linux";
#elif ANDROID
    return "android";
#else
    return "unknown";
#endif
}

std::string Application::getClientFileSize() { return std::format("{}", FILE_SIZE); }
std::string Application::getClientModulesSize() { return std::format("{}", MODULE_SIZE); }
std::string Application::getClientModuleFileCount() { return std::format("{}", MODULE_FILE_COUNT); }

// https://stackoverflow.com/a/46448040
std::string Application::getBuildRevision()
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(3) << (static_cast<float>(GIT_COMMITS) / 1000);
    return ss.str();
}
std::string Application::getVersion() { return ADD_QUOTES(GIT_VERSION); }
std::string Application::getBuildCommit() { return ADD_QUOTES(GIT_BRANCH); }