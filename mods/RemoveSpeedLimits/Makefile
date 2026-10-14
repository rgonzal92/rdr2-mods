# Cross-builds the plugin on Linux with clang and lld. No Windows SDK or C runtime is needed:
# the plugin calls ScriptHookRDR2.dll and one KERNEL32.dll function, and the import library for
# each comes from its .def file. `make test` builds and runs the logic checks natively.

build/RemoveSpeedLimits.asi: src/RemoveSpeedLimits.cpp src/SpeedLogic.h build/ScriptHookRDR2.lib build/Kernel32.lib
	clang++ --target=x86_64-pc-windows-msvc -std=c++17 -O2 -Wall -Wextra -shared -nostdlib -fuse-ld=lld -Wl,-entry:DllMain,-noimplib -o $@ src/RemoveSpeedLimits.cpp build/ScriptHookRDR2.lib build/Kernel32.lib

build/%.lib: src/%.def
	mkdir -p build
	llvm-dlltool -m i386:x86-64 -d $< -l $@

build/SpeedLogicTest: test/SpeedLogicTest.cpp src/SpeedLogic.h
	mkdir -p build
	clang++ -std=c++17 -O2 -Wall -Wextra -o $@ $<

.PHONY: test
test: build/SpeedLogicTest
	./build/SpeedLogicTest
