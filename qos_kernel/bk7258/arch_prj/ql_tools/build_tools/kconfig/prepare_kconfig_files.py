#!/usr/bin/env python
#
# SPDX-FileCopyrightText: 2019-2021 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0

from __future__ import print_function, unicode_literals

import argparse
import json
import sys
import os
from io import open
import re


def _prepare_source_files(env_dict, list_separator):
    """
    Prepares source files which are sourced from the main Kconfig because upstream kconfiglib doesn't support sourcing
    a file list. The inputs are the same environment variables which are used by kconfiglib:
        - COMPONENT_KCONFIGS,
        - COMPONENT_KCONFIGS_SOURCE_FILE,
        - COMPONENT_KCONFIGS_PROJBUILD,
        - COMPONENT_KCONFIGS_PROJBUILD_SOURCE_FILE.

    The outputs are written into files pointed by the value of
        - COMPONENT_KCONFIGS_SOURCE_FILE,
        - COMPONENT_KCONFIGS_PROJBUILD_SOURCE_FILE,

    After running this function, COMPONENT_KCONFIGS_SOURCE_FILE and COMPONENT_KCONFIGS_PROJBUILD_SOURCE_FILE will
    contain a list of source statements based on the content of COMPONENT_KCONFIGS and COMPONENT_KCONFIGS_PROJBUILD,
    respectively. For example, if COMPONENT_KCONFIGS="var1;var2;var3" and
    COMPONENT_KCONFIGS_SOURCE_FILE="/path/file.txt" then the content of file /path/file.txt will be:
        source "var1"
        source "var2"
        source "var3"

    The character used to delimit paths in COMPONENT_KCONFIGS* variables is set using --list-separator option.
    Space separated lists are currently only used by the documentation build system (esp-docs).
    """

    def _dequote(var):
        return var[1:-1] if len(var) > 0 and (var[0], var[-1]) == ('"',) * 2 else var

    def _write_source_file(config_var, config_file):
        dequoted_var = _dequote(config_var)
        if dequoted_var:
            new_content = '\n'.join(['source "{}"'.format(path) for path in dequoted_var.split(list_separator)])
        else:
            new_content = ''

        try:
            with open(config_file, 'r', encoding='utf-8') as f:
                old_content = f.read()
        except Exception:
            # File doesn't exist or other issue
            old_content = None
            # "None" ensures that it won't be equal to new_content when it is empty string because files need to be
            # created for empty environment variables as well

        if new_content != old_content:
            # write or rewrite file only if it is necessary
            with open(config_file, 'w', encoding='utf-8') as f:
                f.write(new_content)

    def _parse_kconfig_group(kconfig_path):
        """
        Parse KCONFIG_GROUP marker from Kconfig file.
        Supports both '# KCONFIG_GROUP:' and '#KCONFIG_GROUP:' formats.
        Returns group name if found, None otherwise.
        """
        try:
            with open(kconfig_path, 'r', encoding='utf-8') as f:
                # Read first 10 lines to find the marker
                for i, line in enumerate(f):
                    if i > 10:  # Only check first 10 lines
                        break
                    line = line.strip()
                    # Support both '# KCONFIG_GROUP:' and '#KCONFIG_GROUP:' formats
                    if line.startswith('#KCONFIG_GROUP:') or line.startswith('# KCONFIG_GROUP:'):
                        # Extract group name - handle both formats
                        if line.startswith('#KCONFIG_GROUP:'):
                            group = line.replace('#KCONFIG_GROUP:', '').strip()
                        else:
                            group = line.replace('# KCONFIG_GROUP:', '').strip()
                        return group
        except Exception:
            pass
        return None

    def _group_components_by_marker(kconfig_paths, list_separator):
        """
        Group Kconfig files by their KCONFIG_GROUP marker.
        Returns dict: {group_name: [kconfig_paths]}, list of ungrouped paths
        """
        groups = {}
        ungrouped = []
        
        for kconfig_path in kconfig_paths.split(list_separator):
            kconfig_path = kconfig_path.strip().strip('"')
            if not kconfig_path:
                continue
            
            group = _parse_kconfig_group(kconfig_path)
            if group:
                if group not in groups:
                    groups[group] = []
                groups[group].append(kconfig_path)
            else:
                ungrouped.append(kconfig_path)
        
        return groups, ungrouped

    def _generate_nested_menu_structure(content_lines, menu_tree, armino_ap_dir, indent_level=0):
        """
        Generate nested menu structure from menu tree.
        menu_tree structure: {
            'components': [list of kconfig paths],  # Components at this level
            'children': {                           # Child menus
                'MenuName': {menu_tree},
                ...
            }
        }
        """
        indent = '    ' * indent_level
        
        # First, add components at current level
        if menu_tree.get('components'):
            for kconfig_path in sorted(menu_tree['components']):
                # Convert absolute path to relative path using ARMINO_AP_DIR
                if armino_ap_dir and armino_ap_dir in kconfig_path:
                    rel_path = kconfig_path.replace(armino_ap_dir, '${ARMINO_AP_DIR}')
                else:
                    rel_path = kconfig_path
                content_lines.append('{}source "{}"'.format(indent, rel_path))
        
        # Then, add child menus
        if menu_tree.get('children'):
            for menu_name in sorted(menu_tree['children'].keys()):
                child_tree = menu_tree['children'][menu_name]
                content_lines.append('{}menu "{}"'.format(indent, menu_name))
                content_lines.append('')
                _generate_nested_menu_structure(content_lines, child_tree, armino_ap_dir, indent_level + 1)
                content_lines.append('')
                content_lines.append('{}endmenu'.format(indent))

    def _generate_group_kconfig_files(groups, armino_ap_dir, group_kconfigs_dir):
        """
        Generate group Kconfig files automatically.
        Supports multi-level grouping using '::' separator (e.g., "Demos::Peripheral::Touch").
        Returns list of generated file paths.
        """
        os.makedirs(group_kconfigs_dir, exist_ok=True)
        
        generated_files = []
        
        # Separate flat groups and nested groups
        flat_groups = {}
        nested_groups = {}
        
        for group_name, kconfig_paths in groups.items():
            if '::' in group_name:
                # Multi-level group
                menu_path = [part.strip() for part in group_name.split('::')]
                # Use the full path as key to avoid conflicts
                path_key = '::'.join(menu_path)
                if path_key not in nested_groups:
                    nested_groups[path_key] = {'path': menu_path, 'components': []}
                nested_groups[path_key]['components'].extend(kconfig_paths)
            else:
                # Flat group
                flat_groups[group_name] = kconfig_paths
        
        # Merge flat groups and nested groups by top-level menu name
        # This ensures that groups with the same top-level name (e.g., "Demos" and "Demos::Net")
        # are merged into a single file
        merged_top_level_groups = {}
        
        # Process flat groups - treat them as top-level groups
        for group_name in sorted(flat_groups.keys()):
            if group_name not in merged_top_level_groups:
                merged_top_level_groups[group_name] = {'components': [], 'children': {}}
            # Add flat group components directly to top level
            merged_top_level_groups[group_name]['components'].extend(flat_groups[group_name])
        
        # Process nested groups - merge by top-level menu
        for path_key, group_info in nested_groups.items():
            top_level = group_info['path'][0]
            if top_level not in merged_top_level_groups:
                merged_top_level_groups[top_level] = {'components': [], 'children': {}}
            # Build menu tree structure
            current = merged_top_level_groups[top_level]
            menu_path = group_info['path'][1:]  # Skip top level
            for menu_name in menu_path:
                if 'children' not in current:
                    current['children'] = {}
                if menu_name not in current['children']:
                    current['children'][menu_name] = {'components': [], 'children': {}}
                current = current['children'][menu_name]
            # Add components to leaf node
            if 'components' not in current:
                current['components'] = []
            current['components'].extend(group_info['components'])
        
        # Generate one file per top-level group (merged)
        for top_level in sorted(merged_top_level_groups.keys()):
            group_file = os.path.join(group_kconfigs_dir, '{}_group.kconfig'.format(top_level.lower().replace(' ', '_').replace('::', '_')))
            content_lines = []
            
            # Start with top-level menu
            content_lines.append('menu "{}"'.format(top_level))
            content_lines.append('')
            
            # Generate nested menu structure from merged tree
            menu_tree = merged_top_level_groups[top_level]
            _generate_nested_menu_structure(content_lines, menu_tree, armino_ap_dir, 1)
            
            content_lines.append('')
            content_lines.append('endmenu')
            
            with open(group_file, 'w', encoding='utf-8') as f:
                f.write('\n'.join(content_lines))
            
            generated_files.append(group_file)
        
        return generated_files

    def _generate_group_index_file(group_kconfigs_dir, group_files):
        """
        Generate index file that sources all group Kconfig files.
        Returns path to index file, or None if no groups.
        """
        os.makedirs(group_kconfigs_dir, exist_ok=True)
        index_file = os.path.join(group_kconfigs_dir, 'group_index.kconfig')
        
        if not group_files:
            # Create empty file if no groups
            with open(index_file, 'w', encoding='utf-8') as f:
                f.write('# No grouped components\n')
            return index_file
        
        content_lines = []
        # Ensure group_kconfigs_dir is absolute
        group_kconfigs_dir_abs = os.path.abspath(os.path.normpath(group_kconfigs_dir))
        # Use set to deduplicate file paths
        seen_files = set()
        for group_file in sorted(group_files):
            # Use absolute path for source statement
            # Ensure path is absolute and normalized
            if os.path.isabs(group_file):
                abs_path = os.path.normpath(group_file)
            else:
                abs_path = os.path.abspath(os.path.normpath(os.path.join(group_kconfigs_dir_abs, group_file)))
            # Deduplicate: only add if not seen before
            if abs_path not in seen_files:
                seen_files.add(abs_path)
                content_lines.append('source "{}"'.format(abs_path))
        
        with open(index_file, 'w', encoding='utf-8') as f:
            f.write('\n'.join(content_lines))
        
        return index_file

    def _write_specific_source_file(config_file_in, config_file_out, pattern, exclude_paths=None):
        """
        Write source file with pattern matching and optional path exclusion.
        
        Args:
            config_file_in: Input file path
            config_file_out: Output file path
            pattern: Regex pattern to match lines
            exclude_paths: Optional set of paths to exclude
        """
        print(config_file_in)
        print(config_file_out)
        try:
            with open(config_file_in, 'r', encoding='utf-8') as f:
                content_lines = f.readlines()
                # Filter by pattern
                content_lines = [content_line for content_line in content_lines 
                               if re.search(pattern, content_line)]
                # Apply exclude filter if provided
                if exclude_paths:
                    filtered_lines = []
                    for line in content_lines:
                        # Extract path from source statement: source "path"
                        match = re.search(r'source\s+"([^"]+)"', line)
                        if match:
                            path = match.group(1)
                            if path not in exclude_paths:
                                filtered_lines.append(line)
                        else:
                            filtered_lines.append(line)
                    content_lines = filtered_lines
        except Exception:
            content_lines = []
        with open(config_file_out, 'w', encoding='utf-8') as f:
            for content_line in content_lines:
                f.write(content_line)
    
    try:
        # First, write the base source files
        _write_source_file(env_dict['COMPONENT_KCONFIGS'], env_dict['COMPONENT_KCONFIGS_SOURCE_FILE'])
        _write_source_file(env_dict['COMPONENT_KCONFIGS_PROJBUILD'], env_dict['COMPONENT_KCONFIGS_PROJBUILD_SOURCE_FILE'])
        
        # Parse component groups from KCONFIG_GROUP markers
        armino_path = env_dict.get('ARMINO_PATH', '')
        armino_ap_dir = os.path.join(armino_path, 'ap') if armino_path else ''
        
        # Get build directory from components_kconfigs_path
        components_kconfigs_path = env_dict.get('COMPONENTS_KCONFIGS_SOURCE_FILE', '')
        if components_kconfigs_path:
            build_dir = os.path.dirname(components_kconfigs_path)
            group_kconfigs_dir = os.path.join(build_dir, 'group_kconfigs')
        else:
            group_kconfigs_dir = os.path.join(os.getcwd(), 'group_kconfigs')
        
        # Group components by KCONFIG_GROUP marker
        component_kconfigs = env_dict.get('COMPONENT_KCONFIGS', '')
        grouped_paths = set()
        group_files = []
        
        if component_kconfigs:
            groups, ungrouped = _group_components_by_marker(component_kconfigs, list_separator)
            
            # Generate group Kconfig files if there are any groups
            if groups:
                group_files = _generate_group_kconfig_files(groups, armino_ap_dir, group_kconfigs_dir)
                # Collect all grouped paths for exclusion
                for group_paths in groups.values():
                    grouped_paths.update(group_paths)
        
        # Always generate index file (even if empty)
        index_file = _generate_group_index_file(group_kconfigs_dir, group_files)
        # Store index file path in environment for later use
        env_dict['GROUP_KCONFIGS_INDEX_FILE'] = index_file
        
        # Write components_kconfigs.in with grouped components excluded
        _write_specific_source_file(
            env_dict['COMPONENT_KCONFIGS_SOURCE_FILE'], 
            env_dict['COMPONENTS_KCONFIGS_SOURCE_FILE'], 
            r'.*/components/.*',
            exclude_paths=grouped_paths if grouped_paths else None
        )
        
        # Write other source files (middleware, projects, properties, extra)
        _write_specific_source_file(env_dict['COMPONENT_KCONFIGS_SOURCE_FILE'], env_dict['MIDDLEWARE_KCONFIGS_SOURCE_FILE'], r'.*/middleware/.*')
        _write_specific_source_file(env_dict['COMPONENT_KCONFIGS_SOURCE_FILE'], env_dict['PROJECTS_KCONFIGS_SOURCE_FILE'], r'.*/projects/.*')
        _write_specific_source_file(env_dict['COMPONENT_KCONFIGS_SOURCE_FILE'], env_dict['PROPERTIES_KCONFIGS_SOURCE_FILE'], r'.*/properties/.*')
        _write_specific_source_file(env_dict['COMPONENT_KCONFIGS_SOURCE_FILE'], env_dict['EXTRA_KCONFIGS_SOURCE_FILE'], r'^(?!.*\/(?:components|middleware|projects|properties)\/).*')
    except KeyError as e:
        print('Error:', e, 'is not defined!')
        sys.exit(1)


def main():
    parser = argparse.ArgumentParser(description='Kconfig Source File Generator')

    parser.add_argument('--env', action='append', default=[],
                        help='Environment value', metavar='NAME=VAL')

    parser.add_argument('--env-file', type=argparse.FileType('r'),
                        help='Optional file to load environment variables from. Contents '
                             'should be a JSON object where each key/value pair is a variable.')

    parser.add_argument('--list-separator', choices=['space', 'semicolon'],
                        default='space',
                        help='Separator used in environment list variables (COMPONENT_KCONFIGS, COMPONENT_KCONFIGS_PROJBUILD)')

    args = parser.parse_args()

    try:
        env = dict([(name, value) for (name, value) in (e.split('=', 1) for e in args.env)])
    except ValueError:
        print('--env arguments must each contain =.')
        sys.exit(1)

    if args.env_file is not None:
        env.update(json.load(args.env_file))

    list_separator = ';' if args.list_separator == 'semicolon' else ' '

    _prepare_source_files(env, list_separator)


if __name__ == '__main__':
    main()
