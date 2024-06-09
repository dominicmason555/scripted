declare module "wasm" {
  interface LuaManager {
    runCallback: function;
    callbackReady: function;
  }

  interface Module {
    LuaManager: LuaManager;
  }

  interface Window {
    Module: Module
  }
}
