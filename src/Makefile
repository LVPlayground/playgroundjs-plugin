# Copyright 2015 Las Venturas Playground. All rights reserved.
# Use of this source code is governed by the MIT license, a copy of which can
# be found in the LICENSE file.

$(shell [ -d "out" ] || mkdir -p out)
$(shell [ -d "out/obj/" ] || mkdir -p out/obj)
$(shell [ -d "out/obj.test_runner/" ] || mkdir -p out/obj.test_runner)

V8LIBS=v8/out/ia32.release/obj.target/tools/gyp

C=clang
CC=clang++
CFLAGSBASE=-c -m32 -fPIC -O3 -std=c++11 -w -DLINUX -DNDEBUG -I. -Iplayground -Iv8 -I/usr/include/mysql
CFLAGS=$(CFLAGSBASE) -DPLAYGROUND_IMPLEMENTATION
FLAGS=-c -m32 -fPIC -O3 -w -DLINUX -I. -Iplayground -Iv8

OUTFILE=playgroundjs-plugin.so

# Target: /
all: tp_gtest playground move link test_runner

# Target: /gtest/
tp_gtest:
	$(CC) $(CFLAGS) gtest/gtest.cc -o out/obj/gtest_gtest.o
	$(CC) $(CFLAGS) gtest/gtest-death-test.cc -o out/obj/gtest_gtest_death_test.o
	$(CC) $(CFLAGS) gtest/gtest-filepath.cc -o out/obj/gtest_gtest_filepath.o
	$(CC) $(CFLAGS) gtest/gtest-port.cc -o out/obj/gtest_gtest_port.o
	$(CC) $(CFLAGS) gtest/gtest-printers.cc -o out/obj/gtest_gtest_printers.o
	$(CC) $(CFLAGS) gtest/gtest-test-part.cc -o out/obj/gtest_gtest_test_part.o
	$(CC) $(CFLAGS) gtest/gtest-typed-test.cc -o out/obj/gtest_gtest_typed_test.o

# Target: /playground/
playground: playground_base playground_bindings playground_plugin playground_third_party playground_test
	$(CC) $(CFLAGS) playground/playground_controller.cc -o out/obj/playground_playground_controller.o
	$(CC) $(CFLAGS) playground/test_runner.cc -o out/obj/playground_test_runner.o

# Target: /playground/*_test.cc
playground_test:
	$(CC) $(CFLAGS) playground/plugin/callback_parser_test.cc -o out/obj/playground_plugin_callback_parser_test.o

# Target: /playground/base/
playground_base:
	$(CC) $(CFLAGS) playground/base/file_path.cc -o out/obj/playground_base_file_path.o
	$(CC) $(CFLAGS) playground/base/logging.cc -o out/obj/playground_base_logging.o
	$(CC) $(CFLAGS) playground/base/string_piece.cc -o out/obj/playground_base_string_piece.o
	$(CC) $(CFLAGS) playground/base/time.cc -o out/obj/playground_base_time.o

# Target: /playground/bindings/modules/mysql/
playground_bindings_mysql:
	$(CC) $(CFLAGS) playground/bindings/modules/mysql_module.cc -o out/obj/playground_bindings_modules_mysql_module.o
	$(CC) $(CFLAGS) playground/bindings/modules/mysql/connection_client.cc -o out/obj/playground_bindings_modules_mysql_connection_client.o
	$(CC) $(CFLAGS) playground/bindings/modules/mysql/connection_host.cc -o out/obj/playground_bindings_modules_mysql_connection_host.o
	$(CC) $(CFLAGS) playground/bindings/modules/mysql/mutex.cc -o out/obj/playground_bindings_modules_mysql_mutex.o
	$(CC) $(CFLAGS) playground/bindings/modules/mysql/thread.cc -o out/obj/playground_bindings_modules_mysql_thread.o

# Target: /playground/bindings/
playground_bindings: playground_bindings_mysql
	$(CC) $(CFLAGS) playground/bindings/console.cc -o out/obj/playground_bindings_console.o
	$(CC) $(CFLAGS) playground/bindings/event.cc -o out/obj/playground_bindings_event.o
	$(CC) $(CFLAGS) playground/bindings/exception_handler.cc -o out/obj/playground_bindings_exception_handler.o
	$(CC) $(CFLAGS) playground/bindings/global_callbacks.cc -o out/obj/playground_bindings_global_callbacks.o
	$(CC) $(CFLAGS) playground/bindings/global_scope.cc -o out/obj/playground_bindings_global_scope.o
	$(CC) $(CFLAGS) playground/bindings/pawn_invoke.cc -o out/obj/playground_bindings_pawn_invoke.o
	$(CC) $(CFLAGS) playground/bindings/promise.cc -o out/obj/playground_bindings_promise.o
	$(CC) $(CFLAGS) playground/bindings/runtime.cc -o out/obj/playground_bindings_runtime.o
	$(CC) $(CFLAGS) playground/bindings/runtime_operations.cc -o out/obj/playground_bindings_runtime_operations.o
	$(CC) $(CFLAGS) playground/bindings/script_prologue.cc -o out/obj/playground_bindings_script_prologue.o

# Target: /playground/plugin/sdk/
playground_plugin_sdk:
	$(CC) $(CFLAGS) playground/plugin/sdk/amxplugin.cpp -o out/obj/playground_plugin_sdk_amxplugin.o

# Target: /playground/plugin/
playground_plugin: playground_plugin_sdk
	$(CC) $(CFLAGS) playground/plugin/arguments.cc -o out/obj/playground_plugin_arguments.o
	$(CC) $(CFLAGS) playground/plugin/callback_hook.cc -o out/obj/playground_plugin_callback_hook.o
	$(CC) $(CFLAGS) playground/plugin/callback_parser.cc -o out/obj/playground_plugin_callback_parser.o
	$(CC) $(CFLAGS) playground/plugin/fake_amx.cc -o out/obj/playground_plugin_fake_amx.o
	$(CC) $(CFLAGS) playground/plugin/native_function_manager.cc -o out/obj/playground_plugin_native_function_manager.o
	$(CC) $(CFLAGS) playground/plugin/pawn_helpers.cc -o out/obj/playground_plugin_pawn_helpers.o
	$(CC) $(CFLAGS) playground/plugin/plugin.cc -o out/obj/playground_plugin_plugin.o
	$(CC) $(CFLAGS) playground/plugin/plugin_controller.cc -o out/obj/playground_plugin_plugin_controller.o

# Target: /playground/third_party/subhook/
playground_third_party_subhook:
	$(C) $(FLAGS) playground/third_party/subhook/subhook.c -o out/obj/playground_third_party_subhook_subhook.o
	$(C) $(FLAGS) playground/third_party/subhook/subhook_linux.c -o out/obj/playground_third_party_subhook_subhook_linux.o
	$(C) $(FLAGS) playground/third_party/subhook/subhook_x86.c -o out/obj/playground_third_party_subhook_x86.o

# Target: /playground/third_party/
playground_third_party: playground_third_party_subhook

# Link playground.so
link:
	$(CC) -O2 -m32 -fshort-wchar -shared -o out/$(OUTFILE) \
		out/obj/*.o \
		$(V8LIBS)/libv8_libplatform.a \
		$(V8LIBS)/libv8_libbase.a \
		$(V8LIBS)/libv8_nosnapshot.a \
		out/libv8.so -lmysqlclient

# Move libv8.so
move:
	cp v8/out/ia32.release/lib.target/libv8.so out/libv8.so
	strip out/libv8.so

# Target: test_runner
test_runner:
	$(CC) $(CFLAGSBASE) runner/logging.cc -o out/obj.test_runner/runner_logging.o
	$(CC) $(CFLAGSBASE) runner/main.cc -o out/obj.test_runner/runner_main.o
	cd out && $(CC) -O2 -m32 -fshort-wchar -o test_runner \
		obj.test_runner/*.o \
		$(OUTFILE) libv8.so

# Clean
clean:
	rm -rf out
