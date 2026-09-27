import unittest
from baccable_lab.can.usb_status import decode_status
from baccable_lab.cli import build_parser

class UsbStatusTests(unittest.TestCase):
    def test_live_c1_and_stale_bh_are_distinct(self):
        packet = bytearray(64)
        packet[:4] = bytes([1, 0, 3, 2])
        packet[4:8] = (123456).to_bytes(4, "little")
        packet[10:12] = (6000).to_bytes(2, "little")
        packet[16:23] = bytes([1, 1, 7, 3, 0, 3, 1])
        packet[23:27] = (0x08000000).to_bytes(4, "little")
        packet[62:64] = (4).to_bytes(2, "little")
        status = decode_status(packet)
        self.assertEqual(status["main_loop_ms"], 123456)
        self.assertEqual(status["pending_ack"], ["BH"])
        self.assertTrue(status["boards"]["C1"]["configured"])
        self.assertEqual(status["boards"]["C1"]["reset_flags"], "0x08000000")
        self.assertFalse(status["boards"]["BH"]["fresh"])
        self.assertEqual(status["boards"]["BH"]["age_ms"], 6000)
        self.assertEqual(status["boards"]["BH"]["uart_recoveries"], 4)

    def test_bad_protocol_rejected(self):
        for data in (b"", bytes(64), bytes([2]) + bytes(63)):
            with self.assertRaises(ValueError):
                decode_status(data)

    def test_doctor_diagnostics_are_opt_in(self):
        self.assertFalse(build_parser().parse_args(["doctor"]).usb_status)
        args = build_parser().parse_args(["doctor", "--usb-status", "--samples", "3"])
        self.assertEqual(args.samples, 3)
