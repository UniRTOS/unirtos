#!/usr/bin/env python3

import argparse
import sys
from pathlib import Path

import kconfiglib

from check_config_cap import NOT_SET_RE, SET_RE, parse_config

DESCRIPTION = """
Generates a CMake file with defines from the configuration.
"""


def select_platform(kconf, capability_config):
    """Select the module platform without changing the existing Kconfig rules."""
    choices = [
        choice for choice in kconf.unique_choices
        if any(sym.name == "QOSA_EIGEN718_PLATFORM_FUNC" for sym in choice.syms)
    ]
    if len(choices) != 1:
        raise ValueError("Expected exactly one platform choice in Kconfig")
    platform_choice = choices[0]
    capabilities = parse_config(capability_config)
    selected = [
        sym for sym in platform_choice.syms
        if capabilities.get("CONFIG_" + sym.name, {}).get("value") == "y"
    ]
    if len(selected) != 1:
        raise ValueError(
            "Expected exactly one platform in " + str(capability_config)
            + "; found: " + str([sym.name for sym in selected])
        )
    selected[0].set_value("y")
    return platform_choice


def resolve_platform_config(kconf, capability_config, output_config):
    """Apply platform/AT dependency results to a build copy, not the app config."""
    platform_choice = select_platform(kconf, capability_config)
    at_names = {"QOSA_AT_PARSER_FUNC", "QOSA_AT_QGMR_APP", "QAPP_UNIRTOS_STD_FUNC"}
    at_names.update(
        sym.name for sym in kconf.unique_defined_syms
        if any(
            node.filename.replace("\\", "/").endswith("qos_applications/unirtos_std/Kconfig")
            for node in sym.nodes
        )
    )
    resolved = set(platform_choice.syms) | {kconf.syms[name] for name in at_names}
    overrides = {"CONFIG_" + sym.name: sym for sym in resolved}
    path = Path(output_config)
    requested = parse_config(path)
    retained = []
    for line in path.read_text(encoding="utf-8").splitlines():
        match = SET_RE.match(line.strip()) or NOT_SET_RE.match(line.strip())
        if not match or match.group(1) not in overrides:
            retained.append(line)

    # Do not write the entire Kconfig result: unrelated invalid requests must
    # remain visible to the capability checker rather than being silently lost.
    retained.append("# Platform and AT dependency results (build copy only)")
    for name, sym in sorted(overrides.items()):
        config_line = sym.config_string.rstrip("\n")
        if config_line:
            retained.append(config_line)
        elif sym.type in (kconfiglib.BOOL, kconfiglib.TRISTATE):
            retained.append("# " + name + " is not set")
        if requested.get(name, {}).get("value") == "y" and sym.str_value == "n":
            print("[cmakeconfig] " + name + " disabled by Kconfig dependencies")
    path.write_text("\n".join(retained) + "\n", encoding="utf-8")
    print("[cmakeconfig] platform: " + platform_choice.selection.name)


def main():
    parser = argparse.ArgumentParser(description=DESCRIPTION)

    parser.add_argument(
        "--kconfig",
        metavar="KCONFIG_FILENAME",
        dest="kconfig",
        default="Kconfig",
        help="top-level Kconfig file (default: Kconfig)")

    parser.add_argument(
        "config",
        metavar="CONFIG_FILENAME",
        help="configuration file name")

    parser.add_argument(
        "cmake",
        metavar="OUTPUT_FILENAME",
        help="generated CMake file name")

    parser.add_argument(
        "--capability-config",
        help="Module capability config used to select the actual platform")
    parser.add_argument(
        "--resolved-config",
        help="Build config copy to update with platform/AT dependency results")

    args = parser.parse_args()
    if bool(args.capability_config) != bool(args.resolved_config):
        parser.error("--capability-config and --resolved-config must be used together")

    kconf = kconfiglib.Kconfig(args.kconfig)

    kconf.load_config(args.config)

    if args.capability_config:
        resolve_platform_config(kconf, args.capability_config, args.resolved_config)

    kconf.write_cmakeconf(args.cmake)

if __name__ == "__main__":
    main()
