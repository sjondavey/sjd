"""Write sjd/_core.pyi into the staged package.

The build directory is not the working directory: a binary named sjd there
would be imported instead of the package. Enum defaults in the pybind11
docstrings are rewritten to sjd._core. The two spot-path methods return the
Simulation type, which lives in an anonymous C++ namespace and is not a
valid annotation until it is replaced.
"""

import os
import re
import subprocess
import sys
from pathlib import Path


def main() -> None:
    stage = Path(sys.argv[1])
    root = Path(sys.argv[2])
    env = os.environ.copy()
    env["PYTHONPATH"] = str(stage)
    env["SJD_NO_DEBUG_DIALOGS"] = "1"
    subprocess.run(
        [
            sys.executable,
            "-m",
            "pybind11_stubgen",
            "sjd._core",
            "-o",
            str(stage),
            "--exit-code",
            "--enum-class-locations",
            ".*:sjd._core",
            "--ignore-invalid-expressions",
            ".*anonymous namespace.*",
        ],
        cwd=root,
        env=env,
        check=True,
    )
    stub = stage / "sjd" / "_core.pyi"
    text = stub.read_text(encoding="utf-8").replace("-> ...:", "-> Simulation:")
    # pybind11 records an optional argument as `T = None`. mypy reads that as a
    # non-optional T unless the annotation itself includes None.
    text = re.sub(r": ([A-Za-z_][\w.]*) = None\b", r": \1 | None = None", text)
    stub.write_text(text, encoding="utf-8", newline="\n")


if __name__ == "__main__":
    main()
