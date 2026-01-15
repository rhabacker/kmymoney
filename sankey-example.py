import os, sys
import pandas as pd
import plotly.graph_objects as go


def split_hierarchy(label: str):
    """
    Split 'A: B' → ('A', 'B')
    If no hierarchy, returns (None, label)
    """
    #if ":" in label:
    #    parent, child = label.split(":", 1)
    #    return parent.strip(), child.strip()
    return None, label.strip()


def load_and_validate_csv(csv_path: str) -> pd.DataFrame:
    df = pd.read_csv(csv_path)

    required_cols = {"source", "target", "value"}
    if not required_cols.issubset(df.columns):
        raise ValueError(f"CSV must contain {required_cols}")

    # --- Clean numeric values ---
    df["value"] = (
        df["value"]
        .astype(str)
        .str.replace(",", "", regex=False)
        .str.strip()
        .astype(float)
    )

    expanded_rows = []

    for _, row in df.iterrows():
        src_parent, src_child = split_hierarchy(row["source"])
        tgt_parent, tgt_child = split_hierarchy(row["target"])
        v = row["value"]

        # Base flow (parent → parent)
        expanded_rows.append((src_parent or src_child,
                              tgt_parent or tgt_child, v))

        # Parent → child
        if src_parent:
            expanded_rows.append((src_parent, src_child, v))
        if tgt_parent:
            expanded_rows.append((tgt_parent, tgt_child, v))

        # Child → parent / child → child
        expanded_rows.append((src_child,
                              tgt_parent or tgt_child, v))
        if tgt_parent:
            expanded_rows.append((src_child, tgt_child, v))

    expanded_df = pd.DataFrame(
        expanded_rows, columns=["source", "target", "value"]
    )

    # --- Aggregate identical links ---
    expanded_df = (
        expanded_df
        .groupby(["source", "target"], as_index=False)
        .agg({"value": "sum"})
    )

    return expanded_df


def validate_flow_balance(df: pd.DataFrame) -> None:
    """
    Warn (do not fail) if nodes have unbalanced inflow/outflow.
    """
    inflow = df.groupby("target")["value"].sum()
    outflow = df.groupby("source")["value"].sum()

    nodes = set(inflow.index).union(outflow.index)

    warnings = []
    for node in nodes:
        in_v = inflow.get(node, 0)
        out_v = outflow.get(node, 0)
        if abs(in_v - out_v) > 1e-6 and in_v != 0 and out_v != 0:
            warnings.append((node, in_v, out_v))

    if warnings:
        print("\n⚠️  Flow balance warnings:")
        for node, i, o in warnings:
            print(f"  {node}: inflow={i:,.2f}, outflow={o:,.2f}")
        print("  (This is allowed, but may indicate missing links)\n")


def plot_sankey(csv_path: str) -> None:
    """
    Read Sankey data from a CSV file and display a Plotly Sankey diagram.

    CSV must have columns: source, target, value
    """

    # --- Read CSV ---
    df = load_and_validate_csv(csv_path)

    print("\n--- Expanded Sankey Links (after hierarchy + grouping) ---")
    print(df.sort_values(["source", "target"]).to_string(index=False))
    validate_flow_balance(df)

    # --- Build node list automatically ---
    nodes = sorted(set(df["source"]).union(df["target"]))
    node_index = {name: i for i, name in enumerate(nodes)}

    # --- Convert to Plotly arrays ---
    source = df["source"].map(node_index)
    target = df["target"].map(node_index)
    value  = df["value"]

    totals = (
        df.groupby("source")["value"].sum()
        .add(df.groupby("target")["value"].sum(), fill_value=0)
    )

    node_labels = [f"{n}:\n{totals.get(n, 0):,.0f}" for n in nodes]


    # --- Build Sankey diagram ---
    fig = go.Figure(go.Sankey(
        node=dict(
            pad=20,
            thickness=18,
            label=node_labels
            # no colors → Plotly defaults
        ),
        link=dict(
            source=source,
            target=target,
            value=value,
            hovertemplate="%{value:,.0f}<extra></extra>"
        )
    ))

    fig.update_layout(
        title_text="Financial Sankey Diagram",
        font_size=11
    )

    fig.show()



def usage(appname):
    print("Usage:")
    print(f"  python {appname} <data.csv>")
    print(f"  python {appname}       # looks for {appname}.csv (script name)")
    sys.exit(1)


def main():
    # Case 1: explicit filename provided
    if len(sys.argv) == 2:
        csv_path = sys.argv[1]

    # Case 2: no filename → fall back to script name with .csv
    elif len(sys.argv) == 1:
        script_name = os.path.splitext(os.path.basename(sys.argv[0]))[0]
        csv_path = f"{script_name}.csv"

    # Anything else is invalid
    else:
        usage(sys.argv[0])

    if not os.path.isfile(csv_path):
        print(f"Error: CSV file not found: {csv_path}")
        usage(sys.argv[0])

    plot_sankey(csv_path)


if __name__ == "__main__":
    main()
