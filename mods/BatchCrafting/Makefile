# Cross-builds the plugin on Linux with clang and lld. No Windows SDK or C runtime is needed:
# the plugin calls ScriptHookRDR2.dll and four KERNEL32.dll functions, and the import library
# for each comes from its .def file. -fno-builtin keeps the compiler from turning the plugin's
# own loops into calls to that runtime's memset. `make test` builds and runs the logic checks
# natively.

build/BatchCrafting.asi: src/BatchCrafting.cpp src/BatchLogic.h build/ScriptHookRDR2.lib build/Kernel32.lib
	clang++ --target=x86_64-pc-windows-msvc -std=c++17 -O2 -Wall -Wextra -fno-builtin -shared -nostdlib -fuse-ld=lld -Wl,-entry:DllMain,-noimplib -o $@ src/BatchCrafting.cpp build/ScriptHookRDR2.lib build/Kernel32.lib

build/%.lib: src/%.def
	mkdir -p build
	llvm-dlltool -m i386:x86-64 -d $< -l $@

build/BatchLogicTest: test/BatchLogicTest.cpp src/BatchLogic.h
	mkdir -p build
	clang++ -std=c++17 -O2 -Wall -Wextra -o $@ $<

.PHONY: test
test: build/BatchLogicTest
	./build/BatchLogicTest
