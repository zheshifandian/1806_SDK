/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BEEROCKS_OS_UTILS_H_
#define _BEEROCKS_OS_UTILS_H_

#ifdef __ANDROID__
////
#ifndef IS_ANDROID
#define IS_ANDROID
#endif
////
#else
////
#if __linux
#elif __unix
#elif __posix
#else
////
#ifndef IS_WINDOWS
#define IS_WINDOWS
#endif
////
#endif
////
#ifndef IS_WINDOWS
#ifndef IS_LINUX
#define IS_LINUX
#endif
#endif
////
#endif

#ifdef IS_WINDOWS
#ifndef snprintf
#define snprintf _snprintf
#endif
#define UTILS_SLEEP_MSEC(msec) Sleep(msec)
#else // Linux
#include <unistd.h>
#define UTILS_SLEEP_MSEC(msec) usleep(msec * 1000)
#endif

#include <string>

namespace beerocks {
#define SYSTEM_CALL os_utils::system_call

class os_utils {
public:
    ///
    /// @brief Function to get current process executable path
    ///
    /// @return string containing executable binary path location.
    ///     if information can't be acquired.
    ///
    static std::string get_process_path();

    ///
    /// @brief Function to get current process directory
    ///
    /// @return string containing the directory where the executable binary is located.
    ///
    static std::string get_process_dir();

    ///
    /// @brief Function that checks if file exists
    ///
    /// @return boolean that is true if the file exists and false otherwise.
    ///
    static bool file_exists(const std::string &fname);

    /**
     * @brief Operates system call.
     *
     * Detached flag operate command without blocking.
     *
     * @param [in] cmd command string.
     * @param [in] detached flag to request to run command in detached mode.
     */
    static void system_call(const std::string &cmd, bool detached = false);

    /**
     * @brief Operates system call and return its output.
     *
     * If enable_stderr is enabled, requested call stdout and stderr are returned.
     * Output of command is limited to size of buffer defined in the method (10kb).
     *
     * @param [in] cmd command string.
     * @param [in] enable_stderr request get output from stderr with stdout.
     * @return Returns system call output string.
     */
    static std::string system_call_with_output(const std::string &cmd, bool enable_stderr = false);

    static void kill_pid(const std::string &path, const std::string &file_name);

    static bool is_pid_running(const std::string &path, std::string file_name,
                               int *pid_out = nullptr);

    /**
     * @brief Reads a PID from the file in the provided path.
     *
     * @param path Path to where the file is located.
     * @param file_name Name of the pid file for the process in question.
     * @param[out] pid Process id read from the file
     * @return true on success, false if file doesn't exist or failed to read it
     */
    static bool read_pid_file(const std::string &path, const std::string &file_name, int &pid);

    static bool write_pid_file(const std::string &path, const std::string &file_name);

    static bool touch_pid_file(std::string file_path);

    static int redirect_console_std(std::string log_file_name);

    static void close_file(int fd);

    /**
     * @brief Removes the residue files from previous process instance.
     *
     * @param path Path to where the residual file are located.
     * @param file_name Name of the file to be removed if exist.
     */
    static void remove_residual_files(const std::string &path, const std::string &file_name);

    /**
     * @brief Get the operating system name and version.
     * 
     * @details Most of Linux distribution have the file "/etc/os-release" which contain the name
     * of the operating system and the version. This function extract it from the file and return
     * it.
     * 
     * @return std::string containing operating system name and version. 
     */
    static std::string get_os_name();
};
} // namespace beerocks

#endif //_BEEROCKS_OS_UTILS_H_
