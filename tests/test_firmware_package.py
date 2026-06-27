from __future__ import annotations

import pathlib
import sys
import tempfile
import unittest

PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(PROJECT_ROOT / "tools"))

from firmware_package import parse_dts_partitions


class FirmwarePackageTests(unittest.TestCase):
    def test_parse_dts_partition_label_ignores_trailing_comments(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            dts = pathlib.Path(tmpdir) / "zephyr.dts"
            dts.write_text(
                """
                / {
                    flash0: flash@0 {
                        partitions {
                            compatible = \"fixed-partitions\";
                            slot0_partition: partition@20000 {
                                label = \"slot0_partition\"; /* DTS generated comment */
                                reg = <0x20000 0x150000>;
                            };
                        };
                    };
                };
                """,
                encoding="utf-8",
            )

            partitions = parse_dts_partitions(dts)

        self.assertEqual(len(partitions), 1)
        self.assertEqual(partitions[0].node_label, "slot0_partition")
        self.assertEqual(partitions[0].label, "slot0_partition")
        self.assertEqual(partitions[0].offset, 0x20000)
        self.assertEqual(partitions[0].size, 0x150000)


if __name__ == "__main__":
    unittest.main()
