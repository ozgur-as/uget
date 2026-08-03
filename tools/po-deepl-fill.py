#!/usr/bin/env python3
"""Fill untranslated entries in po/*.po using the DeepL API.

Only touches entries whose msgstr is empty; existing (human) translations are
left alone.  GTK mnemonics and trailing punctuation are stripped before the
text is sent and restored afterwards, using the convention already present in
each catalogue: CJK locales get the "translated(_X)" form, everyone else gets
an inline underscore.

Usage:  DEEPL_AUTH_KEY=xxx tools/po-deepl-fill.py [lang ...]
        tools/po-deepl-fill.py --list-languages
"""

import json
import os
import re
import sys
import time
import urllib.error
import urllib.parse
import urllib.request

# uGet locale -> DeepL target code.  Locales absent here are not offered by
# DeepL and are skipped.
DEEPL_TARGET = {
    "ar": "AR", "be": "BE", "bg": "BG", "bn_BD": "BN", "ca": "CA", "cs": "CS",
    "da": "DA", "de": "DE", "es": "ES", "fa": "FA", "fr": "FR", "hr": "HR",
    "hu": "HU", "id": "ID", "it": "IT", "ja": "JA", "ka_GE": "KA", "kk": "KK",
    "lt": "LT", "pl": "PL", "pt_BR": "PT-BR", "ru": "RU", "sk_SK": "SK",
    "sr": "SR", "sr@latin": "SR", "sv": "SV", "tr": "TR", "uk": "UK",
    "uz@Latn": "UZ", "vi": "VI", "zh_CN": "ZH-HANS", "zh_TW": "ZH-HANT",
}

# DeepL returns Serbian in Cyrillic only; sr@latin needs transliterating.
SR_LATIN = [
    ("Љ", "Lj"), ("Њ", "Nj"), ("Џ", "Dž"), ("љ", "lj"), ("њ", "nj"),
    ("џ", "dž"), ("А", "A"), ("Б", "B"), ("В", "V"), ("Г", "G"), ("Д", "D"),
    ("Ђ", "Đ"), ("Е", "E"), ("Ж", "Ž"), ("З", "Z"), ("И", "I"), ("Ј", "J"),
    ("К", "K"), ("Л", "L"), ("М", "M"), ("Н", "N"), ("О", "O"), ("П", "P"),
    ("Р", "R"), ("С", "S"), ("Т", "T"), ("Ћ", "Ć"), ("У", "U"), ("Ф", "F"),
    ("Х", "H"), ("Ц", "C"), ("Ч", "Č"), ("Ш", "Š"), ("а", "a"), ("б", "b"),
    ("в", "v"), ("г", "g"), ("д", "d"), ("ђ", "đ"), ("е", "e"), ("ж", "ž"),
    ("з", "z"), ("и", "i"), ("ј", "j"), ("к", "k"), ("л", "l"), ("м", "m"),
    ("н", "n"), ("о", "o"), ("п", "p"), ("р", "r"), ("с", "s"), ("т", "t"),
    ("ћ", "ć"), ("у", "u"), ("ф", "f"), ("х", "h"), ("ц", "c"), ("ч", "č"),
    ("ш", "š"),
]


def to_serbian_latin(text):
    for cyr, lat in SR_LATIN:
        text = text.replace(cyr, lat)
    return text

# Locales that mark accelerators as "translated(_X)" rather than inline "_".
PAREN_MNEMONIC = {"ja", "zh_CN", "zh_TW"}

BATCH = 40

# msgids that must never be machine-translated: gettext metadata, and strings
# with no natural-language content for DeepL to work on.
SKIP_MSGID = {"translator-credits", "%"}

# Passed to DeepL as untranslated context so short UI labels are not read as
# prose.  Without it "New Batch Sequence" comes back as a manufacturing batch.
CONTEXT = ("User interface strings for uGet, a download manager. Short button, "
           "menu and column labels. Translate as an application would label "
           "them, not as prose sentences.")

# Labels that DeepL cannot tell apart without help, and that must not collapse
# onto each other in the finished catalogue.  "Move Up" moves one position,
# "Move Top" goes all the way; "Quit" leaves the application while "Shutdown"
# and "Turn off" power the machine down at the end of a schedule.
PER_STRING_CONTEXT = {
    "Move Up": "Toolbar tooltip.",
    "Move Down": "Toolbar tooltip.",
    "Move Top": "Toolbar tooltip.",
    "Move Bottom": "Toolbar tooltip.",
    "Move to Top": "Menu item acting on the selected download.",
    "Move to Bottom": "Menu item acting on the selected download.",
    "Quit": "Menu item that closes the uGet application. It does not power off "
            "the computer.",
    "Shutdown": "One of the actions uGet can perform once every download has "
                "finished, alongside Hibernate, Suspend and Reboot. It powers "
                "the computer off.",
    "Turn off": "A scheduler state, shown next to the states Normal and Limited "
                "speed. During an hour marked this way uGet stops all tasks and "
                "downloads nothing. It does NOT power the computer off.",
    "Best": "The first entry of a quality drop-down whose other entries are "
            "2160p, 1440p, 1080p and mp4, webm, mkv. It means the highest "
            "quality available. It is NOT the closing greeting of a letter.",
    "Remember": "A menu toggle. Checking it makes uGet remember the "
                "after-download action the user picked and reuse it next time. "
                "It is not an instruction to the user to remember something.",
    "Num": "One of three batch-sequence types offered together: None, Num, "
           "Char. This one numbers the generated URLs with digits.",
    "Char": "One of three batch-sequence types offered together: None, Num, "
            "Char. This one labels the generated URLs with letters.",
}

# Where the msgid is too terse for any amount of context to disambiguate, hand
# DeepL a fuller English sentence instead.  All four are toolbar tooltips, so a
# phrase rather than a clipped label is what belongs there anyway.
SOURCE_OVERRIDE = {
    "Move Up": "Move up one position",
    "Move Down": "Move down one position",
    "Move Top": "Move to the very top of the list",
    "Move Bottom": "Move to the very bottom of the list",
    # bare "Best" is read as the sign-off of a letter, "Remember" as an
    # instruction to the reader
    "Best": "Best available",
    "Remember": "Remember this choice",
    "Num": "Number",
    "Char": "Character",
}


def api_base(key):
    return "https://api-free.deepl.com" if key.endswith(":fx") else "https://api.deepl.com"


def call(key, path, params, attempts=6):
    url = api_base(key) + path
    data = urllib.parse.urlencode(params, doseq=True).encode()
    for n in range(attempts):
        req = urllib.request.Request(
            url, data=data,
            headers={"Authorization": "DeepL-Auth-Key " + key,
                     "Content-Type": "application/x-www-form-urlencoded"})
        try:
            with urllib.request.urlopen(req, timeout=60) as r:
                return json.loads(r.read().decode())
        except urllib.error.HTTPError as e:
            # 429 is DeepL throttling, 5xx is transient; anything else is ours
            if e.code not in (429, 500, 502, 503, 529) or n == attempts - 1:
                raise
            time.sleep(2 ** n)
    raise RuntimeError("unreachable")


def split_mnemonic(s):
    """'_Cancel' -> ('Cancel', 'C');  'New _Torrent...' -> ('New Torrent...', 'T')"""
    m = re.search(r"(?<!_)_(?!_)(\w)", s)
    letter = m.group(1) if m else None
    return re.sub(r"(?<!_)_(?!_)", "", s).replace("__", "_"), letter


def restore_mnemonic(text, letter, lang):
    if not letter:
        return text
    if lang in PAREN_MNEMONIC:
        return "%s(_%s)" % (text.rstrip(), letter.upper())
    # keep the original accelerator key if it starts a word in the translation
    for m in re.finditer(r"\b\w", text):
        if m.group(0).lower() == letter.lower():
            return text[:m.start()] + "_" + text[m.start():]
    # otherwise accelerate the first word; never place it mid-word
    m = re.search(r"\w", text)
    return text[:m.start()] + "_" + text[m.start():] if m else text


# A trailing ellipsis/colon, or a literal HTML tag name such as the "<A>" in
# "Link <A>", which must reach the catalogue untouched.
TRAIL = re.compile(r"(\s*(?:\.\.\.|\u2026|:|<[A-Za-z]+>))\s*$")


def split_trailing(s):
    m = TRAIL.search(s)
    if not m:
        return s, ""
    tail = m.group(1)
    # keep exactly one space in front of a tag, none in front of punctuation
    return s[:m.start()], (" " + tail.strip() if "<" in tail else tail)


def unescape(s):
    return s.replace('\\"', '"').replace("\\n", "\n").replace("\\t", "\t").replace("\\\\", "\\")


def escape(s):
    return s.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n").replace("\t", "\\t")


def fill(path, lang, key, target, force=frozenset()):
    lines = open(path, encoding="utf-8").read().split("\n")
    todo = []          # (first_line, last_line, source_text, mnemonic, trailing)
    i = 0
    while i < len(lines):
        if not lines[i].startswith("msgid "):
            i += 1
            continue
        # msgid, possibly continued over following bare-string lines
        gid = re.findall(r'"((?:[^"\\]|\\.)*)"', lines[i])[0]
        j = i + 1
        while j < len(lines) and lines[j].startswith('"'):
            gid += re.findall(r'"((?:[^"\\]|\\.)*)"', lines[j])[0]
            j += 1
        if j >= len(lines) or not lines[j].startswith("msgstr "):
            i = j
            continue
        # msgstr, likewise
        gstr = re.findall(r'"((?:[^"\\]|\\.)*)"', lines[j])[0]
        k = j + 1
        while k < len(lines) and lines[k].startswith('"'):
            gstr += re.findall(r'"((?:[^"\\]|\\.)*)"', lines[k])[0]
            k += 1
        src = unescape(gid)
        wanted = (not gstr) or (src in force)
        if gid and wanted and src not in SKIP_MSGID:
            body, letter = split_mnemonic(SOURCE_OVERRIDE.get(src, src))
            body, trail = split_trailing(body)
            todo.append((j, k, body, letter, trail, PER_STRING_CONTEXT.get(src)))
        i = k
    if not todo:
        print("%-9s nothing to do" % lang)
        return 0

    # strings carrying their own disambiguating context cannot share a request
    generic = [t for t in todo if t[5] is None]
    batches = [(CONTEXT, generic[o:o + BATCH]) for o in range(0, len(generic), BATCH)]
    batches += [(t[5], [t]) for t in todo if t[5] is not None]

    done = 0
    edits = {}         # first_line -> (last_line, replacement lines)
    for ctx, chunk in batches:
        params = {
            "text": [c[2] for c in chunk],
            "source_lang": "EN",
            "target_lang": target,
            "preserve_formatting": "1",
            "context": CONTEXT + " " + ctx if ctx is not CONTEXT else CONTEXT,
        }
        try:
            res = call(key, "/v2/translate", params)
        except urllib.error.HTTPError as e:
            print("%-9s HTTP %s: %s" % (lang, e.code, e.read().decode()[:200]))
            return done
        for (first, last, _src, letter, trail, _c), tr in zip(chunk, res["translations"]):
            out = tr["text"].strip()
            if lang == "sr@latin":
                out = to_serbian_latin(out)
            out = restore_mnemonic(out, letter, lang)
            if trail:
                # DeepL rounds a stripped fragment off into a sentence; drop the
                # punctuation it added so we do not end up with "... ..." or ".:"
                out = re.sub(r"[.:。،؟：]+\s*$", "", out)
            out += trail
            edits[first] = (last, ['msgstr "%s"' % escape(out)])
            done += 1

    # apply back to front so earlier line numbers stay valid
    for first in sorted(edits, reverse=True):
        last, repl = edits[first]
        lines[first:last] = repl
    open(path, "w", encoding="utf-8").write("\n".join(lines))
    print("%-9s filled %d" % (lang, done))
    return done


def main():
    key = os.environ.get("DEEPL_AUTH_KEY", "").strip()
    if not key:
        sys.exit("DEEPL_AUTH_KEY is not set")

    if "--list-languages" in sys.argv:
        req = urllib.request.Request(
            api_base(key) + "/v2/languages?type=target",
            headers={"Authorization": "DeepL-Auth-Key " + key})
        with urllib.request.urlopen(req, timeout=30) as r:
            langs = json.loads(r.read().decode())
        codes = sorted(x["language"] for x in langs)
        print("DeepL targets:", " ".join(codes))
        mapped = {v for v in DEEPL_TARGET.values()}
        print("mapped but unsupported:", sorted(mapped - set(codes)) or "none")
        return

    wanted = [a for a in sys.argv[1:] if not a.startswith("-")]
    # --redo re-translates the ambiguous labels even where they are filled
    force = frozenset(PER_STRING_CONTEXT) | frozenset(SOURCE_OVERRIDE) \
        if "--redo" in sys.argv else frozenset()
    total = 0
    for lang, target in sorted(DEEPL_TARGET.items()):
        if wanted and lang not in wanted:
            continue
        p = os.path.join("po", lang + ".po")
        if os.path.exists(p):
            total += fill(p, lang, key, target, force)
    print("total filled:", total)


if __name__ == "__main__":
    main()
