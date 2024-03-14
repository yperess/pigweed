#!/usr/bin/env python3
# Copyright 2022 The Pigweed Authors
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
"""Unit test for proxy.py"""

import abc
import asyncio
from struct import pack
import time
from typing import List
import unittest

from pigweed.pw_rpc.internal import packet_pb2
from pigweed.pw_transfer import transfer_pb2
from pw_hdlc import encode
from pw_transfer.chunk import Chunk, ProtocolVersion

import proxy


class MockRng(abc.ABC):
    def __init__(self, results: List[float]):
        self._results = results

    def uniform(self, from_val: float, to_val: float) -> float:
        val_range = to_val - from_val
        val = self._results.pop()
        val *= val_range
        val += from_val
        return val


class ProxyTest(unittest.IsolatedAsyncioTestCase):
    async def test_transposer_simple(self):
        sent_packets: List[bytes] = []
        new_packets_event: asyncio.Event = asyncio.Event()

        # Async helper so DataTransposer can await on it.
        async def append(list: List[bytes], data: bytes):
            list.append(data)
            # Notify that a new packet was "sent".
            new_packets_event.set()

        transposer = proxy.DataTransposer(
            lambda data: append(sent_packets, data),
            name="test",
            rate=0.5,
            timeout=100,
            seed=1234567890,
        )
        transposer._rng = MockRng([0.6, 0.4])
        await transposer.process(b'aaaaaaaaaa')
        await transposer.process(b'bbbbbbbbbb')

        expected_packets = [b'bbbbbbbbbb', b'aaaaaaaaaa']
        while True:
            # Wait for new packets with a generous timeout.
            try:
                await asyncio.wait_for(new_packets_event.wait(), timeout=60.0)
            except TimeoutError:
                self.fail(
                    f'Timeout waiting for data.  Packets sent: {sent_packets}'
                )

            # Only assert the sent packets are corrected when we've sent the
            # expected number.
            if len(sent_packets) == len(expected_packets):
                self.assertEqual(sent_packets, expected_packets)
                return

    async def test_transposer_timeout(self):
        sent_packets: List[bytes] = []

        # Async helper so DataTransposer can await on it.
        async def append(list: List[bytes], data: bytes):
            list.append(data)

        transposer = proxy.DataTransposer(
            lambda data: append(sent_packets, data),
            name="test",
            rate=0.5,
            timeout=0.100,
            seed=1234567890,
        )
        transposer._rng = MockRng([0.4, 0.6])
        await transposer.process(b'aaaaaaaaaa')

        # Even though this should be transposed, there is no following data so
        # the transposer should timout and send this in-order.
        await transposer.process(b'bbbbbbbbbb')

        # Give the transposer time to timeout.
        await asyncio.sleep(0.5)

        self.assertEqual(sent_packets, [b'aaaaaaaaaa', b'bbbbbbbbbb'])

    async def test_server_failure(self):
        sent_packets: List[bytes] = []

        # Async helper so DataTransposer can await on it.
        async def append(list: List[bytes], data: bytes):
            list.append(data)

        packets_before_failure = [1, 2, 3]
        server_failure = proxy.ServerFailure(
            lambda data: append(sent_packets, data),
            name="test",
            packets_before_failure_list=packets_before_failure.copy(),
            start_immediately=True,
        )

        # After passing the list to ServerFailure, add a test for no
        # packets dropped
        packets_before_failure.append(5)

        packets = [
            b'1',
            b'2',
            b'3',
            b'4',
            b'5',
        ]

        for num_packets in packets_before_failure:
            sent_packets.clear()
            for packet in packets:
                await server_failure.process(packet)
            self.assertEqual(len(sent_packets), num_packets)
            server_failure.handle_event(
                proxy.Event(
                    proxy.EventType.TRANSFER_START,
                    Chunk(ProtocolVersion.VERSION_TWO, Chunk.Type.START),
                )
            )

    async def test_server_failure_transfer_chunks_only(self):
        sent_packets = []

        # Async helper so DataTransposer can await on it.
        async def append(list: List[bytes], data: bytes):
            list.append(data)

        packets_before_failure = [2]
        server_failure = proxy.ServerFailure(
            lambda data: append(sent_packets, data),
            name="test",
            packets_before_failure_list=packets_before_failure.copy(),
            start_immediately=True,
            only_consider_transfer_chunks=True,
        )

        transfer_chunk = _encode_rpc_frame(
            Chunk(ProtocolVersion.VERSION_TWO, Chunk.Type.DATA, data=b'1')
        )

        packets = [
            b'1',
            b'2',
            transfer_chunk,  # 1
            b'3',
            transfer_chunk,  # 2
            b'4',
            b'5',
            transfer_chunk,  # Transfer chunks should be dropped starting here.
            transfer_chunk,
            b'6',
            b'7',
            transfer_chunk,
        ]

        for packet in packets:
            await server_failure.process(packet)

        expected_result = [
            b'1',
            b'2',
            transfer_chunk,
            b'3',
            transfer_chunk,
            b'4',
            b'5',
            b'6',
            b'7',
        ]
        self.assertEqual(sent_packets, expected_result)

    async def test_keep_drop_queue_loop(self):
        sent_packets: List[bytes] = []

        # Async helper so DataTransposer can await on it.
        async def append(list: List[bytes], data: bytes):
            list.append(data)

        keep_drop_queue = proxy.KeepDropQueue(
            lambda data: append(sent_packets, data),
            name="test",
            keep_drop_queue=[2, 1, 3],
        )

        expected_sequence = [
            b'1',
            b'2',
            b'4',
            b'5',
            b'6',
            b'9',
        ]
        input_packets = [
            b'1',
            b'2',
            b'3',
            b'4',
            b'5',
            b'6',
            b'7',
            b'8',
            b'9',
        ]

        for packet in input_packets:
            await keep_drop_queue.process(packet)
        self.assertEqual(sent_packets, expected_sequence)

    async def test_keep_drop_queue(self):
        sent_packets: List[bytes] = []

        # Async helper so DataTransposer can await on it.
        async def append(list: List[bytes], data: bytes):
            list.append(data)

        keep_drop_queue = proxy.KeepDropQueue(
            lambda data: append(sent_packets, data),
            name="test",
            keep_drop_queue=[2, 1, 1, -1],
        )

        expected_sequence = [
            b'1',
            b'2',
            b'4',
        ]
        input_packets = [
            b'1',
            b'2',
            b'3',
            b'4',
            b'5',
            b'6',
            b'7',
            b'8',
            b'9',
        ]

        for packet in input_packets:
            await keep_drop_queue.process(packet)
        self.assertEqual(sent_packets, expected_sequence)

    async def test_keep_drop_queue_transfer_chunks_only(self):
        sent_packets: List[bytes] = []

        # Async helper so DataTransposer can await on it.
        async def append(list: List[bytes], data: bytes):
            list.append(data)

        keep_drop_queue = proxy.KeepDropQueue(
            lambda data: append(sent_packets, data),
            name="test",
            keep_drop_queue=[2, 1, 1, -1],
            only_consider_transfer_chunks=True,
        )

        transfer_chunk = _encode_rpc_frame(
            Chunk(ProtocolVersion.VERSION_TWO, Chunk.Type.DATA, data=b'1')
        )

        expected_sequence = [
            b'1',
            transfer_chunk,
            b'2',
            transfer_chunk,
            b'3',
            b'4',
            b'5',
            b'6',
            b'7',
            transfer_chunk,
            b'8',
            b'9',
            b'10',
        ]
        input_packets = [
            b'1',
            transfer_chunk,  # keep
            b'2',
            transfer_chunk,  # keep
            b'3',
            b'4',
            b'5',
            transfer_chunk,  # drop
            b'6',
            b'7',
            transfer_chunk,  # keep
            transfer_chunk,  # drop
            b'8',
            transfer_chunk,  # drop
            b'9',
            transfer_chunk,  # drop
            transfer_chunk,  # drop
            b'10',
        ]

        for packet in input_packets:
            await keep_drop_queue.process(packet)
        self.assertEqual(sent_packets, expected_sequence)

    async def test_window_packet_dropper(self):
        sent_packets: List[bytes] = []

        # Async helper so DataTransposer can await on it.
        async def append(list: List[bytes], data: bytes):
            list.append(data)

        window_packet_dropper = proxy.WindowPacketDropper(
            lambda data: append(sent_packets, data),
            name="test",
            window_packet_to_drop=0,
        )

        packets = [
            _encode_rpc_frame(
                Chunk(
                    ProtocolVersion.VERSION_TWO,
                    Chunk.Type.DATA,
                    data=b'1',
                    session_id=1,
                )
            ),
            _encode_rpc_frame(
                Chunk(
                    ProtocolVersion.VERSION_TWO,
                    Chunk.Type.DATA,
                    data=b'2',
                    session_id=1,
                )
            ),
            _encode_rpc_frame(
                Chunk(
                    ProtocolVersion.VERSION_TWO,
                    Chunk.Type.DATA,
                    data=b'3',
                    session_id=1,
                )
            ),
            _encode_rpc_frame(
                Chunk(
                    ProtocolVersion.VERSION_TWO,
                    Chunk.Type.DATA,
                    data=b'4',
                    session_id=1,
                )
            ),
            _encode_rpc_frame(
                Chunk(
                    ProtocolVersion.VERSION_TWO,
                    Chunk.Type.DATA,
                    data=b'5',
                    session_id=1,
                )
            ),
        ]

        expected_packets = packets[1:]

        # Test each even twice to assure the filter does not have issues
        # on new window bondaries.
        events = [
            proxy.Event(
                proxy.EventType.PARAMETERS_RETRANSMIT,
                Chunk(
                    ProtocolVersion.VERSION_TWO,
                    Chunk.Type.PARAMETERS_RETRANSMIT,
                ),
            ),
            proxy.Event(
                proxy.EventType.PARAMETERS_CONTINUE,
                Chunk(
                    ProtocolVersion.VERSION_TWO, Chunk.Type.PARAMETERS_CONTINUE
                ),
            ),
            proxy.Event(
                proxy.EventType.PARAMETERS_RETRANSMIT,
                Chunk(
                    ProtocolVersion.VERSION_TWO,
                    Chunk.Type.PARAMETERS_RETRANSMIT,
                ),
            ),
            proxy.Event(
                proxy.EventType.PARAMETERS_CONTINUE,
                Chunk(
                    ProtocolVersion.VERSION_TWO, Chunk.Type.PARAMETERS_CONTINUE
                ),
            ),
        ]

        for event in events:
            sent_packets.clear()
            for packet in packets:
                await window_packet_dropper.process(packet)
            self.assertEqual(sent_packets, expected_packets)
            window_packet_dropper.handle_event(event)

    async def test_window_packet_dropper_extra_in_flight_packets(self):
        sent_packets: List[bytes] = []

        # Async helper so DataTransposer can await on it.
        async def append(list: List[bytes], data: bytes):
            list.append(data)

        window_packet_dropper = proxy.WindowPacketDropper(
            lambda data: append(sent_packets, data),
            name="test",
            window_packet_to_drop=1,
        )

        packets = [
            _encode_rpc_frame(
                Chunk(
                    ProtocolVersion.VERSION_TWO,
                    Chunk.Type.DATA,
                    data=b'1',
                    offset=0,
                )
            ),
            _encode_rpc_frame(
                Chunk(
                    ProtocolVersion.VERSION_TWO,
                    Chunk.Type.DATA,
                    data=b'2',
                    offset=1,
                )
            ),
            _encode_rpc_frame(
                Chunk(
                    ProtocolVersion.VERSION_TWO,
                    Chunk.Type.DATA,
                    data=b'3',
                    offset=2,
                )
            ),
            _encode_rpc_frame(
                Chunk(
                    ProtocolVersion.VERSION_TWO,
                    Chunk.Type.DATA,
                    data=b'2',
                    offset=1,
                )
            ),
            _encode_rpc_frame(
                Chunk(
                    ProtocolVersion.VERSION_TWO,
                    Chunk.Type.DATA,
                    data=b'3',
                    offset=2,
                )
            ),
            _encode_rpc_frame(
                Chunk(
                    ProtocolVersion.VERSION_TWO,
                    Chunk.Type.DATA,
                    data=b'4',
                    offset=3,
                )
            ),
        ]

        expected_packets = packets[1:]

        # Test each even twice to assure the filter does not have issues
        # on new window bondaries.
        events = [
            None,
            proxy.Event(
                proxy.EventType.PARAMETERS_RETRANSMIT,
                Chunk(
                    ProtocolVersion.VERSION_TWO,
                    Chunk.Type.PARAMETERS_RETRANSMIT,
                    offset=1,
                ),
            ),
            None,
            None,
            None,
            None,
        ]

        for packet, event in zip(packets, events):
            await window_packet_dropper.process(packet)
            if event is not None:
                window_packet_dropper.handle_event(event)

        expected_packets = [packets[0], packets[2], packets[3], packets[5]]
        self.assertEqual(sent_packets, expected_packets)

    async def test_event_filter(self):
        sent_packets: List[bytes] = []

        # Async helper so EventFilter can await on it.
        async def append(list: List[bytes], data: bytes):
            list.append(data)

        queue = asyncio.Queue()

        event_filter = proxy.EventFilter(
            lambda data: append(sent_packets, data),
            name="test",
            event_queue=queue,
        )

        request = packet_pb2.RpcPacket(
            type=packet_pb2.PacketType.REQUEST,
            channel_id=101,
            service_id=1001,
            method_id=100001,
        ).SerializeToString()

        packets = [
            request,
            _encode_rpc_frame(
                Chunk(
                    ProtocolVersion.VERSION_TWO, Chunk.Type.START, session_id=1
                )
            ),
            _encode_rpc_frame(
                Chunk(
                    ProtocolVersion.VERSION_TWO,
                    Chunk.Type.DATA,
                    session_id=1,
                    data=b'3',
                )
            ),
            _encode_rpc_frame(
                Chunk(
                    ProtocolVersion.VERSION_TWO,
                    Chunk.Type.DATA,
                    session_id=1,
                    data=b'3',
                )
            ),
            request,
            _encode_rpc_frame(
                Chunk(
                    ProtocolVersion.VERSION_TWO, Chunk.Type.START, session_id=2
                )
            ),
            _encode_rpc_frame(
                Chunk(
                    ProtocolVersion.VERSION_TWO,
                    Chunk.Type.DATA,
                    session_id=2,
                    data=b'4',
                )
            ),
            _encode_rpc_frame(
                Chunk(
                    ProtocolVersion.VERSION_TWO,
                    Chunk.Type.DATA,
                    session_id=2,
                    data=b'5',
                )
            ),
        ]

        expected_events = [
            None,  # request
            proxy.EventType.TRANSFER_START,
            None,  # data chunk
            None,  # data chunk
            None,  # request
            proxy.EventType.TRANSFER_START,
            None,  # data chunk
            None,  # data chunk
        ]

        for packet, expected_event_type in zip(packets, expected_events):
            await event_filter.process(packet)
            try:
                event_type = queue.get_nowait().type
            except asyncio.QueueEmpty:
                event_type = None
            self.assertEqual(event_type, expected_event_type)


def _encode_rpc_frame(chunk: Chunk) -> bytes:
    packet = packet_pb2.RpcPacket(
        type=packet_pb2.PacketType.SERVER_STREAM,
        channel_id=101,
        service_id=1001,
        method_id=100001,
        payload=chunk.to_message().SerializeToString(),
    ).SerializeToString()
    return encode.ui_frame(73, packet)


if __name__ == '__main__':
    unittest.main()
