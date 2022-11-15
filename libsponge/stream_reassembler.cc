#include "stream_reassembler.hh"

#include <cassert>

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _buffer()
    , _first_unreasemble_idx(0)
    , _unreachable_idx(-1)
    , _eof_idx(-1)
    , _eof(false)
    , _n_unreasemble(0)
    , _output(capacity)
    , _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // caveat: We should treat data.length() == 0 as a byte nomally.
    _pop_buffer();
    const size_t data_len = data.size();
    const uint64_t data_end = index + data_len;
    _unreachable_idx = _first_unreasemble_idx + _capacity - _output.buffer_size();
    std::string input = std::string(std::move(data));
    if (data_end > _unreachable_idx) {
        input = input.substr(0, _unreachable_idx - index);
    }
    if (eof) {
        _eof = true;
        _eof_idx = index + data_len;
    }

    _inbound(input, index);
    _pop_buffer();

    if (_first_unreasemble_idx >= _eof_idx) {
        _output.end_input();
    }
}

void StreamReassembler::_inbound(const std::string &data, const uint64_t index) {
    uint64_t real_idx = index;
    size_t nwrite = 0;
    std::string input = data;

    if (real_idx <= _first_unreasemble_idx) {
        if (index + data.size() <= _first_unreasemble_idx) {
            return;
        }
        real_idx = _first_unreasemble_idx;
        input = std::string(std::move(input.substr(real_idx - index)));
        auto it = _buffer.lower_bound(real_idx);
        if (it != _buffer.end() && it->first < real_idx + input.size()) {
            const uint64_t split_pot = it->first - real_idx;
            std::string tmp = input;
            input = tmp.substr(0, split_pot);
            _push_buffer(tmp.substr(split_pot), it->first);
        }
        nwrite = _output.write(input);
        _first_unreasemble_idx += nwrite;
        if (nwrite == input.length()) {
            return;
        }
        real_idx = nwrite + real_idx;
    }
    _push_buffer(input, real_idx);
}

void StreamReassembler::_push_buffer(const std::string &data, const uint64_t index) {
    auto it = _buffer.lower_bound(index);
    std::string input = data;
    uint64_t start_idx = index;
    uint64_t data_end = index + data.size();
    if (it != _buffer.end() && it->first < data_end) {
        if (it->first >= data_end) {
            return;
        }
        bool no_lower = (it->first == start_idx);
        input = std::string(std::move(input.substr(0, it->first - start_idx)));
        auto tmp_it = it;
        auto prev = tmp_it;
        auto next = ++tmp_it;
        uint64_t read_idx = prev->first + prev->second.size();

        for (; next != _buffer.end() && next->first < data_end; prev = next++) {
            uint64_t prev_end = prev->first + prev->second.size();
            read_idx = next->first + next->second.size();
            if (prev_end != next->first) {
                _do_push_buffer(data.substr(prev_end - index, next->first - prev_end), prev_end);
            }
        }
        if (read_idx < data_end) {
            _do_push_buffer(data.substr(read_idx - index), read_idx);
        }
        if (no_lower) {
            return;
        }
    }
    if (it != _buffer.begin()) {
        auto tmp_it = it;
        --tmp_it;
        uint64_t lower_end = tmp_it->first + tmp_it->second.size();
        if (lower_end > index) {
            if (lower_end >= index + input.size()) {
                return;
            }
            input = input.substr(lower_end - start_idx);
            start_idx = lower_end;
        }
    }
    _do_push_buffer(input, start_idx);
}

void StreamReassembler::_do_push_buffer(const std::string &data, const uint64_t index) {
    _buffer[index] = data;
    _n_unreasemble += data.size();
}

void StreamReassembler::_pop_buffer() {
    auto next_it = _buffer.end();
    for (auto it = _buffer.begin(); it != _buffer.end() && it->first == _first_unreasemble_idx; it = next_it) {
        uint64_t index = it->first;
        next_it = it;
        ++next_it;
        std::string ret = _do_pop_buffer(it);
        size_t nwrite = _output.write(ret);
        _first_unreasemble_idx += nwrite;
        if (nwrite < ret.size()) {
            _push_buffer(ret.substr(nwrite), index + nwrite);
        }
    }
}

std::string StreamReassembler::_do_pop_buffer(uint64_t index) {
    auto it = _buffer.find(index);
    return _do_pop_buffer(it);
}

std::string StreamReassembler::_do_pop_buffer(MAP_IDX_STR::iterator it) {
    if (it != _buffer.end()) {
        std::string tmp = it->second;
        _n_unreasemble -= it->second.size();
        _buffer.erase(it);
        return tmp;
    }
    return "";
}

size_t StreamReassembler::unassembled_bytes() const { return _n_unreasemble; }

bool StreamReassembler::empty() const { return unassembled_bytes() == 0; }