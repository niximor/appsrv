#pragma once

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>

#include <gcm/config/value.h>

#include "pidfile.h"

namespace gcm {
namespace thread {

class UserNotFound: public std::runtime_error {
public:
    UserNotFound(const std::string &msg): std::runtime_error(msg)
    {}
};

class GroupNotFound: public std::runtime_error {
public:
    GroupNotFound(const std::string &msg): std::runtime_error(msg)
    {}
};

uid_t getuid(const std::string &user_name) {
    ::passwd *pwd = getpwnam(user_name.c_str());
    if (pwd != nullptr) {
        return pwd->pw_uid;
    } else {
        throw UserNotFound("User " + user_name + " was not found in the system.");
    }
}

gid_t getgid(const std::string &group_name) {
    ::group *grp = getgrnam(group_name.c_str());
    if (grp != nullptr) {
        return grp->gr_gid;
    } else {
        throw GroupNotFound("Group " + group_name + " was not found in the system.");
    }
}

class Daemon {
public:
    Daemon(): uid(::getuid()), gid(::getgid()), has_uid(false), has_gid(false)
    {}

    /**
     * Automatically configure the daemon.
     */
    template<typename T>
    void start(gcm::config::Value &cfg, T routine) {
        auto &log = gcm::logging::getLogger("start-up");

        if (cfg.hasItem("pidfile")) {
            set_pidfile(cfg["pidfile"].asString());
        }

        try {
            if (cfg.hasItem("uid")) {
                auto &cfg_uid = cfg["uid"];
                if (cfg_uid.isString()) {
                    set_uid(cfg_uid.asString());
                } else {
                    set_uid(cfg_uid.asInt());
                }
            }
        } catch (UserNotFound &e) {
            WARNING(log) << e.what();
        }

        try {
            if (cfg.hasItem("gid")) {
                auto &cfg_gid = cfg["gid"];
                if (cfg_gid.isString()) {
                    set_gid(cfg_gid.asString());
                } else {
                    set_gid(cfg_gid.asInt());
                }
            }
        } catch (GroupNotFound &e) {
            WARNING(log) << e.what();
        }

        if (cfg.hasItem("daemon") && cfg["daemon"].asBool()) {
            INFO(log) << "Going to background.";
            try {
                start(routine);
            } catch (gcm::io::IOException &e) {
                INFO(log) << "Unable to start daemon: " << e.what();
            }
        } else {
            // No daemon, just set uid, gid and optionally PID file.
            if (has_uid) {
                ::setuid(uid);
            }

            if (has_gid) {
                ::setgid(gid);
            }

            if (!pid_file_name.empty()) {
                PidFile pid(pid_file_name);
                routine();
            } else {
                routine();
            }
        }
    }

    template<typename T>
    void start(T routine) {
        // see man 7 daemon for steps necessary.

        // Steps 1 through 4 are not managable by this class, it is the responsibility of the
        // caller to ensure that the environment is sane.

        // Create pipe for notifying original process of completion of daemon startup.
        if (pipe(fd) != 0) {
            throw gcm::io::IOException(errno);
        }

        // 5. Call fork() to create a background process.
        pid_t pid = ::fork();
        if (pid < 0) {
            throw gcm::io::IOException(errno);
        } else if (pid > 0) {
            exec_parent();
        } else if (pid == 0) {
            try {
                exec_child(routine);
            } catch (...) {
                // Unblock parent in case of exception...
                const char *buf = "1";
                write(fd[1], buf, 1);
                close(fd[1]);

                throw;
            }
        }        
    }

    void set_uid(uid_t uid) {
        this->uid = uid;
        has_uid = true;
    }

    void set_uid(const std::string &user_name) {
        this->uid = getuid(user_name);
    }

    void set_gid(gid_t gid) {
        this->gid = gid;
        has_gid = true;
    }

    void set_gid(const std::string &group_name) {
        this->gid = getgid(group_name);
    }

    void set_pidfile(const std::string &file_name) {
        pid_file_name = file_name;
    }

    uid_t get_uid() {
        return uid;
    }

    gid_t get_gid() {
        return gid;
    }

protected:
    uid_t uid;
    gid_t gid;

    bool has_uid;
    bool has_gid;

    int fd[2];

    std::string pid_file_name;

private:
    void exec_parent() {
        // Wait for daemon to initialize.
        char buf[1];
        read(fd[0], buf, 1);

        // 15. Call exit() in the original process.
        ::exit(EXIT_SUCCESS);
    }

    template<typename T>
    void exec_child(T routine) {
        // 6. In the child, call setsid() to detach from any terminal and create independent session.
        if (setsid() < 0) {
            throw gcm::io::IOException(errno);
        }

        // 7. In the child, call fork again, to ensure that the daemon can never re-acquire a terminal again.
        pid_t pid = ::fork();
        if (pid < 0) {
            throw gcm::io::IOException(errno);
        }

        // 8. Call exit() in the first child, so that only the second child (the actual daemon process) stays around.
        // This ensures that the daemon process is re-parented to init/PID 1, as all daemons should be.
        if (pid > 0) {
            ::exit(EXIT_SUCCESS);
        }

        // 10. In the daemon process, reset umask to 0, so that the file modes passed to open(), mkdir() and suchlike
        // directly control the access mode of the created files and directories.
        umask(0);

        // 11. In the daemon process, change the current directory to the root directory (/), in order to avoid
        // that the daemon involuntarily blocks mount points from being unmounted.
        chdir("/");

        // 12. In the daemon process, write the daemon PID (as returned by getpid()) to a PID file.
        if (!pid_file_name.empty()) {
            PidFile pid(pid_file_name);
            exec_daemon(routine);
        } else {
            exec_daemon(routine);
        }
    }

    template<typename T>
    void exec_daemon(T &routine) {
        // 13. In the daemon process, drop privileges, if possible and applicable.
        if (has_uid) {
            setuid(uid);
        }

        if (has_gid) {
            setgid(gid);
        }

        // 14. From the daemon process, notify the original process starged that initialization is complete.
        const char *buf = "1";
        write(fd[1], buf, 1);
        close(fd[1]);

        // 9. In the daemon process, connect /dev/null to standard input, output and error.
        {
            int fd = open("/dev/null", O_RDONLY);
            dup2(fd, STDIN_FILENO);
        }

        {
            int fd = open("/dev/null", O_RDWR);
            dup2(fd, STDOUT_FILENO);
        }

        {
            int fd = open("/dev/null", O_RDWR);
            dup2(fd, STDERR_FILENO);
        }

        // Call the main routine itself.
        routine();

        ::exit(EXIT_SUCCESS);
    }
};

} // namespace thread
} // namespace gcm
