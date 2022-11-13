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
    , _n_unreassembled_bytes(0) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    std::string s = (eof ? "y" : "n");
    std::cout << "传了" + data + " " << index << " " + s + " next:" << _next_idx << std::endl;

    const size_t data_len = data.size();
    const size_t first_unacceptable_idx = _next_idx + _capacity - _output.buffer_size();
    // when index < first_unacceptable_idx,we still accept all of the data,because the exceeded size is about 1kb,
    // which is acceptable
    const size_t data_end = index + data_len - 1;
    if (eof) {
        _eof_idx = data_end + 1;
    }
    if (_output.input_ended() || index >= first_unacceptable_idx) {
        return;
    }
    if (data_len == 0) {
        if (_eof_idx <= _next_idx) {
            printf("已经结束辣\n");
            _output.end_input();
        }
        return;
    }
    // check _unreassembled_strs and clean out expire data
    std::string input = data;

    auto it = _unreassembled_strs.find(_next_idx);
    if (it != _unreassembled_strs.end()) {
        std::cout << "fffff" << std::endl;
        _try_write(it->second);
    }
    if (index > _next_idx) {
        _push_to_unreassemblered(index, input);
        return;
    } else {
        if (data_end < _next_idx) {
            std::cout << "qaq" << std::endl;
            return;
        }
        std::cout << _next_idx << "----" << data_end << std::endl;
        input = data.substr(_next_idx - index);
    }
    // try to output
    _try_write(input);
}

void StreamReassembler::_try_write(const std::string &data) {
    size_t w_len = 0;
    std::string input = data;
    bool from_unreass = false;
    bool end_input = (_eof_idx <= _next_idx);
    while (_output.remaining_capacity() > 0) {
        w_len = _output.write(input);
        _next_idx += w_len;
        _evict_expired();
        std::cout << ("写进去了" + data.substr(0, w_len)) << " 共" << w_len << "个,更新" << _next_idx << std::endl;
        end_input = _eof_idx <= _next_idx;
        if (end_input) {
            printf("已经结束辣\n");
            _output.end_input();
            break;
        }
        auto next = _unreassembled_strs.find(_next_idx);
        if (next != _unreassembled_strs.end()) {
            std::cout << "到了这里了" << std::endl;
            input = next->second;
            if (from_unreass) {
                _n_unreassembled_bytes -= w_len;
            }
            from_unreass = true;
        } else {
            break;
        }
    }
    if (w_len < input.size()) {
        std::cout << "居然到了这里了" << std::endl;
        _push_to_unreassemblered(_next_idx, input.substr(w_len));
    }
    std::cout << "_try_write_finish,next_idx " << _next_idx << std::endl;
}

void StreamReassembler::_evict_expired() {
    auto limit = _unreassembled_strs.lower_bound(_next_idx);
    for (auto iter = _unreassembled_strs.begin(); iter != limit; ++iter) {
        const std::string bytes = std::string(std::move(iter->second));
        const uint64_t end_idx = iter->first + bytes.size() - 1;
        const uint64_t start_idx = iter->first;
        _unreassembled_strs.erase(iter);
        if (end_idx >= _next_idx) {
            _unreassembled_strs[_next_idx] = bytes.substr(_next_idx - start_idx);
            _n_unreassembled_bytes -= (_next_idx - start_idx + 1);
        } else {
            _n_unreassembled_bytes -= bytes.size();
        }
    }
}

void StreamReassembler::_push_to_unreassemblered(const size_t index, const std::string &data) {
    if (data == "") {
        return;
    }
    CombinationDiscriptor cb(_unreassembled_strs);
    uint64_t data_end_idx = index + data.size() - 1;
    cb.upper_it = _unreassembled_strs.lower_bound(index);
    bool miss_upper = ((cb.upper_it == _unreassembled_strs.end()) || (cb.upper_it->first > data_end_idx));
    if (miss_upper) {
        cb.right_idx = data_end_idx;
    }
    auto rit = MAP_IDX_STR::reverse_iterator(cb.upper_it);
    ++rit;
    cb.lower_it = rit.base();
    bool miss_lower = ((rit == _unreassembled_strs.rend()) || (rit->first + rit->second.size() - 1 < index));
    if (miss_lower) {
        cb.left_idx = index;
        if (cb.right_idx == data_end_idx) {
            _do_push_to_unreassemblered(index, data);
            return;
        }
    } else if (rit->first + rit->second.size() - 1 >= data_end_idx) {
        return;
    }

    std::string to_insert_str;
    if (!miss_lower) {
        // deal with lower,if reach here, there is overlap in lower part
        std::string tmp_str = cb.lower_it->second;
        size_t lower_len = tmp_str.size();
        cb.l_lower_idx = cb.lower_it->first;
        cb.r_lower_idx = cb.l_lower_idx + lower_len - 1;
        _n_unreassembled_bytes -= lower_len;
        _unreassembled_strs.erase(cb.lower_it->first);
        cb.left_idx = cb.l_lower_idx;
        to_insert_str += (tmp_str + data.substr(cb.r_lower_idx - index + 1));
    }
    if (!miss_upper) {
        // deal with upper,if reach here, there is overlap in upper part
        std::string tmp_str = cb.upper_it->second;
        cb.l_upper_idx = cb.upper_it->first;
        cb.r_upper_idx = cb.l_upper_idx + tmp_str.size() - 1;

        auto next_it = cb.upper_it;
        for (; next_it != _unreassembled_strs.end() && (next_it->first + next_it->second.size() - 1) <= data_end_idx;
             next_it++) {
            _n_unreassembled_bytes -= next_it->second.size();
            cb.l_upper_idx = next_it->first;
            cb.r_upper_idx = next_it->first + next_it->second.size() - 1;
            _unreassembled_strs.erase(next_it);
        }
        if (next_it != _unreassembled_strs.end() && cb.l_upper_idx <= data_end_idx) {
            to_insert_str +=
                (to_insert_str == ""
                     ? data
                     : to_insert_str +
                           _unreassembled_strs.find(cb.l_upper_idx)->second.substr(data_end_idx - cb.l_upper_idx + 1));
            _unreassembled_strs.erase(cb.l_upper_idx);
            _n_unreassembled_bytes -= (cb.r_upper_idx - cb.l_upper_idx + 1);
        }
    }
    _do_push_to_unreassemblered(cb.left_idx, to_insert_str);
    // deal with upper

    // // TODO: refactor to simplify
    // size_t data_len = data.size();
    // u_int64_t end_idx = data_len + index;

    // auto upper_it = _unreassembled_strs.lower_bound(index);
    // if (upper_it == _unreassembled_strs.end()) {
    //     auto l_lower_it = _unreassembled_strs.rbegin();
    //     uint64_t r_lower_idx = l_lower_it->first + l_lower_it->second.size();
    //     if (r_lower_idx < index) {
    //         // add
    //         _unreassembled_strs[index] = data;
    //         _n_unreassembled_bytes += data_len;
    //         return;
    //     }
    //     // merge
    //     std::string substr = data.substr(r_lower_idx - index + 1);
    //     _unreassembled_strs[l_lower_it->first] = l_lower_it->second + substr;
    //     _n_unreassembled_bytes += substr.size();
    //     return;
    // } else if (upper_it == _unreassembled_strs.begin()) {
    //     if (end_idx < upper_it->first) {
    //         // add
    //         _unreassembled_strs[index] = data;
    //         _n_unreassembled_bytes += data_len;
    //         return;
    //     }
    //     // merge
    //     const std::string substr = data.substr(0, upper_it->first - index + 1);
    //     const std::string new_data = substr + upper_it->second;
    //     _unreassembled_strs.erase(upper_it);
    //     _unreassembled_strs[index] = new_data;
    //     _n_unreassembled_bytes += substr.size();
    // } else {
    //     auto lower_it = upper_it;
    //     --lower_it;
    //     const uint64_t r_upper_idx = upper_it->first + upper_it->second.size();
    //     if (upper_it->first > end_idx && lower_it->first < index) {
    //         // add
    //         _unreassembled_strs[index] = data;
    //         _n_unreassembled_bytes += data_len;
    //         return;
    //     }
    //     const uint64_t r_lower_idx = lower_it->first + lower_it->second.size();
    //     if (r_upper_idx > end_idx) {
    //         std::string upper_str = std::string(std::move(upper_it->second));
    //         _unreassembled_strs.erase(upper_it);
    //         std::string sub_str = upper_str.substr(end_idx - r_upper_idx);
    //         std::string tmp_str = data + sub_str;

    //         _n_unreassembled_bytes += (sub_str.size() + data_len - upper_str.size());
    //         if (r_lower_idx < index) {
    //             _unreassembled_strs[index] = tmp_str;
    //             return;
    //         }
    //         sub_str = tmp_str.substr(r_lower_idx - index + 1);
    //         tmp_str = lower_it->second + sub_str;

    //         _n_unreassembled_bytes -= sub_str.size();
    //         _unreassembled_strs[lower_it->first] = tmp_str;
    //         return;
    //     } else {
    //         // should be careful with that case:
    //         //                    data
    //         //               +------------------------------------+
    //         //     |------------|     |---------------| ... |---------|
    //         //  l_lower      r_lower l_upper      r_upper
    //         auto new_upper_it = _unreassembled_strs.upper_bound(r_upper_idx);
    //         --new_upper_it;
    //         uint64_t new_r_upper_idx = new_upper_it->first + new_upper_it->second.size();
    //         std::string sub_str = new_upper_it->second.substr(new_r_upper_idx - end_idx + 1);
    //         std::string tmp_str = data + sub_str;

    //         _n_unreassembled_bytes += sub_str.size() + data_len;
    //         for (auto it = upper_it; it != new_upper_it; ++it) {
    //             _unreassembled_strs.erase(it);
    //             _n_unreassembled_bytes -= it->second.size();
    //         }
    //         _unreassembled_strs.erase(new_upper_it);
    //         _n_unreassembled_bytes -= new_upper_it->second.size();
    //         if (r_lower_idx < index) {
    //             _unreassembled_strs[index] = tmp_str;
    //             return;
    //         }
    //         _n_unreassembled_bytes -= (r_lower_idx - index + 1);
    //         tmp_str = lower_it->second + tmp_str.substr(r_lower_idx - index + 1);
    //         _unreassembled_strs[lower_it->first] = tmp_str;
    //     }
    // }
}

void StreamReassembler::_do_push_to_unreassemblered(const uint64_t index, const std::string &data) {
    _unreassembled_strs[index] = data;
    _n_unreassembled_bytes += data.size();
}

size_t StreamReassembler::unassembled_bytes() const { return _n_unreassembled_bytes; }

bool StreamReassembler::empty() const { return _n_unreassembled_bytes == 0; }
