# Part 3: every figure from Data/indicators_msft.csv, saved to figures/.
# Run as a script from python/, or cell by cell (# %%) in the notebook.
# Plots use one point per second (or per mid change) so they stay fast; all numbers use every row.

# %% Setup
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

DATA_PATH = Path("../Data/indicators_msft.csv")
FIG_DIR = Path("../figures")
FIG_DIR.mkdir(exist_ok=True)

TAU_S = 1.0                   # must match tau in Indicators (1e9 ns = 1 s)
OFI_INTERVALS_S = [1, 10, 60]
OFI_MAIN_INTERVAL_S = 10

BLUE = "#2a78d6"
LIGHT_BLUE = "#b7d3f6"
INK = "#0b0b0b"
INK_2 = "#52514e"
plt.rcParams.update({
    "figure.dpi": 110,
    "axes.edgecolor": INK_2,
    "axes.labelcolor": INK,
    "axes.titlecolor": INK,
    "axes.titlesize": 12,
    "axes.titleweight": "bold",
    "axes.grid": True,
    "grid.color": "#e6e5e0",
    "grid.linewidth": 0.8,
    "axes.axisbelow": True,
    "axes.spines.top": False,
    "axes.spines.right": False,
    "xtick.color": INK_2,
    "ytick.color": INK_2,
    "lines.linewidth": 1.2,
})


def save(fig, name: str) -> None:
    fig.tight_layout()
    fig.savefig(FIG_DIR / name)
    plt.show()
    print(f"saved figures/{name}")


df = pd.read_csv(DATA_PATH)
t = df["timestamp"].to_numpy() / 1e9                 # seconds since 9:30
df["hour"] = 9.5 + t / 3600                          # clock time
df["mid"] = df["double_mid"] / 2 / 100               # half-ticks -> dollars
df["second"] = df["timestamp"] // 1_000_000_000      # for one-point-per-second plots
dt = np.diff(t, append=t[-1])                        # time each row stays in force
has_mid = df["double_mid"].notna()
changes = df[df["mid_changed"] == 1]
print(f"{len(df):,} rows, {len(changes):,} mid changes, {t[-1] - t[0]:,.0f} s")

# %% 1. Mid price over the day
mid_per_sec = df[has_mid].groupby("second")[["hour", "mid"]].last()
fig, ax = plt.subplots(figsize=(10, 3.5))
ax.plot(mid_per_sec["hour"], mid_per_sec["mid"], color=BLUE, linewidth=0.8)
ax.set_title("MSFT mid price, 21 June 2012")
ax.set_xlabel("Time of day (hours)")
ax.set_ylabel("Mid ($)")
save(fig, "mid_price.png")

# %% 2. EWMA vol of mid returns (rows where the mid changed)
fig, ax = plt.subplots(figsize=(10, 3.5))
ax.plot(changes["hour"], changes["ret_vol"], color=BLUE, linewidth=0.8)
ax.set_title("EWMA vol of mid returns (alpha = 0.05 per mid change)")
ax.set_xlabel("Time of day (hours)")
ax.set_ylabel("Vol (ticks per mid change)")
ax.set_ylim(bottom=0)
save(fig, "ewma_vol.png")

# %% 3. Vol and share of time with a wide spread (two panels, one time axis)
spread = df.loc[has_mid, ["timestamp", "hour", "spread"]].copy()
spread["duration"] = spread["timestamp"].shift(-1) - spread["timestamp"]
spread["bin"] = np.floor(spread["hour"] * 12) / 12                      # 5-minute bins
wide_time = spread["duration"].where(spread["spread"] >= 2, 0).groupby(spread["bin"]).sum()
share_wide = 100 * wide_time / spread.groupby("bin")["duration"].sum()

fig, (ax_vol, ax_spread) = plt.subplots(
    2, 1, figsize=(10, 5), sharex=True, gridspec_kw={"height_ratios": [2, 1]})
ax_vol.plot(changes["hour"], changes["ret_vol"], color=BLUE, linewidth=0.8)
ax_vol.set_title("EWMA vol (top) and time with a wide spread (bottom)")
ax_vol.set_ylabel("Vol (ticks)")
ax_vol.set_ylim(bottom=0)
ax_spread.bar(share_wide.index + 1 / 24, share_wide.values, width=1 / 12 * 0.85, color=BLUE)
ax_spread.set_ylabel("Time with\nspread >= 2 (%)")
ax_spread.set_xlabel("Time of day (hours)")
ax_spread.set_ylim(bottom=0)
save(fig, "vol_and_spread.png")

# %% 4. Mean mid move by level-1 imbalance just before it
valid = df[has_mid].copy()
valid["ret"] = valid["double_mid"].diff() / 2                    # ticks
valid["imb_before"] = valid["imbalance_1"].shift(1)              # imbalance on the row before
moves = valid[valid["mid_changed"] == 1].dropna(subset=["ret", "imb_before"])
edges = np.linspace(-1, 1, 11)
moves["bucket"] = pd.cut(moves["imb_before"], edges, include_lowest=True)
by_bucket = moves.groupby("bucket", observed=False)["ret"].agg(["mean", "count"])
centers = (edges[:-1] + edges[1:]) / 2

fig, ax = plt.subplots(figsize=(8, 4))
ax.bar(centers, by_bucket["mean"], width=0.17, color=BLUE)
ax.axhline(0, color=INK_2, linewidth=0.8)
ax.set_title("Mean mid move by level-1 imbalance just before it")
ax.set_xlabel("Level-1 imbalance before the move")
ax.set_ylabel("Mean mid move (ticks)")
ax.set_xticks(edges)
save(fig, "return_by_imbalance.png")
print(by_bucket.assign(mean=lambda d: d["mean"].round(3)))

# %% 5. Time-weighted EWMA spread (check against the exact time-weighted average)
has_spread = df["spread"].notna().to_numpy()
exact_tw = np.sum(df["spread"].to_numpy()[has_spread] * dt[has_spread]) / np.sum(dt[has_spread])
has_sm = df["spread_mean"].notna().to_numpy()
ewma_tw = np.sum(df["spread_mean"].to_numpy()[has_sm] * dt[has_sm]) / np.sum(dt[has_sm])
print(f"time-weighted spread (exact):   {exact_tw:.4f} ticks")
print(f"time-average of spread_mean:    {ewma_tw:.4f} ticks")
print(f"message-weighted spread (bias): {df['spread'].mean():.4f} ticks")

spread_per_sec = df.groupby("second")[["hour", "spread_mean"]].mean()
fig, ax = plt.subplots(figsize=(10, 3.5))
ax.plot(spread_per_sec["hour"], spread_per_sec["spread_mean"], color=BLUE, linewidth=0.8)
ax.axhline(1, color=INK_2, linewidth=0.8)
ax.set_title(f"Time-weighted EWMA spread (tau = {TAU_S:g} s)")
ax.set_xlabel("Time of day (hours)")
ax.set_ylabel("Spread (ticks)")
save(fig, "spread_mean.png")

# %% 6. Time-weighted EWMA depth at best
depth_per_sec = df.groupby("second")[["hour", "depth_mean"]].mean()
fig, ax = plt.subplots(figsize=(10, 3.5))
ax.plot(depth_per_sec["hour"], depth_per_sec["depth_mean"] / 1000, color=BLUE, linewidth=0.8)
ax.set_title(f"Time-weighted EWMA depth at best, bid + ask (tau = {TAU_S:g} s)")
ax.set_xlabel("Time of day (hours)")
ax.set_ylabel("Depth (thousands of shares)")
ax.set_ylim(bottom=0)
save(fig, "depth_mean.png")

# %% 7. Message intensity vs exact count over the last TAU_S seconds
left = np.searchsorted(t, t - TAU_S, side="right")
df["window_rate"] = (np.arange(len(t)) - left + 1) / TAU_S
corr = df[["intensity", "window_rate"]].corr().iloc[0, 1]
print(f"messages per second over the day: {len(df) / (t[-1] - t[0]):.1f}")
print(f"correlation EWMA vs exact trailing {TAU_S:g} s count: {corr:.3f}")

per_sec = df.groupby("second")[["hour", "intensity", "window_rate"]].max()   # max keeps the peaks
fig, ax = plt.subplots(figsize=(10, 3.5))
ax.plot(per_sec["hour"], per_sec["window_rate"], color=LIGHT_BLUE, linewidth=0.6,
        label=f"Exact count, last {TAU_S:g} s")
ax.plot(per_sec["hour"], per_sec["intensity"], color=BLUE, linewidth=0.8, label="EWMA intensity")
ax.set_title("Message intensity: EWMA vs exact trailing count")
ax.set_xlabel("Time of day (hours)")
ax.set_ylabel("Messages per second")
ax.legend(frameon=False)
save(fig, "intensity.png")


# %% 8. Order flow imbalance: regression of the mid change on summed OFI
def per_interval(data: pd.DataFrame, seconds: int) -> pd.DataFrame:
    """Total OFI and mid change (ticks, end of previous interval -> end of this one)
    per interval. Intervals with no message get OFI 0."""
    bins = data["timestamp"] // (seconds * 1_000_000_000)
    out = pd.DataFrame({
        "ofi": data["ofi"].groupby(bins).sum(),
        "mid_end": data["double_mid"].ffill().groupby(bins).last() / 2,
    })
    out = out.reindex(pd.RangeIndex(out.index.min(), out.index.max() + 1))
    out["ofi"] = out["ofi"].fillna(0)
    out["mid_end"] = out["mid_end"].ffill()
    out["dmid"] = out["mid_end"].diff()
    return out.dropna(subset=["dmid"])


def fit(x: np.ndarray, y: np.ndarray) -> tuple[float, float, float]:
    """Least squares y = a + b x on finite points. Returns (a, b, R^2)."""
    ok = np.isfinite(x) & np.isfinite(y)
    x, y = x[ok], y[ok]
    b, a = np.polyfit(x, y, 1)
    r2 = 1 - (y - (a + b * x)).var() / y.var()
    return a, b, r2


rows = []
for s in OFI_INTERVALS_S:
    iv = per_interval(df, s)
    _, b, r2 = fit(iv["ofi"].to_numpy(dtype=float), iv["dmid"].to_numpy())
    rows.append({"interval (s)": s, "n intervals": len(iv),
                 "slope (ticks per 1,000 shares)": b * 1000, "R^2": r2})
print(pd.DataFrame(rows).to_string(index=False, float_format=lambda v: f"{v:.3f}"))

iv = per_interval(df, OFI_MAIN_INTERVAL_S)
x = iv["ofi"].to_numpy(dtype=float)
y = iv["dmid"].to_numpy()
a, b, r2 = fit(x, y)
fig, ax = plt.subplots(figsize=(7, 5))
ax.scatter(x / 1000, y, s=8, color=BLUE, alpha=0.35, linewidths=0)
xs = np.linspace(np.nanmin(x), np.nanmax(x), 100)
ax.plot(xs / 1000, a + b * xs, color=INK_2, linewidth=1.5)
ax.axhline(0, color=INK_2, linewidth=0.6)
ax.axvline(0, color=INK_2, linewidth=0.6)
ax.set_title(f"Mid change vs order flow imbalance ({OFI_MAIN_INTERVAL_S} s intervals)")
ax.set_xlabel("OFI over the interval (thousands of shares)")
ax.set_ylabel("Mid change over the interval (ticks)")
ax.text(0.02, 0.97, f"R² = {r2:.2f}\nslope = {b * 1000:.3f} ticks per 1,000 shares",
        transform=ax.transAxes, va="top", color=INK_2)
save(fig, "ofi_regression.png")
