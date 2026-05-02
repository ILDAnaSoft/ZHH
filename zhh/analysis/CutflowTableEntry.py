from dataclasses import dataclass

@dataclass
class CutflowTableEntry:
    label: str

@dataclass
class LatexCutflowTableEntry:
    latex: str

@dataclass
class CategorizedCutflowTableEntry(CutflowTableEntry):
    label: str
    category: str
    is_signal: bool = False

@dataclass
class UncategorizedCutflowTableEntry(CutflowTableEntry):
    label: str
    source: str

@dataclass
class SumCutflowTableEntry(CutflowTableEntry):
    label: str
    sum: str|list[str]
    source: str