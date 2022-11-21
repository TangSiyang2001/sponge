#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

void TCPReceiver::segment_received(const TCPSegment &seg) {
    WrappingInt32 seqno = seg.header().seqno;
    bool is_syn = seg.header().syn;
    if (_stat == LISTEN) {
        if (!is_syn) {
            return;
        }
        // recv syn
        _stat = SYN_RECV;
        _isn = seqno;
    }
    uint64_t curr_stream_idx = _reassembler.stream_out().bytes_written();
    // cavat: +1 to cover the syn packet seqno
    uint64_t curr_abs_seqno = curr_stream_idx + 1;
    uint64_t absolute_seqno = unwrap(seqno, _isn, curr_abs_seqno);
    uint next_stream_idx = absolute_seqno - 1 + is_syn;
    _reassembler.push_substring(seg.payload().copy(),next_stream_idx,seg.header().fin);
}

std::optional<WrappingInt32> TCPReceiver::ackno() const { return {}; }

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().buffer_size(); }
