"""Small synthetic corrupt-input checks; no third-party game files needed."""
import hashlib
from pathlib import Path
import struct
import tempfile
import unittest

from pak_inspect import InspectError, inspect_pak


def fstring(value):
    data = value.encode() + b"\0"
    return struct.pack("<i", len(data)) + data


def fixture(name="Test.uasset", location=0, declared_count=1):
    directory = struct.pack("<i", 1) + fstring("") + struct.pack("<i", 1) + fstring(name) + struct.pack("<i", location)
    prefix = b"unused-payload-placeholder"
    fixed = fstring("../../../") + struct.pack("<iQ", declared_count, 0) + struct.pack("<i", 0) + struct.pack("<i", 1)
    tail = struct.pack("<i", 4) + b"\0" * 4 + struct.pack("<i", 0)
    main_size = len(fixed) + 36 + len(tail)
    main = fixed + struct.pack("<qq", len(prefix) + main_size, len(directory)) + hashlib.sha1(directory).digest() + tail
    footer = bytearray(221)
    struct.pack_into("<Iiqq", footer, 17, 0x5a6f12e1, 11, len(prefix), len(main))
    footer[41:61] = hashlib.sha1(main).digest()
    return bytearray(prefix + main + directory + footer), len(prefix)


class MetadataInspectionTests(unittest.TestCase):
    def inspect(self, data):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "fixture.pak"
            path.write_bytes(data)
            return inspect_pak(path)

    def test_valid_metadata_without_payload_parsing(self):
        summary, paths = self.inspect(fixture()[0])
        self.assertEqual(paths, ["Test.uasset"])
        self.assertEqual(summary["file_count"], 1)

    def test_rejects_encrypted_index(self):
        data, _ = fixture()
        data[-221 + 16] = 1
        with self.assertRaisesRegex(InspectError, "Encrypted index"):
            self.inspect(data)

    def test_rejects_unsupported_version(self):
        data, _ = fixture()
        struct.pack_into("<i", data, len(data) - 221 + 21, 10)
        with self.assertRaisesRegex(InspectError, "Unsupported PAK version"):
            self.inspect(data)

    def test_rejects_bad_index_checksum(self):
        data, index_offset = fixture()
        data[index_offset] ^= 1
        with self.assertRaisesRegex(InspectError, "SHA-1"):
            self.inspect(data)

    def test_rejects_index_outside_file(self):
        data, _ = fixture()
        struct.pack_into("<q", data, len(data) - 221 + 25, len(data))
        with self.assertRaisesRegex(InspectError, "Index range"):
            self.inspect(data)

    def test_rejects_inconsistent_count(self):
        with self.assertRaisesRegex(InspectError, "entry count"):
            self.inspect(fixture(declared_count=2)[0])

    def test_rejects_path_traversal(self):
        with self.assertRaisesRegex(InspectError, "unsafe path"):
            self.inspect(fixture(name="../outside.uasset")[0])

    def test_rejects_invalid_metadata_location(self):
        with self.assertRaisesRegex(InspectError, "outside encoded metadata"):
            self.inspect(fixture(location=100)[0])


if __name__ == "__main__":
    unittest.main()
