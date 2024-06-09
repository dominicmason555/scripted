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

    static int getNextFreeThread()
    {
        int res = -1;
        for (int i = 0; i < MAX_THREADS; i++)
        {
            if (threads[i].active == false)
            {
                res = i;
                break;
            }
        }
        return res;
    }

public:
    // Functions callable from Lua:

    static int registerCallback(lua_State* L)
    {
        callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);
        return 0;
    }

    static int createThread(lua_State* L)
    {
        int res = -1;
        int threadIdx = getNextFreeThread();
        if (threadIdx != -1)
        {
            int nargs = lua_gettop(L);
            if (nargs < 2)
            {
                lua_pushliteral(
                    L, "createThread takes 2 args: name and function");
                lua_error(L);
            }
            std::string name = luaL_checkstring(L, 1);
            luaL_checktype(L, 2, LUA_TFUNCTION);
            std::string funName = nargs == 3 ? luaL_checkstring(L, 3) : name;
            lua_insert(L, -2);
            threads[threadIdx]
                = { true, name, funName, luaL_ref(L, LUA_REGISTRYINDEX), 0 };

            res = 0;
        }
        else
        {
            lua_pushfstring(
                L, "Reached %d threads, which is the maximum", MAX_THREADS);
            lua_error(L);
        }
        return res;
    }

    static int continueAt(lua_State* L)
    {
        int newFunc = 0;
        std::string newName;
        int nargs = lua_gettop(L);
        uint32_t when = luaL_checkinteger(L, 1);
        if (nargs > 1)
        {
            newFunc = luaL_ref(L, LUA_REGISTRYINDEX);
        }
        if (nargs > 2)
        {
            newName = luaL_checkstring(L, 3);
            std::cout << "new name" << newName << std::endl;
        }
        lua_getglobal(L, "THREAD_IDX");
        int foundIdx = 0;
        int threadIdx = lua_tointegerx(L, -1, &foundIdx);
        if (foundIdx != 0)
        {
            if ((threadIdx >= 0) && (threadIdx < MAX_THREADS))
            {
                threads[threadIdx].active = true;
                threads[threadIdx].nextRun = when;
                if (nargs > 1)
                {
                    threads[threadIdx].function = newFunc;
                }
                if (nargs > 2)
                {
                    threads[threadIdx].functionName = newName;
                }
            }
        }
        return 0;
    }

    // Functions callable from JS:

    static bool isInitialised() { return initialised; }

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

    static int runScheduler(uint32_t now)
    {
        int ret = -1;
        if (initialised)
        {
            ret = 0;
            for (int threadIdx = 0; threadIdx < MAX_THREADS; threadIdx++)
            {
                if (threads[threadIdx].active
                    && (threads[threadIdx].nextRun <= now))
                {
                    ret++;
                    // Lua function can re-set this using `continueAt`
                    threads[threadIdx].active = false;
                    // Store which thread we're in to a global
                    lua_pushinteger(L, threadIdx);
                    lua_setglobal(L, "THREAD_IDX");
                    int type = lua_rawgeti(
                        L, LUA_REGISTRYINDEX, threads[threadIdx].function);
                    if (type == LUA_TFUNCTION)
                    {
                        // Call thread function with time
                        lua_pushinteger(L, now);
                        int res = lua_pcall(L, 1, 0, 0);
                        if (res)
                        {
                            std::cout << "Error in thread "
                                      << threads[threadIdx].name << ": "
                                      << lua_tostring(L, -1) << std::endl;
                        }
                    }
                }
            }
        }
        return ret;
    }

    static int doString(std::string code)
    {
        int ret = -1;
        if (initialised)
        {
            threads = { 0 };
            ret = luaL_dostring(L, code.c_str());
            if (ret)
            {
                std::cout << "Lua Error: " << lua_tostring(L, -1) << std::endl;
            }
        }
        return ret;
    }

    static void init()
    {
        // std::cerr << "Initialising allocator" << std::endl;
        ta_init(LUA_MEM, LUA_MEM + LUA_MEM_SIZE - 1, 512, 16, 4);
        // std::cerr << "Creating Lua state" << std::endl;
        L = lua_newstate(allocator, nullptr);
        // std::cerr << "Opening Lua libraries" << std::endl;
        luaL_openlibs(L);
        threads = { 0 };
        initialised = true;
        // std::cerr << "Lua initialised" << std::endl;
        lua_pushcfunction(L, registerCallback);
        lua_setglobal(L, "registerCallback");
        lua_pushcfunction(L, createThread);
        lua_setglobal(L, "createThread");
        lua_pushcfunction(L, continueAt);
        lua_setglobal(L, "continueAt");
    }
};

lua_State* LuaManager::L = nullptr;
bool LuaManager::initialised = false;
uint8_t LuaManager::LUA_MEM[LUA_MEM_SIZE] = { 0 };
int LuaManager::callback_ref = -1;
std::array<LuaThread, MAX_THREADS> LuaManager::threads = { 0 };

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
        .class_function("doString", &LuaManager::doString)
        .class_function("runScheduler", &LuaManager::runScheduler);
}
