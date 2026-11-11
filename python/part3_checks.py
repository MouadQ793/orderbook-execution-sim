# Part 3 checks: imbalance hit rates by horizon, average depth at best, and a row-by-row
# recomputation of the EWMA columns. Run as a script from python/, or cell by cell (# %%).

# %% 1. Imbalance hit rates by horizon (ties, i.e. imbalance exactly 0, are excluded) and average depth
import numpy as np
import pandas as pd

df = pd.read_csv("../Data/indicators_msft.csv")
ts = df["timestamp"].to_numpy()

# --- 1. Hit rate: sign of the imbalance some time before a mid move = sign of the move
valid = df[df["double_mid"].notna()].copy()
valid["ret"] = valid["double_mid"].diff()                 # sign is all we need
moves = valid[valid["mid_changed"] == 1]
move_idx = moves.index.to_numpy()                         # row numbers of the moves
move_sign = np.sign(moves["ret"].to_numpy())


def hit_rate(col: str, horizon_ns: int | None) -> float:
    """horizon_ns None = the row just before the move; else the value in force at t_move - horizon."""
    values = df[col].to_numpy()
    if horizon_ns is None:
        before = move_idx - 1
    else:
        # last row with timestamp <= t_move - horizon (and strictly before the move row)
        before = np.searchsorted(ts, ts[move_idx] - horizon_ns, side="right") - 1
        before = np.minimum(before, move_idx - 1)
    ok = before >= 0
    imb = values[before[ok]]
    sign = move_sign[ok]
    keep = ~np.isnan(imb) & (imb != 0)
    return float(np.mean(np.sign(imb[keep]) == sign[keep]))


horizons = {"1 message": None, "10 ms": 10_000_000, "100 ms": 100_000_000, "1 s": 1_000_000_000}
table = pd.DataFrame({name: [hit_rate("imbalance_1", h), hit_rate("imbalance_k", h)]
                      for name, h in horizons.items()},
                     index=["Level-1 imbalance", "5-level imbalance"])
print(table.round(3))
print()

# --- 2. Average depth at best, one side, time-weighted
dt = np.diff(ts, append=ts[-1]).astype(float)
has = df["depth_mean"].notna().to_numpy()
depth_both = np.sum(df["depth_mean"].to_numpy()[has] * dt[has]) / np.sum(dt[has])   # bid + ask
depth_side = depth_both / 2
print(f"time-weighted depth at best, bid + ask: {depth_both:,.0f} shares")
print(f"one side, D:                            {depth_side:,.0f} shares")
print(f"stylized model 1/(2D): 1 tick per {2 * depth_side:,.0f} shares of OFI")


# %% 2. Recompute the EWMAs row by row from the CSV and compare with the C++ columns
# The C++ writes 6 significant digits (default std::ostream precision), so expect
# relative differences around 1e-6, not 0. Anything above ~1e-4 is a real mismatch.
import numpy as np
import pandas as pd

ALPHA = 0.05       # m_alpha in Indicators
TAU_NS = 1e9       # tau in Indicators

df = pd.read_csv("../Data/indicators_msft.csv")
ts = df["timestamp"].to_numpy()
dmid = df["double_mid"].to_numpy()
spread = df["spread"].to_numpy()
has_mid = ~np.isnan(dmid)
changed = df["mid_changed"].to_numpy() == 1

mean_py = np.full(len(df), np.nan)
vol_py = np.full(len(df), np.nan)
spread_py = np.full(len(df), np.nan)
lam_py = np.zeros(len(df))

mean, var, sm, lam = 0.0, 0.0, 0.0, 0.0
prev_ts, prev_dmid, prev_spread, prev_exists = 0, 0.0, 0.0, False
for i in range(len(df)):
    a = 1.0 - np.exp(-(ts[i] - prev_ts) / TAU_NS)
    if prev_exists:                                  # spread in force during dt = previous one
        sm = (1 - a) * sm + a * prev_spread
    lam = (1 - a) * lam + 1e9 / TAU_NS
    if changed[i]:                                   # return EWMA, exact recursion
        r = (dmid[i] - prev_dmid) / 2
        d = r - mean
        mean = (1 - ALPHA) * mean + ALPHA * r
        var = (1 - ALPHA) * (var + ALPHA * d * d)
    if has_mid[i]:
        prev_dmid, prev_spread = dmid[i], spread[i]
        mean_py[i], vol_py[i], spread_py[i] = mean, np.sqrt(var), sm
    lam_py[i] = lam
    prev_ts, prev_exists = ts[i], has_mid[i]


def compare(name: str, cpp: np.ndarray, py: np.ndarray) -> None:
    ok = ~np.isnan(cpp) & ~np.isnan(py)
    diff = np.abs(cpp[ok] - py[ok])
    rel = diff / np.maximum(np.abs(py[ok]), 1e-12)
    worst = np.argmax(diff)
    verdict = "OK" if np.max(np.minimum(rel, diff)) < 1e-4 else "MISMATCH"
    print(f"{name:12s} rows {ok.sum():>8,}  max abs diff {diff.max():.2e}  "
          f"max rel diff {np.max(np.minimum(rel, 1)):.2e}  -> {verdict}")
    if verdict == "MISMATCH":
        print(f"   first bad row: {np.flatnonzero(ok)[worst]}, C++ {cpp[ok][worst]}, Python {py[ok][worst]}")


compare("ret_mean", df["ret_mean"].to_numpy(), mean_py)
compare("ret_vol", df["ret_vol"].to_numpy(), vol_py)
compare("spread_mean", df["spread_mean"].to_numpy(), spread_py)
compare("intensity", df["intensity"].to_numpy(), lam_py)
print("depth_mean: the CSV has no raw depth column, so it can't be recomputed here "
      "(add a `depth` column to toCsvLine to check it the same way as the spread).")
