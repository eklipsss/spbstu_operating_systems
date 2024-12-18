#include "conn_fifo.h"

#include <iostream>
#include <fcntl.h> 
#include <sys/stat.h>
#include <unistd.h>
#include <cstring> 

ConnFifo::ConnFifo(const std::string& path, bool create) : path(path), fd(-1), valid(false) {
    if (create) {
        if (mkfifo(path.c_str(), 0777) == -1) {
            if (errno != EEXIST) {
                std::cerr << "Ошибка создания FIFO: " << strerror(errno) << "\n";
                return;
            }
        }
    }

    fd = open(path.c_str(), O_RDWR | O_NONBLOCK);
    if (fd == -1) {
        std::cerr << "Ошибка создания FIFO: " << strerror(errno) << "\n";
    } else {
        valid = true;
    }
};

ConnFifo::~ConnFifo() {
    close();
    if (valid) {
        if (unlink(path.c_str()) == -1) {
            std::cerr << "Ошибка удаления FIFO: " << strerror(errno) << "\n";
        }
    }
};

bool ConnFifo::write(const std::string& msg) {
    if (!is_valid()) {
        std::cerr << "FIFO недоступен для записи\n";
        return false;
    }

    ssize_t bytes_written = ::write(fd, msg.c_str(), msg.size());
    if (bytes_written == -1) {
        std::cerr << "Ошибка записи в FIFO: " << strerror(errno) << "\n";
        return false;
    }
    return true;
}

bool ConnFifo::read(std::string& msg, size_t max_size) {
    if (!is_valid()) {
        std::cerr << "FIFO недоступен для чтения\n";
        return false;
    }

    char buffer[max_size];
    memset(buffer, '\0', max_size);

    ssize_t bytes_read = ::read(fd, buffer, max_size - 1);
    if (bytes_read == -1) {
        return false;
    }

    msg.assign(buffer, bytes_read);
    return true;
};

bool ConnFifo::is_valid() const {
    return valid;
};

void ConnFifo::close() {
    if (is_valid() && fd != -1) {
        if (::close(fd) == -1) {
            std::cerr << "Ошибка закрытия FIFO: " << strerror(errno) << "\n";
        }
        fd = -1;
    }
};
