g_game.setLastSupportedVersion(1287)
--g_game.setCustomOs(6712)
g_logger.setLogFile(g_resources.getWorkDir() .. g_app.getCompactName() .. ".log")
g_logger.info(os.date("== app started at %b %d %Y %X"))
g_logger.info(g_app.getName() .. ' ' .. g_app.getVersion() .. ' rev ' .. g_app.getBuildRevision() .. ' (' .. g_app.getBuildCommit() .. ') built on ' .. g_app.getBuildDate() .. ' for arch ' .. g_app.getBuildArch())

if not g_app.isPacked() then
  if not g_resources.addSearchPath(g_resources.getWorkDir() .. "data", true) then
    g_logger.fatal("Unable to add data directory to the search path.")
  end
  if not g_resources.addSearchPath(g_resources.getWorkDir() .. "modules", true) then
    g_logger.fatal("Unable to add modules directory to the search path.")
  end
  g_resources.addSearchPath(g_resources.getWorkDir() .. "mods", true)
else
  g_resources.searchAndAddPackages("/", ".zip", true);
end
g_resources.setupUserWriteDir(('%s/'):format(g_app.getCompactName()))
g_resources.searchAndAddPackages('/', '.otpkg', true)
g_configs.loadSettings("/config.otml")
g_modules.discoverModules()
-- libraries modules 0-99
g_modules.autoLoadModules(99)
g_modules.ensureModuleLoaded("corelib")
g_modules.ensureModuleLoaded("gamelib")
-- client modules 100-499
g_modules.autoLoadModules(499)
g_modules.ensureModuleLoaded("client")
-- game modules 500-999
g_modules.autoLoadModules(999)
g_modules.ensureModuleLoaded("game_interface")
-- mods 1000-9999
g_modules.autoLoadModules(9999)
