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

#include <client/client.h>
#include <framework/core/application.h>
#include <framework/core/resourcemanager.h>
#include <framework/luaengine/luainterface.h>
#include "framework/util/compileXor.h"
#include <physfs.h>

int main(int argc, const char* argv[])
{
#if ENABLE_CONSOLE == 1
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
#endif
    std::vector<std::string> args(argv, argv + argc);
    // initialize some security things
    g_app.initFileMap(argv[0]);

    // setup application name and version
    g_app.setName(XorStr("ReTibia Client"));
    g_app.setCompactName(XorStr("OTC"));
    g_app.setOrganizationName(XorStr("ReTibia"));

#if ENABLE_ENCRYPTION == 1 && ENABLE_ENCRYPTION_BUILDER == 1
    if (std::find(args.begin(), args.end(), "--encrypt") != args.end()) {
        g_lua.init();
        g_resources.init(args[0].data());
        g_resources.runEncryption(args.size() >= 3 ? args[2] : ENCRYPTION_PASSWORD);
        std::cout << "Encryption complete" << std::endl;
    #ifdef WIN32
        MessageBoxA(NULL, "Encryption complete", "Success", 0);
    #endif
        return 0;
    }
#endif

    // initialize application framework and otclient
    g_app.init(args);
    Client::init(args);

    // find script init.lua and run it
    if (!g_resources.discoverWorkDir(g_resources.getWorkDir() + XorStr("otclient.exe")))
        g_logger.fatal(XorStr("Unable to find work directory, the application cannot be initialized."));

#if ENCRYPTION_PACKED == 1
    g_resources.searchAndAddPackages(XorStr("/"), XorStr(".zip"));

    if (!g_lua.safeRunScript(XorStr("init.lua")))
        g_logger.fatal(XorStr("Unable to run script init.lua!"));
#else
    if (!g_lua.safeRunScript(XorStr("init.lua")))
    {
        if (!g_resources.addSearchPath(g_resources.getWorkDir() + "data", true))
            g_logger.fatal(XorStr("Unable to add data to the search path"));

        if (!g_lua.safeRunScript("init.lua"))
            g_logger.fatal(XorStr("Unable to run script init.lua!"));
    }
#endif
    // the run application main loop
    g_app.run();

    // unload modules
    g_app.deinit();

    // terminate everything and free memory
    Client::terminate();
    g_app.terminate();
    return 0;
}
