declare module "wasm" {
  interface LuaManager {
    runCallback: function
    callbackReady: function
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
