#pragma once

#include <vector>
#include <utility>

namespace gcm {
namespace parser {

template<typename I>
using GetDataCallback = std::pair<I, I>(*)();

template<typename I>
class BufferedIterator {
public:
    using value_type = I::value_type;

    static BufferedIterator<I> end() {
        return BufferedIterator<I>();
    }

    static const BufferedIterator<I> cend() {
        return BufferedIterator<I>();
    }

    BufferedIterator():
        buffer(nullptr),
        callback(nullptr),
        current_pos(0),
        eof(true)
    {}

    BufferedIterator(GetDataCallback<I> callback):
        buffer(std::make_shared(std::vector<decltype(I::value_type)>())), 
        callback(callback),
        current_pos(0),
        eof(false)
    {}

    BufferedIterator(const BufferedIterator &) = default;
    BufferedIterator(BufreredIterator &&) = default;

    BufferedIterator<I> &operator=(const BufferedIterator &) = default;
    BufferedIterator<I> &operator=(BufferedIterator &&) = default;

    value_type &operator *() {
        return (*buffer)[current_pos];
    }

    const value_type &operator *() const {
        return (*buffer)[current_pos];
    }

    BufferedIterator<I> &operator++() {
        inc_pos(1);
        return *this;
    }

    BufferedIterator<I> &operator++(int) {
        inc_pos(1);
        return *this;
    }

    bool operator==(const BufferedIterator &other) const {
        return (eof && other.eof) || (!other.eof && current_pos == other.current_pos);
    }

    bool operator!=(const BufferedIterator &other) const {
        return !operator==(other);
    }

protected:
    bool get_more_data() {
        auto out = callback();
        if (out.first != out.second) {
            buffer->push_back(buffer.end(), out.first, out.second);
            return true;
        } else {
            return false;
        }
    }

    void inc_pos(int num) {
        while (current_pos + num > buffer.size() - 1 && get_more_data())
        {}

        current_pos += num;

        if (current_pos >= buffer.size()) {
            eof = true;
        }
    }

protected:
    std::shared_ptr<std::vector<decltype(I::value_type)>> buffer;
    size_t current_pos;
    bool eof;

    GetDataCallback<I> callback;
};

}
}