#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

Iterator ByteStream::_iterator(const size_t idx) const {
    _check_idx(idx);
    return &_buffer.get()[idx];
}

size_t ByteStream::_len_to_end(const size_t idx) const {
    _check_idx(idx);
    return _capicity - idx;
}

bool ByteStream::_need_to_ring(const size_t input_len) const {
    const size_t len_to_end = _len_to_end(_write_idx);
    return _write_idx >= _read_idx && len_to_end < input_len;
}

ByteStream::ByteStream(const size_t capacity)
    : _capicity(capacity)
    , _write_idx(0)
    , _read_idx(0)
    , _bytes_written(0)
    , _bytes_read(0)
    , _buffer_size(0)
    , _buffer(ByteBuf(new byte[capacity]))
    , _is_ended(false)
    , _error(false) {}

size_t ByteStream::write(const std::string &data) {
    const size_t len = data.length();
    if (len == 0) {
        return 0;
    }
    size_t write_len = std::min(len, remaining_capacity());
    Iterator write_pos = _iterator(_write_idx);
    const byte *start = data.c_str();
    const size_t len_to_end = _len_to_end(_write_idx);
    if (_write_idx >= _read_idx && len_to_end < write_len) {
        // split into two parts

        memcpy(write_pos, start, len_to_end);
        const byte *half = start + len_to_end;
        const size_t remain_len = write_len - len_to_end;
        memcpy(_iterator(0), half, remain_len);
    } else {
        memcpy(write_pos, data.c_str(), write_len);
    }
    _write_idx = (_write_idx + write_len) % _capicity;
    _bytes_written += write_len;
    _buffer_size += write_len;
    return write_len;
}

//! \param[in] len bytes will be copied from the output side of the buffer
std::string ByteStream::peek_output(const size_t len) const {
    if (len == 0) {
        return "";
    }
    size_t read_len = std::min(len, buffer_size());
    Iterator read_pos = _iterator(_read_idx);

    std::shared_ptr<byte> out(new byte[read_len]);
    const size_t len_to_end = _len_to_end(_write_idx);
    const Iterator dst_start = out.get();
    if (_read_idx >= _write_idx && len_to_end < read_len) {
        memcpy(dst_start, read_pos, len_to_end);
        memcpy(dst_start + len_to_end, _iterator(0), read_len - len_to_end);
    } else {
        memcpy(out.get(), read_pos, read_len);
    }
    return std::string(dst_start, read_len);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    const size_t read_len = std::min(len, buffer_size());
    _read_idx = (_read_idx + read_len) % _capicity;
    _bytes_read += read_len;
    _buffer_size -= read_len;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    std::string out = peek_output(len);
    pop_output(len);
    return out;
}

void ByteStream::end_input() { _is_ended = true; }

bool ByteStream::input_ended() const { return _is_ended; }

size_t ByteStream::buffer_size() const { return _buffer_size; }

bool ByteStream::buffer_empty() const { return _buffer_size == 0; }

bool ByteStream::eof() const { return input_ended() && buffer_empty(); }

size_t ByteStream::bytes_written() const { return _bytes_written; }

size_t ByteStream::bytes_read() const { return _bytes_read; }

size_t ByteStream::remaining_capacity() const { return _capicity - _buffer_size; }
