#ifndef SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
#define SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

#include "byte_stream.hh"

#include <algorithm>
#include <cstdint>
#include <map>
#include <string>

using MAP_IDX_STR = std::map<size_t, std::string>;
//! \brief A class that assembles a series of excerpts from a byte stream (possibly out of order,
//! possibly overlapping) into an in-order byte stream.
//  |------read---------||----output------|   |--buffer--|
//!                      |---------- capicity--------------|
//! |--------------------|----------------|---------------------|------------------>
//! begin            write_idx  _first_unreasemble_idx      unreach_idx
class StreamReassembler {
  private:
    std::map<uint64_t, std::string> _buffer;

    uint64_t _first_unreasemble_idx;
    // idx of the first unreachable byte
    uint64_t _unreachable_idx;
    // the last
    uint64_t _eof_idx;
    // true if eof is passed in
    bool _eof;
    size_t _n_unreasemble;
    ByteStream _output;  //!< The reassembled in-order byte stream
    size_t _capacity;    //!< The maximum number of bytes

  private:
    void _inbound(const std::string &data, const uint64_t index);

    void _push_buffer(const std::string &data, const uint64_t index);

    void _do_push_buffer(const std::string &data, const uint64_t index);

    void _pop_buffer();

    std::string _do_pop_buffer(const uint64_t index);

    std::string _do_pop_buffer(MAP_IDX_STR::iterator it);

  public:
    //! \brief Construct a `StreamReassembler` that will store up to `capacity` bytes.
    //! \note This capacity limits both the bytes that have been reassembled,
    //! and those that have not yet been reassembled.
    StreamReassembler(const size_t capacity);

    //! \brief Receive a substring and write any newly contiguous bytes into the stream.
    //!
    //! The StreamReassembler will stay within the memory limits of the `capacity`.
    //! Bytes that would exceed the capacity are silently discarded.
    //!
    //! \param data the substring
    //! \param index indicates the index (place in sequence) of the first byte in `data`
    //! \param eof the last byte of `data` will be the last byte in the entire stream
    void push_substring(const std::string &data, const uint64_t index, const bool eof);

    //! \name Access the reassembled byte stream
    //!@{
    const ByteStream &stream_out() const { return _output; }
    ByteStream &stream_out() { return _output; }
    //!@}

    //! The number of bytes in the substrings stored but not yet reassembled
    //!
    //! \note If the byte at a particular index has been pushed more than once, it
    //! should only be counted once for the purpose of this function.
    size_t unassembled_bytes() const;

    //! \brief Is the internal state empty (other than the output stream)?
    //! \returns `true` if no substrings are waiting to be assembled
    bool empty() const;
};

#endif  // SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH