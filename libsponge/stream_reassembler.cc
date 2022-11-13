#include "stream_reassembler.hh"

#include <iostream>
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
    , _n_unreassembled_bytes(0)
    , _eof(false) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    const size_t data_len = data.size();
    const size_t first_unacceptable_idx = _next_idx + _capacity - _output.buffer_size();
    // when index < first_unacceptable_idx,we still accept all of the data,because the exceeded size is about 1kb,
    // which is acceptable
    const size_t data_end = index + data_len;
    if (_eof && data_end >= _eof_idx) {
        return;
    }
    if (eof) {
        _eof_idx = data_end;
        _eof = true;
    }

    if (index >= first_unacceptable_idx || data_len == 0) {
        _manage_eof();
        return;
    }
    // check _unreassembled_strs and clean out expire data
    _try_poll_unreassemblered();

    std::string input = data;

    if (index > _next_idx) {
        _push_to_unreassemblered(index, input);
        return;
    } else {
        if (data_end <= _next_idx) {
            _manage_eof();
            return;
        }
        input = data.substr(_next_idx - index);
    }
    // try to output
    _try_write(input);
    _manage_eof();
}

void StreamReassembler::_manage_eof() {
    if (_eof_idx <= _next_idx) {
        _output.end_input();
    }
}

void StreamReassembler::_try_poll_unreassemblered() {
    auto it = _unreassembled_strs.find(_next_idx);
    if (it != _unreassembled_strs.end()) {
        const std::string &input = std::string(std::move(it->second));
        _rm_from_unreassemblered(it);
        _try_write(input);
    }
}

size_t StreamReassembler::_try_write(const std::string &data) {
    if (_output.input_ended()) {
        return 0;
    }
    size_t w_len = 0;
    std::string input = data;
    const uint64_t data_end = _next_idx + data.size();
    auto upper_it = _unreassembled_strs.upper_bound(_next_idx);
    if (upper_it != _unreassembled_strs.end() && data_end > upper_it->first) {
        // overlap
        const uint64_t upper_l_idx = upper_it->first;
        const uint64_t split_pot = upper_it->first - _next_idx;
        input = input.substr(0, split_pot);
        const std::string rest = input.substr(split_pot);
        _rm_from_unreassemblered(upper_it);
        _push_to_unreassemblered(split_pot + upper_l_idx, rest);
    }
    do {
        w_len = _output.write(input);
        _next_idx += w_len;
        if (w_len < input.size()) {
            _push_to_unreassemblered(_next_idx, input.substr(w_len));
            break;
        }
        auto it = _unreassembled_strs.find(_next_idx);
        if (it == _unreassembled_strs.end()) {
            break;
        }
        input = it->second;
        _rm_from_unreassemblered(it);
    } while (true);
    return w_len;
}

void StreamReassembler::_rm_from_unreassemblered(MAP_IDX_STR::iterator it) {
    _n_unreassembled_bytes -= it->second.size();
    _unreassembled_strs.erase(it);
}

void StreamReassembler::_push_to_unreassemblered(const size_t index, const std::string &data) {
    if (data == "") {
        return;
    }
    CombinationDiscriptor cb(_unreassembled_strs);
    // next idx of the last byte
    uint64_t data_end_idx = index + data.size();
    cb.left_idx = index;
    cb.right_idx = data_end_idx - 1;
    cb.upper_it = _unreassembled_strs.lower_bound(index);
    bool miss_upper = ((cb.upper_it == _unreassembled_strs.end()) || (cb.upper_it->first >= data_end_idx));

    auto rit = MAP_IDX_STR::reverse_iterator(cb.upper_it);
    ++rit;
    cb.lower_it = rit.base();
    bool miss_lower = ((rit == _unreassembled_strs.rend()) || (rit->first + rit->second.size() <= index));
    if (miss_lower) {
        cb.left_idx = index;
        if (cb.right_idx == data_end_idx) {
            _do_push_to_unreassemblered(index, data);
            return;
        }
    } else if (cb.lower_it->first + cb.lower_it->second.size() >= data_end_idx) {
        return;
    }

    std::string to_insert_str;
    if (!miss_lower) {
        // to_insert_str += (tmp_str + data.substr(cb.r_lower_idx - index + 1));
        cb.r_lower_idx = cb.lower_it->first + cb.lower_it->second.size() - 1;
        cb.left_idx = cb.r_lower_idx + 1;
    }
    if (!miss_upper) {
        cb.l_upper_idx = cb.upper_it->first;
        cb.r_upper_idx = cb.upper_it->first + cb.upper_it->second.size() - 1;
        cb.right_idx = cb.l_upper_idx - 1;
        if (cb.r_upper_idx < data_end_idx - 1) {
            // multiple upper case
            auto prev_it = cb.upper_it;
            for (auto it = ++cb.upper_it; it != _unreassembled_strs.end() && it->first < data_end_idx; ++it) {
                uint64_t start_idx = cb.r_upper_idx + 1;
                std::string sub = data.substr(start_idx - index, it->first - start_idx);
                _do_push_to_unreassemblered(start_idx, sub);
                prev_it = it;
                cb.l_upper_idx = prev_it->first;
                cb.r_upper_idx = prev_it->first + prev_it->second.size() - 1;
            }
        }
    }
    to_insert_str = data.substr(cb.l_upper_idx - index, cb.right_idx - cb.left_idx + 1);
    _do_push_to_unreassemblered(cb.left_idx, to_insert_str);
}

void StreamReassembler::_do_push_to_unreassemblered(const uint64_t index, const std::string &data) {
    _unreassembled_strs[index] = data;
    _n_unreassembled_bytes += data.size();
}

size_t StreamReassembler::unassembled_bytes() const { return _n_unreassembled_bytes; }

bool StreamReassembler::empty() const { return _n_unreassembled_bytes == 0; }
