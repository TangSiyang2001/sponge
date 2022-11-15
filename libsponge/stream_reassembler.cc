#include "stream_reassembler.hh"

#include <cassert>
#include <iostream>

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
    std::cout<<"ang? "<<index<<" end? "<<data_end<<" len? "<<data_len<<" unre? "<<_unreachable_idx<<std::endl;
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

            std::cout << "cuol" << _first_unreasemble_idx << " " << index << std::endl;
            _push_buffer(tmp.substr(split_pot), it->first);
        }
        nwrite = _output.write(input);
        std::cout << "n::::" << nwrite << std::endl;
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
                // std::cout << "1------------>" << prev_end - index << "data:" << data << std::endl;
                _do_push_buffer(data.substr(prev_end - index, next->first - prev_end), prev_end);
            }
        }
        if (read_idx < data_end) {
            // std::cout << "2------------>" << read_idx - index << "data:" << data << std::endl;
            std::cout << read_idx << "minos" << index << std::endl;
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
            // std::cout << "4------------>" << lower_end - index << "data:" << input << std::endl;
            input = input.substr(lower_end - start_idx);
            start_idx = lower_end;
        }
    }
    if (input.size() == 1) {
        std::cout << "sub::" << input << " stt::" << start_idx << std::endl;
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
        std::cout << "active index:" << it->first << std::endl;
        uint64_t index = it->first;
        next_it = it;
        ++next_it;
        std::string ret = _do_pop_buffer(it);
        size_t nwrite = _output.write(ret);
        std::cout << "n::::" << nwrite << std::endl;
        _first_unreasemble_idx += nwrite;
        // std::cout << nwrite << "---" << _first_unreasemble_idx << "---" << ret.size() << std::endl;
        // std::cout << "now_size:" << _buffer.size() << " next" << _first_unreasemble_idx << std::endl;
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
        std::cout << "pop:" << tmp.size() << "index:" << it->first << std::endl;
        if (it->second.size() == 0) {
            std::cout << "what??";
        }
        _n_unreasemble -= it->second.size();
        _buffer.erase(it);
        return tmp;
    }
    return "";
}

size_t StreamReassembler::unassembled_bytes() const { return _n_unreasemble; }

bool StreamReassembler::empty() const { return unassembled_bytes() == 0; }

// auto lower_it = _buffer.upper_bound(index);
//     if (lower_it != _buffer.begin()) {
//         --lower_it;
//     }

//     size_t new_idx = index;
//     if (lower_it != _buffer.end() && lower_it->first <= index) {
//         const size_t l_lower_idx = lower_it->first;

//         if (index < l_lower_idx + lower_it->second.size()) {
//             new_idx = l_lower_idx + lower_it->second.size();
//         }
//     } else if (index < _next_idx) {
//         new_idx = _next_idx;
//     }
//     const size_t data_start_pos = new_idx - index;
//     ssize_t data_size = data.size() - data_start_pos;
//     lower_it = _buffer.lower_bound(new_idx);
//     while (lower_it != _buffer.end() && new_idx <= lower_it->first) {
//         const size_t data_end_pos = new_idx + data_size;
//         if (lower_it->first < data_end_pos) {
//             if (data_end_pos < lower_it->first + lower_it->second.size()) {
//                 data_size = lower_it->first - new_idx;
//                 break;
//             } else {
//                 _n_unreassembled -= lower_it->second.size();
//                 lower_it = _buffer.erase(lower_it);
//                 continue;
//             }
//         } else {
//             break;
//         }
//     }

//     size_t first_unacceptable_idx = _next_idx + _capacity - _output.buffer_size();
//     if (first_unacceptable_idx <= new_idx) {
//         return;
//     }

//     if (data_size > 0) {
//         const string new_data = data.substr(data_start_pos, data_size);

//         if (new_idx == _next_idx) {
//             const size_t write_byte = _output.write(new_data);
//             _next_idx += write_byte;

//             if (write_byte < new_data.size()) {
//                 const string data_to_store = new_data.substr(write_byte, new_data.size() - write_byte);
//                 _n_unreassembled += data_to_store.size();
//                 _buffer.insert(make_pair(_next_idx, std::move(data_to_store)));
//             }
//         } else {
//             const string data_to_store = new_data.substr(0, new_data.size());
//             _n_unreassembled += data_to_store.size();
//             _buffer.insert(make_pair(new_idx, std::move(data_to_store)));
//         }
//     }

//     for (auto iter = _buffer.begin(); iter != _buffer.end(); /* nop */) {
//         assert(_next_idx <= iter->first);
//         if (iter->first == _next_idx) {
//             const size_t write_num = _output.write(iter->second);
//             _next_idx += write_num;

//             if (write_num < iter->second.size()) {
//                 _n_unreassembled += iter->second.size() - write_num;
//                 _buffer.insert(make_pair(_next_idx, std::move(iter->second.substr(write_num))));

//                 _n_unreassembled -= iter->second.size();
//                 _buffer.erase(iter);
//                 break;
//             }

//             _n_unreassembled -= iter->second.size();
//             iter = _buffer.erase(iter);
//         }
//         else {
//             break;
//         }
//     }
//     if (eof) {
//         _eof_idx = index + data.size();
//     }
//     if (_eof_idx <= _next_idx) {
//         _output.end_input();
//     }