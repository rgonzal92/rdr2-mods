# Cross-builds the plugin on Linux with clang and lld. No Windows SDK or C runtime is needed:
# the plugin calls nothing but ScriptHookRDR2.dll, whose import library comes from the .def file.

build/BetterPlayerHorses.asi: src/BetterPlayerHorses.cpp build/ScriptHookRDR2.lib
	clang++ --target=x86_64-pc-windows-msvc -std=c++17 -O2 -Wall -Wextra -shared -nostdlib -fuse-ld=lld -Wl,-entry:DllMain,-noimplib -o $@ $^

build/ScriptHookRDR2.lib: src/ScriptHookRDR2.def
	mkdir -p build
	llvm-dlltool -m i386:x86-64 -d $< -l $@
