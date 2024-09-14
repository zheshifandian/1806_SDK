/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <bcl/beerocks_os_utils.h>
#include <bcl/beerocks_string_utils.h>

#include <chrono>
#include <fcntl.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>

#include <easylogging++.h>

using namespace beerocks;

std::string os_utils::get_process_path()
{
    char exe_path[PATH_MAX] = {0};
    if (-1 == readlink("/proc/self/exe", exe_path, sizeof(exe_path))) {
        LOG(ERROR) << "unable to determine execution path";
    }
    return std::string(exe_path);
}

std::string os_utils::get_process_dir()
{
    auto exe_path = get_process_path();
    auto dir_end  = exe_path.find_last_of("/");
    return exe_path.substr(0, dir_end);
}

bool os_utils::file_exists(const std::string &fname)
{
    struct stat st;
    return (stat(fname.c_str(), &st) == 0);
}

void os_utils::system_call(const std::string &cmd, bool detached)
{
    if (cmd.empty()) {
        LOG(ERROR) << "Empty call.";
        return;
    }

    auto command{cmd};
    command += (detached) ? " 2>&1 &" : " 2>&1";

    LOG(INFO) << "System call cmd: " << command;

    auto ret = system(command.c_str());
    if (ret != 0) {
        LOG(ERROR) << command << " failed with return code " << ret;
    }
}

std::string os_utils::system_call_with_output(const std::string &cmd, bool enable_stderr)
{
    std::string ret_str;

    if (cmd.empty()) {
        LOG(ERROR) << "Empty call.";
        return std::string();
    }

    auto command{cmd};
    command += (enable_stderr) ? " 2>&1" : " 2>/dev/null";

    LOG(INFO) << "System call cmd: " << command;

    // Maximum output string size.
    std::array<char, 10000> buffer;
    std::unique_ptr<FILE, decltype(&pclose)> command_pipe(popen(command.c_str(), "r"), pclose);

    if (!command_pipe) {
        LOG(ERROR) << "Failed to create pipe for " << command;
        return std::string();
    }

    auto read_bytes = fread(buffer.data(), 1, buffer.size(), command_pipe.get());
    ret_str.append(buffer.data(), read_bytes);

    return ret_str;
}

void os_utils::kill_pid(const std::string &path, const std::string &file_name)
{
    int pid_out;
    if (is_pid_running(path, file_name, &pid_out)) {
        LOG(DEBUG) << __FUNCTION__ << " SIGTERM pid=" << pid_out << std::endl;
        kill(pid_out, SIGTERM);
        auto timeout = std::chrono::steady_clock::now() + std::chrono::seconds(15);
        // wait until the process is down or timeout expires
        while (getpgid(pid_out) >= 0 && std::chrono::steady_clock::now() < timeout)
            ;

        if (getpgid(pid_out) >= 0) {
            LOG(DEBUG) << __FUNCTION__ << " SIGKILL pid=" << pid_out << std::endl;
            kill(pid_out, SIGKILL);
        }
    }
}

bool os_utils::is_pid_running(const std::string &path, std::string file_name, int *pid_out)
{
    std::string pid_file_name = path + file_name;
    std::string pid_str;
    std::string cmdline;

    // get pid from file
    std::ifstream pid_file(pid_file_name);
    if (pid_file.is_open()) {
        std::getline(pid_file, pid_str);
        pid_file.close();
        int pid = beerocks::string_utils::stoi(pid_str);

        //check pid program name
        {
            std::string proc_file_path = "/proc/" + pid_str + "/cmdline";
            std::ifstream proc_file(proc_file_path);
            if (proc_file.is_open()) {
                char buffer[1024] = {0};
                proc_file.read(buffer, sizeof(buffer));
                buffer[sizeof(buffer) - 1] = 0; // putting null terminator
                cmdline.assign(buffer);
                proc_file.close();
            }
        }

        if (cmdline.length() > 0) {
            auto p1 = cmdline.rfind("/");
            if (p1 != std::string::npos) {
                cmdline = cmdline.substr(p1 + 1);
            }

            // check if pid running
            if ((file_name.find(cmdline) != std::string::npos) && (getpgid(pid) >= 0)) {
                if (pid_out) {
                    *pid_out = pid;
                }
                return true; //pid is running
            }
        }
    }
    return false; //pid is not running
}

bool os_utils::read_pid_file(const std::string &path, const std::string &file_name, int &pid)
{
    std::string pid_str;
    std::string pid_file_name = path + "pid/" + file_name;
    std::ifstream pid_file;
    pid_file.open(pid_file_name.c_str(), std::fstream::out);
    if (!pid_file.is_open()) {
        LOG(ERROR) << "Failed to read pid from file: " << pid_file_name;
        return false;
    }
    std::getline(pid_file, pid_str);
    pid_file.close();
    pid = beerocks::string_utils::stoi(pid_str);
    return true;
}

bool os_utils::write_pid_file(const std::string &path, const std::string &file_name)
{
    std::string pid_file_path = path + "pid";
    mkdir(pid_file_path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    std::string pid_file_name = pid_file_path + "/" + file_name;
    int pid                   = getpid();
    std::ofstream pid_file;
    pid_file.open(pid_file_name.c_str(), std::fstream::out);
    if (pid_file.is_open()) {
        pid_file << pid << std::endl;
        pid_file.close();
        return true;
    } else {
        LOG(ERROR) << "can't save pid to file: " << pid_file_name;
        return false;
    }
}

bool os_utils::touch_pid_file(std::string file_path)
{
    if (utimensat(0, file_path.c_str(), nullptr, 0)) {
        LOG(ERROR) << "couldn't update file timestamp: " << file_path;
        return false;
    }

    return true;
}

int os_utils::redirect_console_std(std::string log_file_name)
{
    int fd_log_file_std = open(log_file_name.c_str(), O_CREAT | O_APPEND | O_RDWR, 0644);
    if (fd_log_file_std > 0) {
        std::ostringstream msg;
        msg << std::endl << "Start Log" << std::endl << std::endl;
        if (write(fd_log_file_std, msg.str().c_str(), msg.str().size()) < 0) {
            LOG(ERROR) << "Failed writing to file";
        }
        // dup2() - If the newfd was previously open,
        // it is silently closed before being reused
        dup2(fd_log_file_std, STDOUT_FILENO);
        dup2(fd_log_file_std, STDERR_FILENO);
    }
    close_file(fd_log_file_std);
    return fd_log_file_std;
}

void os_utils::close_file(int fd)
{
    if (fd) {
        close(fd);
    }
}

void os_utils::remove_residual_files(const std::string &path, const std::string &file_name)
{
    std::string file = path + file_name;
    if (file_exists(file)) {
        LOG(DEBUG) << "removing residual file: " << file;
        if (remove(file.c_str()) != 0) {
            LOG(ERROR) << "failed to remove residual file: " << file;
        }
    }
}

std::string os_utils::get_os_name()
{
    constexpr char os_info_file[] = "/etc/os-release";
    std::ifstream file(os_info_file);
    if (!file.is_open()) {
        return {};
    }

    std::string line;
    while (std::getline(file, line)) {
        constexpr char os_release_field[] = "PRETTY_NAME=\"";
        auto pos = line.find(os_release_field, 0, sizeof(os_release_field) - 1);
        if (pos == std::string::npos) {
            continue;
        }

        return line.substr(sizeof(os_release_field) - 1, line.size() - sizeof(os_release_field));
    }

    return {};
}
