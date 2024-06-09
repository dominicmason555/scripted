declare module "wasm" {
  interface LuaManager {
    runScheduler: function
    init: function
  }

  interface Module {
    LuaManager: LuaManager
    print: function
  }

  interface Window {
    Module: Module
  }
}
