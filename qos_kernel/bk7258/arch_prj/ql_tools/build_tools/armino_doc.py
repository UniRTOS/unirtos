#!/usr/bin/env python3

import os
import subprocess
import sys
import re
from typing import Match

class MarkdownToRST:
    """Markdown → RST 转换器"""

    def __init__(self, src_path=None, dst_path=None):
        """初始化转换器"""
        self.src_path = src_path  # 源文件路径
        self.dst_path = dst_path  # 目标文件路径

    def convert_file(self, src_file, dst_file):
        """将指定的Markdown文件转换为RST文件"""
        # 构建完整的文件路径
        src_file_path = os.path.join(self.src_path, src_file) if self.src_path else src_file
        dst_file_path = os.path.join(self.dst_path, dst_file) if self.dst_path else dst_file

        # 确保目标目录存在
        if self.dst_path:
            os.makedirs(self.dst_path, exist_ok=True)

        # 读取源文件
        with open(src_file_path, "r", encoding="utf-8") as f:
            md_text = f.read()

        # 转换内容
        rst_text = self.convert(md_text)

        # 写入目标文件
        with open(dst_file_path, "w", encoding="utf-8") as f:
            f.write(rst_text)

    def convert(self, text: str) -> str:
        """将 Markdown 文本转换为 reStructuredText (RST)"""
        # 首先清理文本，移除可能导致问题的内容
        text = self._clean_text_before_conversion(text)

        # 然后在完整文本上处理代码块（因为代码块可能跨多行）
        text = self._convert_codeblocks(text)

        # 然后按行处理其他格式
        lines = text.splitlines()
        processed_lines = []

        for line in lines:
            # 检查是否已经包含reST语法
            if self._contains_rst_syntax(line):
                # 如果已经包含reST语法，则不进行转换，直接添加到结果中
                processed_lines.append(line)
            else:
                # 否则进行正常的Markdown到reST转换
                processed_line = line
                # 处理行内代码
                processed_line = self._convert_inline_code(processed_line)
                # 然后处理其他格式
                processed_line = self._convert_headings(processed_line)
                processed_line = self._convert_images(processed_line)
                processed_line = self._convert_link_to_translation(processed_line)
                processed_line = self._convert_links(processed_line)
                processed_line = self._convert_bold(processed_line)
                processed_line = self._convert_italic(processed_line)
                processed_line = self._convert_unordered_list(processed_line)
                processed_line = self._convert_ordered_list(processed_line)
                # 最后处理语言链接转换，避免被行内代码转换影响
                processed_lines.append(processed_line)

        # 将处理后的行重新组合为文本
        return '\n'.join(processed_lines)

    def _convert_link_to_translation(self, text: str) -> str:
        # 确保使用正确的反引号格式，避免出现双反引号
        # 匹配无序列表中的语言链接
        text = re.sub(r'^\s*[*-]\s+\[English\]\(\.\/README\.md\)', r':link_to_translation:`en:[English]`', text, flags=re.MULTILINE)
        text = re.sub(r'^\s*[*-]\s+\[中文\]\(\.\/README_CN\.md\)', r':link_to_translation:`zh_CN:[中文]`', text, flags=re.MULTILINE)
        # 匹配普通文本中的语言链接
        text = re.sub(r'\[English\]\(\.\/README\.md\)', r':link_to_translation:`en:[English]`', text)
        text = re.sub(r'\[中文\]\(\.\/README_CN\.md\)', r':link_to_translation:`zh_CN:[中文]`', text)
        return text


    def _contains_rst_syntax(self, line: str) -> bool:
        """检查一行文本是否已经包含reST语法"""
        # 检查常见的reST语法模式
        patterns = [
            # 检查以..开头的指令（如.. image::, .. code-block::等）
            r'^\s*\.\.',
            # 检查以:开头的指令（如:link_to_translation:）
            r'^\s*:',
            # 检查rst链接格式 `link text <url>`_
            r'`[^`]+ <[^>]+>`_',
            # 检查行内代码 ``code``
            r'``[^`]+``',
            # 检查toctree指令
            r'\.\.\s+toctree\s*::',
            # 检查note等提示框指令
            r'\.\.\s+(note|warning|tip|important|caution)\s*::'
        ]

        # 如果包含任何一个reST语法模式，则返回True
        for pattern in patterns:
            if re.search(pattern, line):
                return True

        return False

    def _convert_headings(self, text: str) -> str:
        def repl(m: Match) -> str:
            level = len(m.group(1))
            title = m.group(2).strip()
            if level == 1:
                underline = "=" * len(title) * 3
            elif level == 2:
                underline = "-" * len(title) * 3
            elif level == 3:
                underline = "," * len(title) * 3
            elif level == 4:
                underline = "." * len(title) * 3
            elif level == 5:
                underline = "*" * len(title) * 3
            else:
                underline = "~" * len(title) * 3
            return f"{title}\n{underline}\n"
        # 先处理标准的# 标题格式
        text = re.sub(r"^(#{1,6})\s+(.*)$", repl, text, flags=re.MULTILINE)
        # 再处理特殊的#. 标题格式
        text = re.sub(r"^(#)\.\s+(.*)$", repl, text, flags=re.MULTILINE)
        return text

    def _convert_images(self, text: str) -> str:
        def repl(match):
            alt_text = match.group(1)
            image_path = match.group(2)

            # 如果指定了源路径和目标路径，则调整图片相对路径
            if self.src_path and self.dst_path:
                # 判断是否为绝对路径
                if not os.path.isabs(image_path):
                    # 构建图片的绝对路径
                    absolute_image_path = os.path.join(self.src_path, image_path)
                    # 计算相对于目标路径的相对路径
                    relative_image_path = os.path.relpath(absolute_image_path, self.dst_path)
                    # 确保路径使用正斜杠（RST标准）
                    image_path = relative_image_path.replace(os.sep, '/')

            return f".. image:: {image_path}\n   :alt: {alt_text}"

        return re.sub(r'!\[(.*?)\]\((.*?)\)', repl, text)

    def _convert_links(self, text: str) -> str:
        return re.sub(r'\[(.*?)\]\((.*?)\)', r'`\1 <\2>`_', text)

    def _convert_bold(self, text: str) -> str:
        return re.sub(r'(\*\*|__)(.*?)\1', r'**\2**', text)

    def _convert_italic(self, text: str) -> str:
        """
        *italic* 或 _italic_ -> *italic*
        避免误伤普通下划线，如 link_to
        """
        # 只匹配被空格或行首行尾包围的 _xxx_
        text = re.sub(r'(?<!\w)\*(?!\*)(.+?)(?<!\*)\*(?!\w)', r'*\1*', text)
        text = re.sub(r'(?<!\w)_(?!_)(.+?)(?<!_)_(?!\w)', r'*\1*', text)
        return text

    def _convert_codeblocks(self, text: str) -> str:
        """转换Markdown代码块为reST代码块"""
        def repl(m: Match) -> str:
            lang = m.group(1).strip() if m.group(1) else ""
            code = m.group(2)
            rst = f".. code-block:: {lang}\n\n"
            for line in code.splitlines():
                # 确保代码行正确缩进，避免reST语法错误
                rst += f"   {line}\n"
            return rst + "\n"

        # 使用更健壮的正则表达式匹配代码块
        # 使用DOTALL标志使.匹配换行符，使用贪婪模式确保完整匹配代码块
        return re.sub(r"```(\w*)\n(.*?)```", repl, text, flags=re.DOTALL)

    def _clean_text_before_conversion(self, text: str) -> str:
        """在转换前清理文本，移除可能导致问题的内容"""
        # 移除编辑器相关的标记或注释
        text = re.sub(r'用户\d+\s+复制\s+删除\s+', '', text)
        # 移除多余的空行
        text = re.sub(r'\n{3,}', '\n\n', text)
        return text

    def _convert_inline_code(self, text: str) -> str:
        return re.sub(r'`([^`]+)`', r'``\1``', text)

    def _convert_unordered_list(self, text: str) -> str:
        return re.sub(r'^[\-\*]\s+', r'* ', text, flags=re.MULTILINE)

    def _convert_ordered_list(self, text: str) -> str:
        # 保持原始数字格式，不转换为#.格式
        return text

def translate_md2rst(src_path, dst_path, lan):
    src_file = ""
    dst_file = ""

    if lan == 'en':
        src_file = "README.md"
        dst_file = "index.rst"

    elif lan == 'zh_CN':
        src_file = "README_CN.md"
        dst_file = "index.rst"
    else:
        return

    # 检查源文件是否存在
    if not os.path.isfile(os.path.join(src_path, src_file)):
        return

    # 使用类的方法进行文件转换
    converter = MarkdownToRST(src_path, dst_path)
    converter.convert_file(src_file, dst_file)

def run_cmd(cmd):
    p = subprocess.Popen(cmd, shell=True)
    ret = p.wait()
    return p

def copy_projects_doc(src_path, dst_path, lan):
    print(f"copy_projects_doc: {src_path} -> {dst_path}")
    if not os.path.isdir(src_path):
        return 0

    has_doc = False
    has_cmakelist = 0

    # 检查当前文件夹是否为projects，如果是则设置has_doc = True
    if os.path.basename(src_path) == 'projects':
        has_doc = True

    for item in os.listdir(src_path):
        item_path = os.path.join(src_path, item)
        if os.path.isfile(item_path):
            if item == "README.md" and lan == 'en':
                has_doc = True
            elif item == "README_CN.md" and lan == 'zh_CN':
                has_doc = True
            elif item == "projects.rst":
                has_doc = True

            if item == "CMakeLists.txt":
                has_cmakelist = 1

    if has_doc == False:
        return 0

    run_cmd(f'mkdir -p {dst_path}')
    translate_md2rst(src_path, dst_path, lan)

    if has_cmakelist == 1:
        return 1

    for item in os.listdir(src_path):
        item_path = os.path.join(src_path, item)
        item_dst_path = os.path.join(dst_path, item)
        if os.path.isdir(item_path):
            has_cmakelist = has_cmakelist + copy_projects_doc(item_path, item_dst_path, lan)

    if (has_cmakelist == 0):
        run_cmd(f'rm -rf {dst_path}')

    return has_cmakelist

def build_lan_doc(doc_path, target, lan):
    # 无论路径是否包含ap/docs，都确保lan_dir被定义
    lan_dir = f'{doc_path}/{lan}'

    if "ap/docs" in doc_path:
        print("cp/docs not support")
        print(f"doc_path: {doc_path}")
        print(f"target: {target}")
        print(f"lan: {lan}")
        armino_path = os.getenv('ARMINO_PATH')
        print(f"armino_path: {armino_path}")
        print(f"lan_dir: {lan_dir}")
        if (target == 'bk7236' or target == 'bk7258'):
            copy_projects_doc(f'{lan_dir}/../../../../projects', f'{lan_dir}/examples/projects', lan)

    os.chdir(lan_dir)

    # clean build space
    run_cmd(f'rm -rf {doc_path}/{lan}/_build')
    run_cmd(f'rm -rf {doc_path}/{lan}/xml')
    run_cmd(f'rm -rf {doc_path}/{lan}/xml_in')
    run_cmd(f'rm -rf {doc_path}/{lan}/man')
    run_cmd(f'rm -rf {doc_path}/{lan}/__pycache__')

    p = run_cmd(f'make arminodocs -j32')
    if p.returncode:
        print("make doc failed!")
        exit(1)
    run_cmd(f'mkdir -p ../build/{lan}')
    run_cmd(f'cp -r _build/* ../build/{lan}')
    os.chdir(doc_path)

def build_with_target(clean, target, doc_build_path):
    cur_dir_is_docs_dir = True
    saved_dir = os.getcwd()
    if 'ARMINO_PATH' in os.environ:
        armino_path = os.getenv('ARMINO_PATH')
        DOCS_PATH = f"{armino_path}/docs/{target}"
        cur_path = os.getcwd()
        if cur_path != DOCS_PATH:
            cur_dir_is_docs_dir = False
        print(f'DOCS_PATH set to {DOCS_PATH}')
    else:
        #print('ARMINO_PATH env is not set, set DOCS_PATH to current dir')
        DOCS_PATH = f"{os.getcwd()}/docs/{target}"

    build_dir = doc_build_path
    if (clean):
        run_cmd(f'rm -rf {build_dir}')
        run_cmd(f'rm -rf {DOCS_PATH}/en/_build')
        run_cmd(f'rm -rf {DOCS_PATH}/en/xml')
        run_cmd(f'rm -rf {DOCS_PATH}/en/xml_in')
        run_cmd(f'rm -rf {DOCS_PATH}/en/man')
        run_cmd(f'rm -rf {DOCS_PATH}/zh_CN/_build')
        run_cmd(f'rm -rf {DOCS_PATH}/zh_CN/xml')
        run_cmd(f'rm -rf {DOCS_PATH}/zh_CN/xml_in')
        run_cmd(f'rm -rf {DOCS_PATH}/zh_CN/man')
        run_cmd(f'rm -rf {DOCS_PATH}/__pycache__')
        if (target == 'bk7236' or target == 'bk7258'):
            run_cmd(f'rm -rf {DOCS_PATH}/en/projects')
            run_cmd(f'rm -rf {DOCS_PATH}/zh_CN/projects')
        return

    if not os.path.exists(build_dir):
        run_cmd(f'mkdir -p {build_dir}')

    build_lan_doc(DOCS_PATH, target, 'zh_CN')
    build_lan_doc(DOCS_PATH, target, 'en')

    if cur_dir_is_docs_dir == False:
        run_cmd(f'rm -rf {build_dir}/{target}')
        run_cmd(f'cp -rf {DOCS_PATH}/build/ {build_dir}/{target}')
        run_cmd(f'rm -rf {build_dir}/{target}/*/inc')

    os.chdir(saved_dir)

def build_doc_internal(clean, target):
    if 'ARMINO_AVDK_DIR' in os.environ:
        sdk_path = os.getenv('ARMINO_AVDK_DIR')
    else:
        raise RuntimeError("not find env ARMINO_AVDK_DIR")

    if 'ARMINO_DIR' in os.environ:
        armino_path = os.getenv('ARMINO_DIR')
    else:
        raise RuntimeError("not find env ARMINO_DIR")

    sub_doc_name = os.path.basename(os.getcwd())
    doc_build_path = sdk_path + f"/build/doc/{sub_doc_name}_doc"
    if not os.path.exists(doc_build_path):
        os.makedirs(doc_build_path)

    if (target == "all"):
        build_with_target(clean, "bk7258", doc_build_path)
    else:
        build_with_target(clean, target, doc_build_path)

    run_cmd(f'cp {armino_path}/docs/version.json {doc_build_path}/version.json')

def build_doc(target):
    build_doc_internal(False, target)

def main(argv):
    if (len(argv) > 1 and argv[1] == "clean"):
        target = "all"
        if (len(argv) == 3):
            target = sys.argv[2]
        build_doc_internal(True, target)
    else:
        build_doc_internal(False, "all")

if __name__ == "__main__":
    main(sys.argv)
