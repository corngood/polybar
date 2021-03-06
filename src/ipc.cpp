#include <fcntl.h>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <vector>

#include "common.hpp"
#include "utils/file.hpp"

using namespace polybar;

void log(const string& msg) {
  std::cerr << "polybar-msg: " << msg << std::endl;
}

void log(int exit_code, const string& msg) {
  std::cerr << "polybar-msg: " << msg << std::endl;
  exit(exit_code);
}

void usage(const string& parameters) {
  std::cerr << "Usage: polybar-msg [-p pid] " << parameters << std::endl;
  exit(127);
}

bool validate_type(const string& type) {
  if (type == "action") {
    return true;
  } else if (type == "cmd") {
    return true;
  } else if (type == "hook") {
    return true;
  } else {
    return false;
  }
}

int main(int argc, char** argv) {
  const int E_GENERIC{1};
  const int E_NO_CHANNELS{2};
  const int E_MESSAGE_TYPE{3};
  const int E_INVALID_PID{4};
  const int E_INVALID_CHANNEL{5};
  const int E_WRITE{6};

  vector<string> args{argv + 1, argv + argc};
  string::size_type p;
  int pid{0};

  // If -p <pid> is passed, check if the process is running and that
  // a valid channel pipe is available
  if (args.size() >= 2 && args[0].compare(0, 2, "-p") == 0) {
    if (!file_util::exists("/proc/" + args[1])) {
      log(E_INVALID_PID, "No process with pid " + args[1]);
    } else if (!file_util::is_fifo("/tmp/polybar_mqueue." + args[1])) {
      log(E_INVALID_CHANNEL, "No channel available for pid " + args[1]);
    }

    pid = std::atoi(args[1].c_str());
    args.erase(args.begin());
    args.erase(args.begin());
  }

  // Validate args
  if (args.size() < 2) {
    usage("<command=(action|cmd|hook)> <payload> [...]");
  } else if (!validate_type(args[0])) {
    log(E_MESSAGE_TYPE, "\"" + args[0] + "\" is not a valid type.");
  }

  string ipc_type{args[0]};
  args.erase(args.begin());
  string ipc_payload{args[0]};
  args.erase(args.begin());

  // Check hook specific args
  if (ipc_type == "hook") {
    if (args.size() != 1) {
      usage("hook <module-name> <hook-index>");
    } else if ((p = ipc_payload.find("module/")) != 0) {
      ipc_payload = "module/" + ipc_payload + args[0];
      args.erase(args.begin());
    } else {
      ipc_payload += args[0];
      args.erase(args.begin());
    }
  }

  // Get availble channel pipes
  auto channels = file_util::glob("/tmp/polybar_mqueue.*");
  if (channels.empty()) {
    log(E_NO_CHANNELS, "There are no active ipc channels");
  }

  // Write the message to each channel in the list and remove stale
  // channel pipes that may be left lingering if the owning process got
  // SIGKILLED or crashed
  for (auto&& channel : channels) {
    string handle{channel};
    int handle_pid{0};

    if ((p = handle.rfind('.')) != string::npos) {
      handle_pid = std::atoi(handle.substr(p + 1).c_str());
    }

    if (!file_util::exists("/proc/" + to_string(handle_pid))) {
      if (unlink(handle.c_str()) == -1) {
        log(E_GENERIC, "Could not remove stale ipc channel: " + string{std::strerror(errno)});
      } else {
        log("Removed stale ipc channel: " + handle);
      }
    } else if (!pid || pid == handle_pid) {
      string payload{ipc_type + ":" + ipc_payload};

      try {
        fd_stream<std::ostream> out(handle, O_WRONLY);
        out << payload << '\n';
        log("Successfully wrote \"" + payload + "\" to \"" + handle + "\"");
      } catch (const std::exception& err) {
        log(E_WRITE, "Failed to write \"" + payload + "\" to \"" + handle + "\" (err: " + err.what() + ")");
      }
    }
  }

  return 0;
}
