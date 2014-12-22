#pragma once

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string>
#include <stdexcept>
#include <cstring>

#include <unistd.h>
#include <errno.h>

#include <gcm/io/util.h>
#include <gcm/io/exception.h>

namespace gcm {
namespace thread {

class PidFileExists: public std::runtime_error {
public:
    PidFileExists(const std::string &file_name): std::runtime_error("Unable to write PID file " + file_name + ": " + strerror(errno))
    {}
};

class PidFile {
public:
    /**
     * Default constructor. Write current pid to pid file.
     */
    PidFile(const std::string &file_name):
        file_name(file_name),
        fd(0)
    {
        auto &log = gcm::logging::getLogger("start-up");

        fd = ::open(file_name.c_str(), O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC, 0644);
        if (fd > 0) {
            char buf[32];
            snprintf(buf, 32, "%d\n", ::getpid());

            write(fd, buf, strlen(buf));
        } else {
            throw PidFileExists(file_name);
        }
    }

    /**
     * Empty constructor to avoid no pid file guard to be constructed.
     */
    PidFile()
    {}

    /**
     * Move constructor, to allow delegate PidFile guard to another
     * method.
     */
    PidFile(PidFile &&other): file_name(other.file_name)
    {
        other.file_name = "";
    }

    /**
     * Destroy PID file at exit.
     */
    ~PidFile()
    {
        if (fd > 0) {
            close(fd);
        }
    }

protected:
    std::string file_name;
    int fd;
};

}
}