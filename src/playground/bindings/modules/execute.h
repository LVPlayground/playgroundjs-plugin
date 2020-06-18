// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_EXECUTE_H_
#define PLAYGROUND_BINDINGS_MODULES_EXECUTE_H_

#include <string>
#include <vector>

#include <boost/asio/io_context.hpp>
#include <boost/function.hpp>

namespace bindings {

// Callback type that will be used for relaying the result of a process execution.
using ExecuteCallback = boost::function<void(int exit_code,
                                             const std::string& out,
                                             const std::string& error)>;

// Executes the given |command|, optionally with the given |parameters|. Will invoke the |callback|
// when execution has completed, with stdout in |out|, and stderr in |error|.
void Execute(boost::asio::io_context& io_context,
             const std::string& command,
             const std::vector<std::string>& arguments,
             ExecuteCallback callback);

}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_MODULES_EXECUTE_H_
