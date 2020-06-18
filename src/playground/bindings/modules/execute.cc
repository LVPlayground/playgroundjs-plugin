// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/modules/execute.h"

#include <boost/process.hpp>
#include <boost/process/async.hpp>
#include <boost/process/extend.hpp>
#include <future>

#include "base/logging.h"

namespace bindings {

struct ExecutionData {
  ExecuteCallback callback;
  std::future<std::string> stdout_data;
  std::future<std::string> stderr_data;
};

struct ExecutionFinishedHandler : public boost::process::extend::async_handler {
  explicit ExecutionFinishedHandler(std::shared_ptr<ExecutionData> data)
      : data_(data) {}

  ExecutionFinishedHandler(ExecutionFinishedHandler&) = default;
  ~ExecutionFinishedHandler() = default;

  template <typename T>
  std::function<void(int, std::error_code const&)> on_exit_handler(T&) {
    std::shared_ptr<ExecutionData> data = data_;

    return [data](int exit_code, std::error_code const& ec) {
      data->callback(exit_code, data->stdout_data.get(), data->stderr_data.get());
    };
  }

  std::shared_ptr<ExecutionData> data_;
};

void Execute(boost::asio::io_context& io_context,
             const std::string& command,
             const std::vector<std::string>& arguments,
             ExecuteCallback callback) {
  std::shared_ptr<ExecutionData> data = std::make_shared<ExecutionData>();
  data->callback = callback;
 
  try {
    boost::process::child process(
        boost::process::search_path(command), boost::process::args(arguments),
        boost::process::std_out > data->stdout_data,
        boost::process::std_err > data->stderr_data,
        ExecutionFinishedHandler { data },
        io_context);
  } catch (const std::exception& exception) {
    callback(-1, "", exception.what());
  }
}

}  // namespace bindings
