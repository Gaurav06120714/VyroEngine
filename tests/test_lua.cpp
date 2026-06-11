// VyroEngine — Lua scripting backend tests
// Mirrors the VyroScript test surface so the two engines stay interchangeable.
#include "vyro/scripting/LuaScriptEngine.hpp"

#include "test_harness.hpp"

int main()
{
    using namespace vyro;
    test::Suite suite("lua");

    // Lua_Eval_Arithmetic
    {
        LuaScriptEngine vm;
        suite.check(vm.eval("2 + 3 * 4").value_or(-1) == 14.0, "precedence: 2+3*4 == 14");
        suite.check(vm.eval("(2 + 3) * 4").value_or(-1) == 20.0, "parens: (2+3)*4 == 20");
        suite.check(!vm.eval("nonsense(").has_value(), "syntax error reported");
        suite.check(!vm.last_error().empty(), "error message captured");
    }

    // Lua_Run_GlobalsAndControlFlow
    {
        LuaScriptEngine vm;
        const auto r = vm.run("x = 10\nif x > 5 then y = 1 else y = 2 end");
        suite.check(r.has_value(), "script runs");
        suite.check(vm.get("y") == 1.0, "if-branch taken");
        suite.check(vm.run("n = 0\nfor i = 1, 5 do n = n + i end").has_value(), "loop runs");
        suite.check(vm.get("n") == 15.0, "for loop sums 1..5");
        suite.check(vm.has("x") && !vm.has("missing"), "has() reflects globals");
    }

    // Lua_SetGet_RoundTrips
    {
        LuaScriptEngine vm;
        vm.set("speed", 3.5);
        suite.check(vm.get("speed") == 3.5, "host-set global readable");
        suite.check(vm.eval("speed * 2").value_or(-1) == 7.0, "script reads host global");
    }

    // Lua_HostFunction_Callable
    {
        LuaScriptEngine vm;
        f64 received = 0.0;
        vm.register_function("spawn", [&](const std::vector<f64>& args) {
            received = args.empty() ? 0.0 : args[0];
            return 99.0;
        });
        const auto r = vm.run("result = spawn(42)");
        suite.check(r.has_value(), "host call runs");
        suite.check(received == 42.0, "host received argument");
        suite.check(vm.get("result") == 99.0, "host return value visible in Lua");
    }

    // Lua_Reset_FreshGlobalsKeepsHostFunctions
    {
        LuaScriptEngine vm;
        vm.register_function("ping", [](const std::vector<f64>&) { return 7.0; });
        vm.set("stale", 1.0);
        vm.reset();
        suite.check(!vm.has("stale"), "globals cleared by reset");
        suite.check(vm.eval("ping()").value_or(-1) == 7.0, "host function survives reset");
    }

    return suite.summary();
}
