#include <array>
#include <cstring>
#include <iostream>
#include <string>

#include "emscripten/bind.h"
#include "tinyalloc.h"

extern "C" {
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}

using namespace emscripten;

constexpr size_t LUA_MEM_SIZE = 640000; // 640k ought to be enough for anybody
constexpr size_t MAX_THREADS = 16; // >16 threads should be too hard to follow

void* allocator(void* ud, void* ptr, size_t osize, size_t nsize)
{
    // Free
    if (nsize == 0)
    {
        ta_free(ptr);
        return nullptr;
    }
    // Realloc
    else if (ptr != nullptr && nsize > osize)
    {
        void* new_ptr = ta_alloc(nsize);
        memcpy(new_ptr, ptr, osize);
        ta_free(ptr);
        return new_ptr;
    }
    // Malloc
    else if (ptr == nullptr && nsize > 0)
    {
        return ta_alloc(nsize);
    }
    else
    {
        return ptr;
    }
}

struct LuaThread
{
    bool active;
    std::string name;
    std::string functionName;
    int function;
    uint32_t nextRun;
};

class LuaManager
{
private:
    static lua_State* L;
    static bool initialised;
    static uint8_t LUA_MEM[LUA_MEM_SIZE];
    static int callback_ref;
    static std::array<LuaThread, MAX_THREADS> threads;

public:
    static bool isInitialised() { return initialised; }

    static int registerCallback(lua_State* L)
    {
        callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);
        return 0;
    }

    static bool callbackReady()
    {
        if (initialised)
        {
            int type = lua_rawgeti(L, LUA_REGISTRYINDEX, callback_ref);
            return type == LUA_TFUNCTION;
        }
        else
        {
            return false;
        }
    }

    static void runCallback()
    {
        if (initialised)
        {
            int type = lua_rawgeti(L, LUA_REGISTRYINDEX, callback_ref);
            int res = lua_pcall(L, 0, 0, 0);
            if (res)
            {
                std::cout << "Error calling back: " << lua_tostring(L, -1)
                          << std::endl;
            }
        }
    }

    static int doString(std::string code)
    {
        if (initialised)
        {
            int res = luaL_dostring(L, code.c_str());
            if (res)
            {
                std::cout << "Lua Error: " << lua_tostring(L, -1) << std::endl;
            }
            return res;
        }
        else
        {
            return -1;
        }
    }

    static void init()
    {
        if (!initialised)
        {
            // std::cerr << "Initialising allocator" << std::endl;
            ta_init(LUA_MEM, LUA_MEM + LUA_MEM_SIZE - 1, 512, 16, 4);
            // std::cerr << "Creating Lua state" << std::endl;
            L = lua_newstate(allocator, nullptr);
            // std::cerr << "Opening Lua libraries" << std::endl;
            luaL_openlibs(L);
            initialised = true;
            // std::cerr << "Lua initialised" << std::endl;
            lua_pushcfunction(L, registerCallback);
            lua_setglobal(L, "registerCallback");
        }
    }
};

lua_State* LuaManager::L = nullptr;
bool LuaManager::initialised = false;
uint8_t LuaManager::LUA_MEM[LUA_MEM_SIZE] = { 0 };
int LuaManager::callback_ref = -1;
std::array<LuaThread, MAX_THREADS> threads = { 0 };

int addTwoNums(int a, int b) { return a + b; }

int main(int argc, char** argv)
{
    LuaManager::init();

    return 0;
}

EMSCRIPTEN_BINDINGS(webbed)
{
    function("addTwoNums", &addTwoNums);
    class_<LuaManager>("LuaManager")
        .class_function("init", &LuaManager::init)
        .class_function("isInitialised", &LuaManager::isInitialised)
        .class_function("runCallback", &LuaManager::runCallback)
        .class_function("callbackReady", &LuaManager::callbackReady)
        .class_function("doString", &LuaManager::doString);
    /* .class_function("runScheduler", &LuaManager::runScheduler); */
}
