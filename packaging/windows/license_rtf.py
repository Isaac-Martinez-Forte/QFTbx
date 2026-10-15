import sys

text = open(sys.argv[1], encoding="utf-8").read()
body = text.replace("\\", "\\\\").replace("{", "\\{").replace("}", "\\}").replace("\n", "\\par\n")
with open(sys.argv[2], "w", encoding="ascii", errors="replace") as out:
    out.write("{\\rtf1\\ansi\\deff0{\\fonttbl{\\f0 Consolas;}}\\f0\\fs16 " + body + "}")
