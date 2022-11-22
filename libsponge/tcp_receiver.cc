#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

void TCPReceiver::segment_received(const TCPSegment &seg) {
    const TCPHeader &header = seg.header();
    const WrappingInt32 &seqno = header.seqno;
    const bool is_syn = header.syn;
    if (_stat == LISTEN) {
        if (!is_syn) {
            return;
        }
        // recv syn
        _stat = SYN_RECV;
        _isn = seqno;
    }
    // caveat: +1 to cover the syn packet seqno
    const uint64_t curr_abs_seqno = _curr_stream_idx + 1 - is_syn;
    const uint64_t absolute_seqno = unwrap(seqno, _isn, curr_abs_seqno);
    // if just syn,next_stream_idx equals absolute_seqno, else if during SYN_RECV,next_stream_idx should exclude syn idx
    const uint input_stream_idx = absolute_seqno - 1 + is_syn;
    _reassembler.push_substring(seg.payload().copy(), input_stream_idx, header.fin);
    // flush _curr_stream_idx
    _curr_stream_idx = _reassembler.stream_out().bytes_written();
    if (_reassembler.stream_out().input_ended()) {
        _stat = FIN_RECV;
    }
}

std::optional<WrappingInt32> TCPReceiver::ackno() const {
    if (_stat == LISTEN) {
        return std::nullopt;
    }
    // calculate next absolute seqno,plus for the syn packet
    uint64_t next_abs_seqno = _curr_stream_idx + 1;
    if (_stat == FIN_RECV) {
        // plus for the fin packet
        ++next_abs_seqno;
    }
    return wrap(next_abs_seqno, _isn);
}

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().buffer_size(); }
