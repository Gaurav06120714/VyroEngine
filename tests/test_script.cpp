// VyroEngine — Scripting tests
#include "vyro/scripting/ScriptEngine.hpp"
#include "vyro/scripting/ScriptHost.hpp"

#include "test_harness.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>

int main()
{
    using namespace vyro;
    test::Suite suite("script");

    // ScriptEngine_Eval_ArithmeticPrecedence (7.1)
    {
        ScriptEngine vm;
        suite.check(vm.eval("2 + 3 * 4").value_or(-1) == 14.0, "precedence: 2+3*4 == 14");
        suite.check(vm.eval("(2 + 3) * 4").value_or(-1) == 20.0, "parens: (2+3)*4 == 20");
        suite.check(vm.eval("-3 + 5").value_or(-1) == 2.0, "unary minus");
        suite.check(!vm.eval("1 / 0").has_value(), "division by zero errors");
    }

    // ScriptEngine_Run_VariablesAndConditionals (7.1)
    {
        ScriptEngine vm;
        const auto r = vm.run("x = 10\nif x > 5 { y = 1 } else { y = 2 }\n");
        suite.check(r.has_value(), "script runs");
        suite.check(vm.get("y") == 1.0, "if-branch taken");
        suite.check(vm.run("if x < 5 { z = 1 } else { z = 2 }\n").has_value(), "else script runs");
        suite.check(vm.get("z") == 2.0, "else-branch taken");
    }

    // ScriptEngine_Run_WhileLoop (7.1)
    {
        ScriptEngine vm;
        const auto r = vm.run("n = 0\ni = 5\nwhile i > 0 { n = n + i\ni = i - 1 }\n");
        suite.check(r.has_value(), "loop script runs");
        suite.check(vm.get("n") == 15.0, "while loop sums 5+4+3+2+1");
    }

    // ScriptEngine_HostFunction_Callable (7.2)
    {
        ScriptEngine vm;
        f64 spawned_x = 0.0;
        vm.register_function("spawn", [&](const std::vector<f64>& args) {
            spawned_x = args.empty() ? 0.0 : args[0];
            return 1.0;
        });
        const auto r = vm.run("ok = spawn(42, 0)\n");
        suite.check(r.has_value(), "host call runs");
        suite.check(spawned_x == 42.0, "host function received argument");
        suite.check(vm.get("ok") == 1.0, "host return value usable");
        suite.check(!vm.eval("missing_fn()").has_value(), "unknown function errors");
    }

    // ScriptHost_HotReload_DetectsChange (7.3)
    {
        const auto path = std::filesystem::temp_directory_path() / "vyro_test_script.vs";
        {
            std::ofstream f(path);
            f << "value = 1\n";
        }
        ScriptHost host(path.string());
        suite.check(host.load(), "initial load succeeds");
        suite.check(host.engine().get("value") == 1.0, "initial value = 1");

        // Rewrite with a newer mtime.
        {
            std::ofstream f(path);
            f << "value = 2\n";
        }
        std::filesystem::last_write_time(path,
                                         std::filesystem::last_write_time(path) + std::chrono::seconds(2));
        suite.check(host.poll(), "poll detects change");
        suite.check(host.engine().get("value") == 2.0, "reloaded value = 2");
        suite.check(host.reload_count() == 1, "one reload counted");
        suite.check(!host.poll(), "no further change, no reload");

        std::filesystem::remove(path);
    }

    return suite.summary();
}
