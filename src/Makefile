# Copyright 2015 Las Venturas Playground. All rights reserved.
# Use of this source code is governed by the MIT license, a copy of which can
# be found in the LICENSE file.

$(shell [ -d "out" ] || mkdir -p out)
$(shell [ -d "out/obj/" ] || mkdir -p out/obj)
$(shell [ -d "out/obj.test_runner/" ] || mkdir -p out/obj.test_runner)

BOOST_HEADERS=/opt/boost_1_73_0_32bit
BOOST_LIBS=/opt/boost_1_73_0_32bit/stage/lib

V8_HEADERS_A=v8
V8_HEADERS_B=v8/include
V8_LIBS=v8/out/x86

MYSQL_HEADERS=/usr/include/mysql

ICU_COMMON_HEADERS=v8/third_party/icu/source/common
ICU_I18N_HEADERS=v8/third_party/icu/source/i18n

CC_INCLUDE=-I$(BOOST_HEADERS) -I$(V8_HEADERS_A) -I$(V8_HEADERS_B) -I$(MYSQL_HEADERS) -I$(ICU_COMMON_HEADERS) -I$(ICU_I18N_HEADERS)

C=gcc
CC=g++
CFLAGSBASE=-c -m32 -fPIC -O3 -std=c++17 -w -DLINUX -DNDEBUG -I. -Iplayground $(CC_INCLUDE)
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
playground: playground_base playground_bindings playground_performance playground_plugin playground_third_party playground_test
	$(CC) $(CFLAGS) playground/playground_controller.cc -o out/obj/playground_playground_controller.o

# Target: /playground/*_test.cc
playground_test:
	$(CC) $(CFLAGS) playground/bindings/modules/streamer/streamer_test.cc -o out/obj/playground_bindings_modules_streamer_streamer_test.o
	$(CC) $(CFLAGS) playground/plugin/callback_parser_test.cc -o out/obj/playground_plugin_callback_parser_test.o
	$(CC) $(CFLAGS) playground/test_runner.cc -o out/obj/playground_test_runner.o

# Target: /playground/base/
playground_base:
	$(CC) $(CFLAGS) playground/base/encoding.cc -o out/obj/playground_base_encoding.o
	$(CC) $(CFLAGS) playground/base/file_path.cc -o out/obj/playground_base_file_path.o
	$(CC) $(CFLAGS) playground/base/file_search.cc -o out/obj/playground_base_file_search.o
	$(CC) $(CFLAGS) playground/base/logging.cc -o out/obj/playground_base_logging.o
	$(CC) $(CFLAGS) playground/base/memory.cc -o out/obj/playground_base_memory.o
	$(CC) $(CFLAGS) playground/base/string_piece.cc -o out/obj/playground_base_string_piece.o
	$(CC) $(CFLAGS) playground/base/time.cc -o out/obj/playground_base_time.o

# Target: /playground/bindings/modules/mysql/
playground_bindings_mysql:
	$(CC) $(CFLAGS) playground/bindings/modules/mysql_module.cc -o out/obj/playground_bindings_modules_mysql_module.o
	$(CC) $(CFLAGS) playground/bindings/modules/mysql/connection_client.cc -o out/obj/playground_bindings_modules_mysql_connection_client.o
	$(CC) $(CFLAGS) playground/bindings/modules/mysql/connection_host.cc -o out/obj/playground_bindings_modules_mysql_connection_host.o
	$(CC) $(CFLAGS) playground/bindings/modules/mysql/mutex.cc -o out/obj/playground_bindings_modules_mysql_mutex.o
	$(CC) $(CFLAGS) playground/bindings/modules/mysql/query_result.cc -o out/obj/playground_bindings_modules_mysql_query_result.o
	$(CC) $(CFLAGS) playground/bindings/modules/mysql/thread.cc -o out/obj/playground_bindings_modules_mysql_thread.o

# Target: /playground/bindings/modules/socket/
playground_bindings_socket:
	$(CC) $(CFLAGS) playground/bindings/modules/socket_module.cc -o out/obj/playground_bindings_modules_socket_module.o
	$(CC) $(CFLAGS) playground/bindings/modules/socket/socket.cc -o out/obj/playground_bindings_modules_socket_socket.o
	$(CC) $(CFLAGS) playground/bindings/modules/socket/socket_open_options.cc -o out/obj/playground_bindings_modules_socket_socket_open_options.o
	$(CC) $(CFLAGS) playground/bindings/modules/socket/socket_protocol.cc -o out/obj/playground_bindings_modules_socket_socket_protocol.o
	$(CC) $(CFLAGS) playground/bindings/modules/socket/socket_ssl_mode.cc -o out/obj/playground_bindings_modules_socket_socket_ssl_mode.o
	$(CC) $(CFLAGS) playground/bindings/modules/socket/tcp_socket.cc -o out/obj/playground_bindings_modules_socket_tcp_socket.o
	$(CC) $(CFLAGS) playground/bindings/modules/socket/web_socket.cc -o out/obj/playground_bindings_modules_socket_web_socket.o

# Target: /playground/bindings/modules/streamer/
playground_bindings_streamer:
	$(CC) $(CFLAGS) playground/bindings/modules/streamer_module.cc -o out/obj/playground_bindings_modules_streamer_module.o
	$(CC) $(CFLAGS) playground/bindings/modules/streamer/streamer.cc -o out/obj/playground_bindings_modules_streamer_streamer.o
	$(CC) $(CFLAGS) playground/bindings/modules/streamer/streamer_host.cc -o out/obj/playground_bindings_modules_streamer_streamer_host.o
	$(CC) $(CFLAGS) playground/bindings/modules/streamer/streamer_worker.cc -o out/obj/playground_bindings_modules_streamer_streamer_worker.o

# Target: /playground/bindings/
playground_bindings: playground_bindings_mysql playground_bindings_streamer playground_bindings_socket
	$(CC) $(CFLAGS) playground/bindings/console.cc -o out/obj/playground_bindings_console.o
	$(CC) $(CFLAGS) playground/bindings/event.cc -o out/obj/playground_bindings_event.o
	$(CC) $(CFLAGS) playground/bindings/exception_handler.cc -o out/obj/playground_bindings_exception_handler.o
	$(CC) $(CFLAGS) playground/bindings/global_callbacks.cc -o out/obj/playground_bindings_global_callbacks.o
	$(CC) $(CFLAGS) playground/bindings/global_scope.cc -o out/obj/playground_bindings_global_scope.o
	$(CC) $(CFLAGS) playground/bindings/modules/execute.cc -o out/obj/playground_bindings_modules_execute.o
	$(CC) $(CFLAGS) playground/bindings/pawn_invoke.cc -o out/obj/playground_bindings_pawn_invoke.o
	$(CC) $(CFLAGS) playground/bindings/profiler.cc -o out/obj/playground_bindings_profiler.o
	$(CC) $(CFLAGS) playground/bindings/provided_natives.cc -o out/obj/playground_bindings_provided_natives.o
	$(CC) $(CFLAGS) playground/bindings/promise.cc -o out/obj/playground_bindings_promise.o
	$(CC) $(CFLAGS) playground/bindings/runtime.cc -o out/obj/playground_bindings_runtime.o
	$(CC) $(CFLAGS) playground/bindings/runtime_modulator.cc -o out/obj/playground_bindings_runtime_modulator.o
	$(CC) $(CFLAGS) playground/bindings/runtime_operations.cc -o out/obj/playground_bindings_runtime_operations.o
	$(CC) $(CFLAGS) playground/bindings/timer_queue.cc -o out/obj/playground_bindings_timer_queue.o

# Target: /playground/performance/
playground_performance:
	$(CC) $(CFLAGS) playground/performance/scoped_trace.cc -o out/obj/playground_performance_scoped_trace.o
	$(CC) $(CFLAGS) playground/performance/trace_manager.cc -o out/obj/playground_performance_trace_manager.o

# Target: /playground/plugin/sdk/
playground_plugin_sdk:
	$(CC) $(CFLAGS) playground/plugin/sdk/amxplugin.cpp -o out/obj/playground_plugin_sdk_amxplugin.o

# Target: /playground/plugin/
playground_plugin: playground_plugin_sdk
	$(CC) $(CFLAGS) playground/plugin/arguments.cc -o out/obj/playground_plugin_arguments.o
	$(CC) $(CFLAGS) playground/plugin/callback_hook.cc -o out/obj/playground_plugin_callback_hook.o
	$(CC) $(CFLAGS) playground/plugin/callback_manager.cc -o out/obj/playground_plugin_callback_manager.o
	$(CC) $(CFLAGS) playground/plugin/callback_parser.cc -o out/obj/playground_plugin_callback_parser.o
	$(CC) $(CFLAGS) playground/plugin/fake_amx.cc -o out/obj/playground_plugin_fake_amx.o
	$(CC) $(CFLAGS) playground/plugin/native_function_manager.cc -o out/obj/playground_plugin_native_function_manager.o
	$(CC) $(CFLAGS) playground/plugin/native_parameters.cc -o out/obj/playground_plugin_native_parameters.o
	$(CC) $(CFLAGS) playground/plugin/native_parser.cc -o out/obj/playground_plugin_native_parser.o
	$(CC) $(CFLAGS) playground/plugin/pawn_helpers.cc -o out/obj/playground_plugin_pawn_helpers.o
	$(CC) $(CFLAGS) playground/plugin/plugin.cc -o out/obj/playground_plugin_plugin.o
	$(CC) $(CFLAGS) playground/plugin/plugin_controller.cc -o out/obj/playground_plugin_plugin_controller.o
	$(CC) $(CFLAGS) playground/plugin/scoped_reentrancy_lock.cc -o out/obj/playground_plugin_scoped_reentrancy_lock.o

# Target: /playground/third_party/subhook/
playground_third_party_subhook:
	$(C) $(FLAGS) playground/third_party/subhook/subhook.c -o out/obj/playground_third_party_subhook_subhook.o
	$(C) $(FLAGS) playground/third_party/subhook/subhook_linux.c -o out/obj/playground_third_party_subhook_subhook_linux.o
	$(C) $(FLAGS) playground/third_party/subhook/subhook_x86.c -o out/obj/playground_third_party_subhook_x86.o

# Target: /playground/third_party/
playground_third_party: playground_third_party_subhook

# Link playground.so
link:
	$(CC) -L${BOOST_LIBS} -O2 -m32 -fshort-wchar -Wl,--export-dynamic -shared -o out/$(OUTFILE) \
		out/obj/*.o -lboost_regex -lboost_system -lboost_filesystem -lboost_thread -lmysqlclient \
		out/libv8_libplatform.so out/libv8_libbase.so out/libv8.so out/libicui18n.so out/libicuuc.so

# Move libv8.so
move:
	cp v8/out.gn/x86.release/libv8.so out/libv8.so
	cp v8/out.gn/x86.release/libv8_libbase.so out/libv8_libbase.so
	cp v8/out.gn/x86.release/libv8_libplatform.so out/libv8_libplatform.so
	cp v8/out.gn/x86.release/libchrome_zlib.so out/libchrome_zlib.so
	cp v8/out.gn/x86.release/icudtl.dat out/icudtl.dat
	cp v8/out.gn/x86.release/libicui18n.so out/libicui18n.so
	cp v8/out.gn/x86.release/libicuuc.so out/libicuuc.so

# Target: test_runner
test_runner:
	$(CC) $(CFLAGSBASE) runner/logging.cc -o out/obj.test_runner/runner_logging.o
	$(CC) $(CFLAGSBASE) runner/main.cc -o out/obj.test_runner/runner_main.o
	cd out && $(CC) -O2 -m32 -fshort-wchar -o test_runner -L${BOOST_LIBS} \
		obj.test_runner/*.o \
		$(OUTFILE) -lboost_regex -lboost_system -lboost_filesystem -lboost_thread -lmysqlclient \
		libv8_libplatform.so libv8_libbase.so libv8.so libicui18n.so libicuuc.so

# Clean
clean:
	rm -rf out
