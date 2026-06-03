from pathlib import Path

Import("env")

project_dir = Path(env.subst("$PROJECT_DIR"))
counter_path = project_dir / "build_counter.txt"
header_path = project_dir / "include" / "build_version.h"

try:
    build_number = int(counter_path.read_text(encoding="utf-8").strip() or "0")
except ValueError:
    build_number = 1

header_path.write_text(
    "#pragma once\n\n"
    f"#define FIRMWARE_VERSION \"beta4.{build_number:02d}\"\n",
    encoding="utf-8",
)

print(f"ChargeScreen firmware version: beta4.{build_number:02d}")


def advance_build_counter(source, target, env):
    counter_path.write_text(f"{build_number + 1}\n", encoding="utf-8")


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", advance_build_counter)
