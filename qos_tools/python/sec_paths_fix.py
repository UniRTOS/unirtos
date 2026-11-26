## Dynamically update the binfile/binheader paths in qosa_secureboot_cfg_ec718pm.json
import json
import sys
import os

def normalize_path(path):
    return path.strip().replace('\\', '/')


def derive_repo_root(input_json):
    normalized = normalize_path(input_json)
    marker = "/qos_kernel/"
    index = normalized.find(marker)
    if index == -1:
        return None
    return normalized[:index]


def canonicalize_path(path):
    normalized = normalize_path(path)
    normalized = normalized.lstrip('/')

    anchored_prefixes = [
        ("qos_build/gccout/", "gccout/"),
        ("qos_build/build/bin/", "bin/"),
        ("qos_tools/eigen_718/", "eigen_718/"),
    ]

    direct_prefixes = ("gccout/", "bin/", "eigen_718/")
    candidates = [normalized]

    colon_index = normalized.find(':')
    if colon_index != -1 and colon_index + 1 < len(normalized):
        candidates.append(normalized[colon_index + 1:].lstrip('/'))

    for candidate in candidates:
        for source_prefix, target_prefix in anchored_prefixes:
            index = candidate.find(source_prefix)
            if index != -1:
                suffix = candidate[index + len(source_prefix):]
                return target_prefix + suffix

        for prefix in direct_prefixes:
            index = candidate.find(prefix)
            if index != -1:
                return candidate[index:]

    return None


def fix_path(path, unirtos_build_out, binary_bin_dir, repo_root):
    normalized = normalize_path(path)
    canonical = canonicalize_path(normalized)
    build_out_root = normalize_path(unirtos_build_out).rstrip('/\\')
    binary_root = normalize_path(binary_bin_dir).rstrip('/\\')
    repo_root_normalized = normalize_path(repo_root).rstrip('/\\') if repo_root else None

    if canonical is None:
        return normalized

    if canonical.startswith("gccout/"):
        subpath = canonical[len("gccout/"):]
        return f"{build_out_root}/gccout/{subpath}"
    if canonical.startswith("bin/"):
        subpath = canonical[len("bin/"):]
        return f"{binary_root}/{subpath}"
    if canonical.startswith("eigen_718/") and repo_root_normalized:
        return f"{repo_root_normalized}/qos_tools/{canonical}"

    return normalized

if __name__ == "__main__":
    input_json = sys.argv[1]
    output_json = sys.argv[2]
    unirtos_build_out = sys.argv[3]
    binary_bin_dir = sys.argv[4]
    repo_root = derive_repo_root(input_json)

    try:
        with open(input_json, 'r') as f:
            data = json.load(f)
    except Exception as e:
        print("ERROR: Failed to read input_json:", e)
        exit(1)

    for item in data.get("AdditionalBin", []):
        if "binfile" in item:
            print("  binfile before:", item["binfile"])
            item["binfile"] = fix_path(item["binfile"], unirtos_build_out, binary_bin_dir, repo_root)
            print("  binfile after:", item["binfile"])
        if "binheader" in item:
            print("  binheader before:", item["binheader"])
            item["binheader"] = fix_path(item["binheader"], unirtos_build_out, binary_bin_dir, repo_root)
            print("  binheader after:", item["binheader"])

    with open(output_json, 'w', encoding='utf-8') as f:
        json.dump(data, f, indent=4, ensure_ascii=False)