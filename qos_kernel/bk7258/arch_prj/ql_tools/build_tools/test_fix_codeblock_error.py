#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
测试脚本：验证MarkdownToRST类修复后的代码块转换功能
"""
import sys
import os
import re
from typing import Match

class MarkdownToRST:
    """Markdown → RST 转换器（包含修复后的代码）"""

    def __init__(self, src_path=None, dst_path=None):
        """初始化转换器"""
        self.src_path = src_path  # 源文件路径
        self.dst_path = dst_path  # 目标文件路径

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

    def _convert_inline_code(self, text: str) -> str:
        return re.sub(r'`([^`]+)`', r'``\1``', text)

    def _convert_unordered_list(self, text: str) -> str:
        return re.sub(r'^[-\*]\s+', r'* ', text, flags=re.MULTILINE)

    def _convert_ordered_list(self, text: str) -> str:
        # 保持原始数字格式，不转换为#.格式
        return text

    def _convert_link_to_translation(self, text: str) -> str:
        # 确保使用正确的反引号格式，避免出现双反引号
        # 匹配无序列表中的语言链接
        text = re.sub(r'^\s*[*-]\s+\[English\]\(\.\/README\.md\)', r':link_to_translation:`en:[English]`', text, flags=re.MULTILINE)
        text = re.sub(r'^\s*[*-]\s+\[中文\]\(\.\/README_CN\.md\)', r':link_to_translation:`zh_CN:[中文]`', text, flags=re.MULTILINE)
        # 匹配普通文本中的语言链接
        text = re.sub(r'\[English\]\(\.\/README\.md\)', r':link_to_translation:`en:[English]`', text)
        text = re.sub(r'\[中文\]\(\.\/README_CN\.md\)', r':link_to_translation:`zh_CN:[中文]`', text)
        return text

def test_codeblock_conversion():
    """测试代码块转换功能"""
    print("开始测试代码块转换功能...")
    
    # 测试用例1: 模拟voice_service_example README中的有问题内容
    test_content_1 = """用户1553302592

复制

删除

voice_service_example README_ref.md 目录下的 voice_service_example 工程是用于测试 bk_voice_service 组件功能的专用项目。"""
    
    # 测试用例2: 带语言标记的代码块
    test_content_2 = """使用以下命令编译项目：
```bash
make bk7258 PROJECT=voice_service_example
```"""
    
    # 测试用例3: 跨多行的代码块
    test_content_3 = """目录结构：
```
voice_service_example/
├── ap/
│   ├── ap_main.c                   # 主入口文件
│   └── voice_service_test/
└── README.md                       # 项目说明文档
```"""
    
    # 创建转换器实例
    converter = MarkdownToRST()
    
    # 执行转换
    result_1 = converter.convert(test_content_1)
    result_2 = converter.convert(test_content_2)
    result_3 = converter.convert(test_content_3)
    
    # 验证结果
    print("\n测试用例1 (清理编辑器标记):")
    print("输入:", test_content_1)
    print("输出:", result_1)
    assert "用户1553302592" not in result_1, "编辑器标记未被正确清理"
    assert "复制" not in result_1, "编辑器标记未被正确清理"
    assert "删除" not in result_1, "编辑器标记未被正确清理"
    
    print("\n测试用例2 (带语言标记的代码块):")
    print("输入:", test_content_2)
    print("输出:", result_2)
    assert ".. code-block:: bash" in result_2, "代码块语言标记未被正确转换"
    assert "   make bk7258 PROJECT=voice_service_example" in result_2, "代码内容未被正确缩进"
    
    print("\n测试用例3 (跨多行代码块):")
    print("输入:", test_content_3)
    print("输出:", result_3)
    assert ".. code-block:: " in result_3, "多行代码块未被正确转换"
    assert "   voice_service_example/" in result_3, "多行代码内容未被正确缩进"
    
    print("\n所有测试用例通过！代码块转换功能修复成功。")

if __name__ == "__main__":
    test_codeblock_conversion()