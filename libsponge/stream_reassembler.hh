#ifndef SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
#define SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

#include "byte_stream.hh"

#include <cstdint>
#include <iterator>
#include <map>
#include <string>

using MAP_IDX_STR = std::map<uint64_t, std::string>;

//! \brief A class that assembles a series of excerpts from a byte stream (possibly out of order,
//! possibly overlapping) into an in-order byte stream.
class StreamReassembler {
  private:
    // Your code here -- add private members as necessary.

    ByteStream _output;  //!< The reassembled in-order byte stream
    size_t _capacity;    //!< The maximum number of bytes
    uint64_t _eof_idx;
    uint64_t _next_idx;
    // use rb tree to orderly manage bytes by idx
    MAP_IDX_STR _unreassembled_strs;
    size_t _n_unreassembled_bytes;
    bool _eof;

  private:
    size_t _try_write(const std::string &data);

    void _push_to_unreassemblered(const uint64_t index, const std::string &data);

    void _do_push_to_unreassemblered(const uint64_t index, const std::string &data);

    void _manage_eof();

    void _try_poll_unreassemblered();

    void _rm_from_unreassemblered(MAP_IDX_STR::iterator it);

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

struct CombinationDiscriptor {
    /*
     * Description of stream
     *
     *                    data
     *               +-------------+
     *     |------------|       |---------------|
     *  l_lower      r_lower  l_upper      r_upper
     *
     *
     *  should be careful with the multiple upper case:
     *                                data
     *              +------------------------------------+
     *      |--------------|     |------------------|  ...  |-----------------|
     *   l_lower  lower r_lower l_upper   upper   r_upper  l_upper_r upper_r r_upper_r
     */
    uint64_t l_lower_idx = -1;
    uint64_t r_lower_idx = -1;
    uint64_t l_upper_idx = -1;
    uint64_t r_upper_idx = -1;
    uint64_t left_idx = -1;
    uint64_t right_idx = -1;
    MAP_IDX_STR::iterator lower_it;
    MAP_IDX_STR::iterator upper_it;

    CombinationDiscriptor(MAP_IDX_STR map) : lower_it(map.end()), upper_it(map.end()) {}
};

#endif  // SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
