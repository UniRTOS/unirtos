import re
import os
from typing import Match

class MarkdownToRST:
    """Markdown → RST 转换器"""

    def __init__(self, src_path=None, dst_path=None):
        """初始化转换器"""
        self.src_path = src_path  # 源文件路径
        self.dst_path = dst_path  # 目标文件路径

    def convert(self, text: str) -> str:
        """将 Markdown 文本转换为 reStructuredText (RST)"""
        # 首先在完整文本上处理代码块（因为代码块可能跨多行）
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
        def repl(m: Match) -> str:
            lang = m.group(1).strip() if m.group(1) else ""
            code = m.group(2)
            rst = f".. code-block:: {lang}\n\n"
            for line in code.splitlines():
                rst += f"   {line}\n"
            return rst + "\n"
        return re.sub(r"```(\w*)\n(.*?)```", repl, text, flags=re.DOTALL)

    def _convert_inline_code(self, text: str) -> str:
        return re.sub(r'`([^`]+)`', r'``\1``', text)

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

    # 以下是其他可能需要的转换方法的简化实现
    def _convert_headings(self, text: str) -> str:
        return text

    def _convert_images(self, text: str) -> str:
        return text

    def _convert_link_to_translation(self, text: str) -> str:
        return text

    def _convert_links(self, text: str) -> str:
        return text

    def _convert_bold(self, text: str) -> str:
        return text

    def _convert_italic(self, text: str) -> str:
        return text

    def _convert_unordered_list(self, text: str) -> str:
        return text

    def _convert_ordered_list(self, text: str) -> str:
        return text

# 测试代码块转换功能
def test_codeblock_conversion():
    # 创建测试文本
    test_text = """
# 代码块测试

这是一个普通代码块：
```
print("Hello, World!")
```

这是一个带语言标记的代码块：
```python
def hello():
    print("Hello, World!")
    return 0
```

这是一个跨多行的代码块：
```javascript
function complexFunction(param1, param2) {
    // 这是一个注释
    let result = param1 + param2;
    
    if (result > 10) {
        return "大于10";
    } else {
        return "小于等于10";
    }
}
```

这是行内代码 `example()` 和其他文本的混合。
    """
    
    # 创建转换器实例
    converter = MarkdownToRST()
    
    # 执行转换
    rst_text = converter.convert(test_text)
    
    # 打印转换结果
    print("=== 转换后的RST文本 ===")
    print(rst_text)
    
    # 验证代码块是否正确转换
    print("\n=== 验证结果 ===")
    
    # 检查普通代码块
    if ".. code-block::" in rst_text and "   print(\"Hello, World!\")" in rst_text:
        print("✓ 普通代码块转换成功")
    else:
        print("✗ 普通代码块转换失败")
    
    # 检查带语言标记的代码块
    if ".. code-block:: python" in rst_text and "   def hello():" in rst_text:
        print("✓ 带语言标记的代码块转换成功")
    else:
        print("✗ 带语言标记的代码块转换失败")
    
    # 检查跨多行的代码块
    if ".. code-block:: javascript" in rst_text and "   function complexFunction(param1, param2) {" in rst_text:
        print("✓ 跨多行代码块转换成功")
    else:
        print("✗ 跨多行代码块转换失败")

if __name__ == "__main__":
    test_codeblock_conversion()