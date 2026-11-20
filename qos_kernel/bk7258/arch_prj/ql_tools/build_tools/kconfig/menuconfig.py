# SPDX-FileCopyrightText: 2025 BEKEN
# SPDX-License-Identifier: Apache-2.0
import sys
import os

sys.path.insert(0, os.path.dirname(__file__))

from menuconfig.core import _main


class FatalError(RuntimeError):
    """
    Class for runtime errors (not caused by bugs but by user input).
    """

    pass


if __name__ == "__main__":
    _main()
    # try:
    #     _main()
    # except FatalError as e:
    #     print(f"A fatal error occurred: {e}", file=sys.stderr)
    #     sys.exit(2)
