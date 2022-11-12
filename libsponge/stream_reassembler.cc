#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _output(capacity)
    , _capacity(capacity)
    , _eof_idx(-1)
    , _next_idx(0)
    , _unreassembled_strs()
    , _n_unreassembled_bytes(0) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    const size_t data_len = data.size();
    if (data_len == 0 || _output.input_ended()) {
        return;
    }
    // check _unreassembled_strs and clean out expire data
    const size_t data_end = index + data_len;
    size_t start_idx = index;
    if (index > _next_idx) {
        _push_to_unreassemblered(index, data);
        return;
    } else {
        if (data_end <= _next_idx) {
            return;
        }
        start_idx = _next_idx;
    }
    // do write
    _write(start_idx, data);
}

void StreamReassembler::_write(const uint64_t start_idx, const std::string &data) {
    size_t w_len = 0;
    std::string input = data;
    do {
        w_len += _output.write(input);
        _next_idx = w_len + start_idx;
        _evict_expired();
        auto next = _unreassembled_strs.find(_next_idx);
        if (next != _unreassembled_strs.end()) {
            input = next->second;
        } else {
            break;
        }
    } while (_output.remaining_capacity() > 0);
    if (w_len < input.size()) {
        _push_to_unreassemblered(_next_idx, input.substr(w_len));
    }
}

void StreamReassembler::_evict_expired() {
    auto limit = _unreassembled_strs.lower_bound(_next_idx);
    for (auto iter = _unreassembled_strs.begin(); iter != limit; ++iter) {
        const size_t end_idx = iter->first + iter->second.size();
        if (end_idx >= _next_idx) {
            _unreassembled_strs[_next_idx] = iter->second.substr(end_idx - _next_idx);
        }
        _unreassembled_strs.erase(iter);
    }
}

void StreamReassembler::_push_to_unreassemblered(const size_t index, const std::string &data) {
    // deduplicate
    size_t data_len = data.size();
    _unreassembled_strs.lower_bound(index);
}

size_t StreamReassembler::unassembled_bytes() const { return _n_unreassembled_bytes; }

bool StreamReassembler::empty() const { return _n_unreassembled_bytes == 0; }
