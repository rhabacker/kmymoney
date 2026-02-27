#!/usr/bin/python3.11

import sys
import csv
import re
from decimal import Decimal, InvalidOperation
import pandas as pd
import plotly.graph_objects as go

import sys
import csv
import re
from decimal import Decimal, InvalidOperation
import pandas as pd
import plotly.graph_objects as go

# ---------- Number parser ----------
def parse_amount(value: str) -> Decimal:
    if value is None:
        return Decimal("0")
    v = value.strip().replace('"', '').replace('€', '').replace('\xa0','').replace(' ','')
    if v == "":
        return Decimal("0")
    if "," in v and "." in v:
        if v.rfind(",") > v.rfind("."):
            v = v.replace(".","").replace(",",".")
        else:
            v = v.replace(",","")
    elif "," in v:
        v = v.replace(",",".")
    try:
        return Decimal(v)
    except InvalidOperation:
        print("PARSE ERROR:", repr(value))
        return Decimal("0")

# ---------- CSV importer ----------
def import_kmymoney_networth_csv(path: str, delimiter=","):
    sections = {}
    current_section = None
    with open(path, newline="", encoding="utf-8") as f:
        reader = csv.reader(f, delimiter=delimiter)
        lines = list(reader)
        title = lines[0][0] if len(lines) > 0 else "Sankey Diagram"
        date_range = lines[1][0] if len(lines) > 1 else ""
        for row in lines[2:]:
            print(row)
            if not row:
                continue
            if row[0] == "Account":
                continue
            if "Grand" in row:
                continue
            if len(row)==1 and row[0] and not row[0].lower().startswith("total"):
                current_section = row[0].strip()
                sections.setdefault(current_section, [])
                continue
            if len(row)>=3 and current_section:
                name = row[0].strip()
                if name.lower().startswith("total") or name.lower().endswith("total"):
                    continue
                amount = parse_amount(row[2])
                if amount != 0:
                    sections[current_section].append((f"{name} [{amount}]", amount))
                    #sections[current_section].append((f"{name}", amount))
    return sections, title, date_range

# ---------- Sankey builder ----------
def build_sankey_from_sections(sections: dict):
    rows = []
    ROOT = "Net Worth"
    for section, items in sections.items():
        section_sum = sum(v for _, v in items)
        if section_sum==0:
            continue
        rows.append((ROOT, section, float(section_sum)))
        for name, amount in items:
            rows.append((section, name, float(amount)))
    df = pd.DataFrame(rows, columns=["source","target","value"])
    return df

# ---------- Main ----------
def main():
    if len(sys.argv)<2:
        print("Usage: python sankey_plot.py <kmymoney_export.csv>")
        sys.exit(1)
    csv_path = sys.argv[1]
    sections, title, date_range = import_kmymoney_networth_csv(csv_path, delimiter=",")
    df = build_sankey_from_sections(sections)
    nodes = sorted(set(df["source"]).union(df["target"]))
    node_index = {name:i for i,name in enumerate(nodes)}
    source = df["source"].map(node_index)
    target = df["target"].map(node_index)
    value  = df["value"]

    # --- Build color map ---
    section_colors = {
        "Asset": "green",
        "Cash": "blue",
        "Checking": "orange",
        "Liability": "red",
    }

    # Determine link colors by section (source node)
    link_colors = []
    for s in df['source']:
        # If the source is ROOT, pick a default color
        if s == "Net Worth":
            link_colors.append("gray")
        else:
            link_colors.append(section_colors.get(s, "lightgray"))

    # --- Build Sankey diagram ---
    fig = go.Figure(go.Sankey(
        node=dict(
            pad=20,
            thickness=18,
            label=nodes
        ),
        link=dict(
            source=source,
            target=target,
            value=value,
            color=link_colors,
            #hovertemplate="%{value:,.2f}<extra></extra>"
            hovertemplate="%{source} → %{target}<br>Value: %{value:,.2f}<extra></extra>"
        )
    ))

    # --- Add node value annotations ---

    fig.update_layout(
        title_text=f"{title}\n{date_range}",
        font_size=11
    )
    fig.show()

if __name__=="__main__":
    main()
