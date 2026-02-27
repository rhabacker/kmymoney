#!/usr/bin/python3.11

import csv
import json
import urllib.parse
import re
import sys
from decimal import Decimal, InvalidOperation

BASE_URL = "https://www.finanzfluss.de/rechner/flussdiagramm/"

# ---------- Helpers ----------

def parse_amount(value: str) -> Decimal:
    if not value:
        return Decimal("0")

    # Normalize whitespace and symbols
    v = (
        value
        .strip()
        .replace("€", "")
        .replace("$", "")
        .replace("\xa0", "")  # non-breaking space
        .replace(" ", "")
    )

    # Case 1: German format: 48.526,62
    if re.match(r"^[+-]?[0-9.]+,[0-9]{2}$", v):
        v = v.replace(".", "").replace(",", ".")

    # Case 2: US format: 48,526.62
    elif re.match(r"^[+-]?[0-9,]+\.[0-9]{2}$", v):
        v = v.replace(",", "")

    # Case 3: Plain decimal: 48526.62
    elif re.match(r"^[+-]?[0-9]+(\.[0-9]{1,2})?$", v):
        pass

    else:
        print("⚠️ Unrecognized number format:", repr(value))
        return Decimal("0")

    try:
        return Decimal(v)
    except Exception as e:
        print("❌ Parse failed:", repr(value), "->", repr(v), e)
        return Decimal("0")


# ---------- Import CSV ----------

def import_kmymoney_networth_csv(path: str):
    sections = {}
    current_section = None

    with open(path, newline="", encoding="utf-8") as f:
        reader = csv.reader(f)

        for row in reader:
            if not row:
                continue

            # Skip report header lines
            if row[0].startswith("Report:"):
                continue
            if "through" in row[0]:
                continue
            if row[0] == "Account":
                continue

            # Section headers: Asset, Cash, Liability, etc.
            if len(row) == 1 and row[0] and not row[0].lower().startswith("total"):
                current_section = row[0].strip()
                sections.setdefault(current_section, [])
                continue

            # Data rows
            if len(row) >= 3 and current_section:
                name = row[0].strip()

                if name.lower().startswith("total"):
                    continue

                amount = parse_amount(row[2])
                sections[current_section].append((name, amount))

    return sections

# ---------- Transform to Finanzfluss ----------

def to_finanzfluss(sections: dict):
    categories = []
    total_value = sum(sum(v for _, v in items) for items in sections.values())

    if total_value == 0:
        raise ValueError("Total net worth is zero – cannot build percentages.")

    for i, (section, items) in enumerate(sections.items(), start=1):
        section_sum = sum(v for _, v in items)
        if section_sum == 0:
            continue

        categories.append({
            "n": section,
            "v": float(section_sum),
            "p": float((section_sum / total_value) * 100),
            "ro": 1,
            "r": f"ref_costs_{i}",
            "po": [
                {"n": name, "v": float(amount)}
                for name, amount in items if amount != 0
            ]
        })

    data = {
        "i": [
            {"n": "Net Worth Snapshot", "v": float(total_value), "p": 100.0}
        ],
        "c": categories
    }

    return data


# ---------- Encode URL ----------

def encode_finanzfluss_url(data: dict) -> str:
    i_json = json.dumps(data["i"], ensure_ascii=False, separators=(",", ":"))
    c_json = json.dumps(data["c"], ensure_ascii=False, separators=(",", ":"))

    i_encoded = urllib.parse.quote(i_json, safe="")
    c_encoded = urllib.parse.quote(c_json, safe="")

    return f"{BASE_URL}?i={i_encoded}&c={c_encoded}"


# ---------- Main ----------

def main():
    if len(sys.argv) < 2:
        print("Usage: python kmymoney_to_finanzfluss.py <kmymoney_export.csv>")
        sys.exit(1)

    csv_path = sys.argv[1]

    sections = import_kmymoney_networth_csv(csv_path)
    print(sections)

    # DEBUG: dump parsed sections
    print("=== DEBUG: Parsed Sections ===")
    for section, items in sections.items():
        print(f"[{section}]")
        for name, amount in items:
            print(f"  {name} -> {amount}")
        print(f"  Section total: {sum(a for _, a in items)}")
    print("=== END DEBUG ===")

    data = to_finanzfluss(sections)
    url = encode_finanzfluss_url(data)

    print(url)


if __name__ == "__main__":
    main()
