# -*- coding: utf-8 -*-
"""Self-check dense sidebar metrics."""
from pathlib import Path
import re
import sys

ui = Path(r"e:\My_work\Commd\CommunicationAssistant\ui\MainWindow.ui").read_text(encoding="utf-8")
cpp = Path(r"e:\My_work\Commd\CommunicationAssistant\src\MainWindow.cpp").read_text(encoding="utf-8")
fails = []

def must(cond, msg):
    if not cond:
        fails.append(msg)

m = re.search(r'name="sideScroll".*?<width>(\d+)</width>.*?<width>(\d+)</width>', ui, re.S)
must(m and int(m.group(1)) <= 270 and int(m.group(2)) <= 270,
     "sideScroll width <=270, got %s" % (str(m.groups()) if m else None))

m = re.search(r'name="sideLay">\s*<property name="spacing">\s*<number>(\d+)</number>', ui)
must(m and int(m.group(1)) <= 2, "sideLay spacing <=2, got %s" % (m.group(1) if m else None))

for edge in ("leftMargin", "topMargin", "rightMargin", "bottomMargin"):
    m = re.search(rf'name="sideLay">.*?name="{edge}">\s*<number>(\d+)</number>', ui, re.S)
    must(m and int(m.group(1)) <= 6, "sideLay %s <=6, got %s" % (edge, m.group(1) if m else None))

must('name="formConnect"' in ui, "formConnect missing (work/transport must be form rows)")
must('vsizetype="Maximum"' in ui.split('name="paramStack"', 1)[1][:500], "paramStack Maximum")
vs = re.findall(r'name="verticalSpacing">\s*<number>(\d+)</number>', ui)
must(vs and all(int(x) <= 1 for x in vs), "form verticalSpacing <=1, got %s" % vs)

must('name="hexDisplayCheck"' in ui and 'name="hexSendCheck"' in ui, "hex checkboxes missing")
must('rxFormatCombo' not in ui and 'txFormatCombo' not in ui, "format combos must be gone")
must("接收设置：" in ui and "发送设置：" in ui, "section titles with colon")

# QSS density markers
must("QWidget#SidePanel QComboBox" in cpp, "SidePanel-scoped combo QSS missing")
must("max-height: 22px" in cpp, "combo max-height 22 missing")
must("max-height: 20px" in cpp, "checkbox max-height 20 missing")
must("min-height: 26px" in cpp and "PrimaryButton" in cpp, "primary button dense style missing")

if fails:
    print("SELFCHECK FAIL")
    for f in fails:
        print(" -", f)
    sys.exit(1)

print("SELFCHECK PASS")
print(" width", re.search(r'name="sideScroll".*?<width>(\d+)</width>', ui, re.S).group(1))
print(" spacing", re.search(r'name="sideLay">\s*<property name="spacing">\s*<number>(\d+)</number>', ui).group(1))
print(" verticalSpacing", sorted(set(vs)))
print(" formConnect", "yes")
print(" SidePanel QSS", "yes")
