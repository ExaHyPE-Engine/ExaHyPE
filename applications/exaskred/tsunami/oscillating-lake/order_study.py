"""Plot the error of the oscillating lake against the order of the finite-volume scheme, for
ADER-WENO as solver and for the subcell schemes of ADER-DG at one fixed DG order.

Reads the runs of limiter_comparison.py under limiter-comparison/order-study:
weno<N>/weno-scaling-fallback-volumes for ADER-WENO of order N, dg7/dg-godunov, dg7/dg-muscl and
dg7-weno<N>/dg-weno-scaling-fallback-volumes for ADER-DG of order 7, and
dg<P>-weno2/dg-weno-scaling-fallback-volumes for ADER-DG of order P limited with WENO 2.
"""

import argparse
import json
from pathlib import Path

from limiter_comparison import GRID, MUTED, NORM_STYLE, OUTPUT, TEXT, read_errors

WENO_ORDERS = (2, 3, 4, 5)
DG_ORDERS = (3, 5, 7, 9)


def panels(study, dg_order):
    weno = [(f"WENO {n}", n + 1, study / f"weno{n}" / "weno-scaling-fallback-volumes") for n in WENO_ORDERS]
    dg = [
        ("Godunov", 1, study / f"dg{dg_order}" / "dg-godunov"),
        ("MUSCL", 2, study / f"dg{dg_order}" / "dg-muscl"),
    ] + [(f"WENO {n}", n + 1, study / f"dg{dg_order}-weno{n}" / "dg-weno-scaling-fallback-volumes") for n in WENO_ORDERS]
    return [("ADER-WENO solver", weno), (f"ADER-DG of order {dg_order} + subcell scheme", dg)]


def dg_order_runs(study):
    return [(f"DG {p}", p + 1, study / f"dg{p}-weno2" / "dg-weno-scaling-fallback-volumes") for p in DG_ORDERS]


def load(runs):
    loaded = []
    for name, order, directory in runs:
        errors = read_errors(directory)
        if errors is None or not errors["t"] or not (directory / "run.json").exists():
            continue
        metadata = json.loads((directory / "run.json").read_text())
        means = {norm: sum(values) / len(values) for norm, values in errors.items() if norm != "t"}
        finals = {norm: values[-1] for norm, values in errors.items()}
        loaded.append((name, order, means, finals, metadata))
    return loaded


def style(axes, runs):
    axes.set_yscale("log")
    axes.set_xticks([order for _, order, *_ in runs], [f"{name}\n({order})" for name, order, *_ in runs])
    axes.set_xlabel("scheme (formal order)", color=MUTED)
    axes.grid(True, which="major", color=GRID, linewidth=0.8)
    axes.tick_params(colors=MUTED)
    for spine in ("top", "right"):
        axes.spines[spine].set_visible(False)
    for spine in ("left", "bottom"):
        axes.spines[spine].set_color(GRID)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--study", type=Path, default=OUTPUT / "order-study")
    parser.add_argument("--dg-order", type=int, default=7)
    args = parser.parse_args()

    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    loaded = [(title, load(runs)) for title, runs in panels(args.study, args.dg_order)]
    dg_orders = load(dg_order_runs(args.study))
    figures = args.study / "figures"
    figures.mkdir(parents=True, exist_ok=True)

    figure, axes_pair = plt.subplots(1, 2, figsize=(12.8, 4.4), dpi=150, sharey=True)
    for axes, (title, runs) in zip(axes_pair, loaded):
        for norm, (color, line) in NORM_STYLE.items():
            orders = [order for _, order, *_ in runs]
            axes.plot(orders, [means[norm] for _, _, means, *_ in runs], color=color, linestyle=line,
                      linewidth=2, marker="o", markersize=8, markeredgecolor="white", markeredgewidth=2,
                      label=norm)
        style(axes, runs)
        axes.set_title(title, loc="left", fontsize=11, color=TEXT)
        axes.legend(frameon=False, fontsize=8, loc="upper right", labelcolor=TEXT)
    axes_pair[0].set_ylabel("error, mean over 0 ≤ t ≤ 10", color=MUTED)
    figure.tight_layout()
    figure.savefig(figures / "order-error.png")
    plt.close(figure)

    figure, axes_pair = plt.subplots(1, 2, figsize=(12.8, 4.0), dpi=150)
    for axes, (title, runs) in zip(axes_pair, loaded):
        orders = [order for _, order, *_ in runs]
        axes.plot(orders, [metadata["seconds"] for *_, metadata in runs], color="#2a78d6", linewidth=2,
                  marker="o", markersize=8, markeredgecolor="white", markeredgewidth=2)
        for order, (*_, metadata) in zip(orders, runs):
            device = "GPU" if "cuda" in metadata["peano_build"] else "CPU"
            axes.annotate(device, (order, metadata["seconds"]), xytext=(0, 8), textcoords="offset points",
                          ha="center", fontsize=8, color=MUTED)
        style(axes, runs)
        axes.set_title(title, loc="left", fontsize=11, color=TEXT)
    axes_pair[0].set_ylabel("wall time to t = 10 [s]", color=MUTED)
    figure.tight_layout()
    figure.savefig(figures / "order-cost.png")
    plt.close(figure)

    figure, axes_pair = plt.subplots(1, 2, figsize=(12.8, 4.4), dpi=150)
    orders = [order for _, order, *_ in dg_orders]
    for norm, (color, line) in NORM_STYLE.items():
        axes_pair[0].plot(orders, [means[norm] for _, _, means, *_ in dg_orders], color=color, linestyle=line,
                          linewidth=2, marker="o", markersize=8, markeredgecolor="white", markeredgewidth=2,
                          label=norm)
    axes_pair[0].legend(frameon=False, fontsize=8, loc="upper right", labelcolor=TEXT)
    axes_pair[0].set_ylabel("error, mean over 0 ≤ t ≤ 10", color=MUTED)
    axes_pair[1].plot(orders, [metadata["seconds"] for *_, metadata in dg_orders], color="#2a78d6", linewidth=2,
                      marker="o", markersize=8, markeredgecolor="white", markeredgewidth=2)
    for order, (*_, metadata) in zip(orders, dg_orders):
        device = "GPU" if "cuda" in metadata["peano_build"] else "CPU"
        axes_pair[1].annotate(device, (order, metadata["seconds"]), xytext=(0, 8), textcoords="offset points",
                              ha="center", fontsize=8, color=MUTED)
    axes_pair[1].set_ylabel("wall time to t = 10 [s]", color=MUTED)
    for axes, title in zip(axes_pair, ("ADER-DG + WENO 2 subcells: error", "ADER-DG + WENO 2 subcells: cost")):
        style(axes, dg_orders)
        axes.set_xlabel("ADER-DG order (formal order)", color=MUTED)
        axes.set_title(title, loc="left", fontsize=11, color=TEXT)
    figure.tight_layout()
    figure.savefig(figures / "dg-order-error.png")
    plt.close(figure)

    lines = ["| solver | scheme | formal order | mean L1 | mean L2 | mean L∞ | final L2 | t reached | wall [s] | device |",
             "|---|---|---|---|---|---|---|---|---|---|"]
    for title, runs in loaded + [("ADER-DG + WENO 2 subcells", dg_orders)]:
        for name, order, means, finals, metadata in runs:
            device = "GPU" if "cuda" in metadata["peano_build"] else "CPU"
            lines.append(
                f"| {title} | {name} | {order} | {means['L1']:.3g} | {means['L2']:.3g} | {means['L∞']:.3g} "
                f"| {finals['L2']:.3g} | {finals['t']:.3g} | {metadata['seconds']:.0f} | {device} |"
            )
    (figures / "order-summary.md").write_text("\n".join(lines) + "\n")
    print("\n".join(lines))


if __name__ == "__main__":
    main()
