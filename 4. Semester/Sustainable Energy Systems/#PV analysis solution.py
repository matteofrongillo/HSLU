#PV analysis updated code

# PV analysis (clean + reusable)
# --------------------------------
# Reads hourly electricity load + PV generation profile from Excel,
# computes self-consumption, grid import/export, and key economic KPIs.
#
# Notes:
# - Assumes PV profile column is in Wh/m² per hour and load is in kWh per hour.
# - "payback_years" is a simple payback time (not ROI in %).

from __future__ import annotations

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

# ----------------------------
# Configuration / Inputs
# ----------------------------
PATH = "Desktop/PV_exercise.xlsx"
SHEET = "el_and_pv_data"

PV_AREA_M2 = 20

ELECTRICITY_PRICE_GRID_RP_PER_KWH = 27
ELECTRICITY_FEED_IN_TARIFF_RP_PER_KWH = 6

PV_SPECIFIC_INVESTMENT_COST_CHF_PER_M2 = 500  # https://solar-ratgeber.ch/photovoltaik/kosten-preise/
PV_SUBSIDY_FRACTION_OF_COST = 0.2  # https://www.stadtluzern.ch/dienstleistungeninformation/6253...


# Column names in your Excel file
COL_TIME = "Time"
COL_LOAD = "Electricity_Profile_MFH [kWh]"
COL_PV_PROFILE = "PV el generation profil [Wh/m2]"

# ----------------------------
# Helper functions
# ----------------------------
def read_input_dataframe(path: str, sheet: str) -> pd.DataFrame:
    """Read required columns, parse time, and return a clean DataFrame."""
    df = pd.read_excel(
        path,
        sheet_name=sheet,
        usecols=[COL_TIME, COL_LOAD, COL_PV_PROFILE],
    ).copy()

    # Parse and index by time (recommended for time series work)
    df[COL_TIME] = pd.to_datetime(df[COL_TIME], errors="coerce")
    if df[COL_TIME].isna().any():
        bad = df[df[COL_TIME].isna()]
        raise ValueError(f"Some time values could not be parsed. Example rows:\n{bad.head()}")

    df = df.sort_values(COL_TIME).set_index(COL_TIME)

    # Standardize column names
    df = df.rename(
        columns={
            COL_LOAD: "load_kWh",
            COL_PV_PROFILE: "pv_Wh_m2",
        }
    )
    return df



def compute_energy_flows(df: pd.DataFrame, pv_area_m2: float) -> pd.DataFrame:
    """Add PV generation (kWh), PV used, grid import, and grid export to df."""
    out = df.copy()

    # Total PV generation in kWh (Wh/m² * m² / 1000 = kWh)
    out["pv_kWh"] = out["pv_Wh_m2"] * pv_area_m2 / 1000.0

    # PV used on-site (self-consumption)
    out["pv_used_kWh"] = np.minimum(out["pv_kWh"], out["load_kWh"])

    # Grid import = remaining load
    out["grid_import_kWh"] = out["load_kWh"] - out["pv_used_kWh"]

    # Grid export = PV surplus (clip to avoid -0 from floating arithmetic)
    out["grid_export_kWh"] = (out["pv_kWh"] - out["pv_used_kWh"]).clip(lower=0)

    # Energy balance checks (optional but helpful)
    # PV split: pv_used + export == pv
    if not np.allclose((out["pv_used_kWh"] + out["grid_export_kWh"]).to_numpy(), out["pv_kWh"].to_numpy(), rtol=1e-9, atol=1e-9):
        raise AssertionError("PV energy balance check failed (pv_used + export != pv).")

    # Load split: pv_used + import == load
    if not np.allclose((out["pv_used_kWh"] + out["grid_import_kWh"]).to_numpy(), out["load_kWh"].to_numpy(), rtol=1e-9, atol=1e-9):
        raise AssertionError("Load energy balance check failed (pv_used + import != load).")

    return out


def economical_values(
    df: pd.DataFrame,
    electricity_price_grid_rp_per_kwh: float,
    feed_in_tariff_rp_per_kwh: float,
    pv_specific_investment_cost_chf_per_m2: float,
    pv_subsidy_fraction: float,
    pv_area_m2: float,
) -> dict:
    """Compute annual totals and economic KPIs."""
    rp_to_chf = 1 / 100.0
    price_grid_chf = electricity_price_grid_rp_per_kwh * rp_to_chf
    feed_in_chf = feed_in_tariff_rp_per_kwh * rp_to_chf

    pv_gen_kwh = float(df["pv_kWh"].sum())
    pv_used_kwh = float(df["pv_used_kWh"].sum())
    grid_import_kwh = float(df["grid_import_kWh"].sum())
    grid_export_kwh = float(df["grid_export_kWh"].sum())
    load_kwh = float(df["load_kWh"].sum())

    investment_cost_user_chf = pv_specific_investment_cost_chf_per_m2 * pv_area_m2 * (1 - pv_subsidy_fraction)

    # Self-consumption share of PV production (%)
    pv_self_consumption_share = (pv_used_kwh / pv_gen_kwh * 100.0) if pv_gen_kwh > 0 else 0.0

    bill_without_pv_chf_per_year = load_kwh * price_grid_chf
    bill_with_pv_chf_per_year = grid_import_kwh * price_grid_chf
    revenue_feed_in_chf_per_year = grid_export_kwh * feed_in_chf

    annual_benefit_chf = (bill_without_pv_chf_per_year - bill_with_pv_chf_per_year) + revenue_feed_in_chf_per_year
    payback_years = (investment_cost_user_chf / annual_benefit_chf) if annual_benefit_chf > 0 else np.inf

    return {
        "pv_generation_kWh": pv_gen_kwh,
        "pv_used_on_site_kWh": pv_used_kwh,
        "grid_import_kWh": grid_import_kwh,
        "grid_export_kWh": grid_export_kwh,
        "investment_cost_user_CHF": investment_cost_user_chf,
        "pv_self_consumption_share_%": pv_self_consumption_share,
        "bill_without_pv_CHF_per_year": bill_without_pv_chf_per_year,
        "bill_with_pv_CHF_per_year": bill_with_pv_chf_per_year,
        "revenue_feed_in_CHF_per_year": revenue_feed_in_chf_per_year,
        "annual_benefit_CHF_per_year": annual_benefit_chf,
        "payback_years": payback_years,
    }


def run_sensitivity_pv_area(
    df_base: pd.DataFrame,
    areas_m2: list[float],
) -> pd.DataFrame:
    """Compute KPIs for multiple PV areas (part g)."""
    rows = []
    for area in areas_m2:
        df_flows = compute_energy_flows(df_base, pv_area_m2=area)
        kpis = economical_values(
            df_flows,
            electricity_price_grid_rp_per_kwh=ELECTRICITY_PRICE_GRID_RP_PER_KWH,
            feed_in_tariff_rp_per_kwh=ELECTRICITY_FEED_IN_TARIFF_RP_PER_KWH,
            pv_specific_investment_cost_chf_per_m2=PV_SPECIFIC_INVESTMENT_COST_CHF_PER_M2,
            pv_subsidy_fraction=PV_SUBSIDY_FRACTION_OF_COST,
            pv_area_m2=area,
        )
        kpis["pv_area_m2"] = area
        rows.append(kpis)

    return pd.DataFrame(rows).set_index("pv_area_m2")


# ----------------------------
# Main
# ----------------------------
def main() -> None:
    df_base = read_input_dataframe(PATH, SHEET)

    # Compute flows for the chosen PV area
    df = compute_energy_flows(df_base, pv_area_m2=PV_AREA_M2)

    # Compute KPIs
    economics = economical_values(
        df,
        electricity_price_grid_rp_per_kwh=ELECTRICITY_PRICE_GRID_RP_PER_KWH,
        feed_in_tariff_rp_per_kwh=ELECTRICITY_FEED_IN_TARIFF_RP_PER_KWH,
        pv_specific_investment_cost_chf_per_m2=PV_SPECIFIC_INVESTMENT_COST_CHF_PER_M2,
        pv_subsidy_fraction=PV_SUBSIDY_FRACTION_OF_COST,
        pv_area_m2=PV_AREA_M2,
    )

    # Print results (similar to your script, but with clear labels + units)

    print(df)
    print(f"PV electricity generation [kWh]: {economics['pv_generation_kWh']:.2f}")
    print(f"Electricity covered by PV [kWh]: {economics['pv_used_on_site_kWh']:.2f}")
    print(f"Electricity bought from the grid [kWh]: {economics['grid_import_kWh']:.2f}")
    print(f"Electricity sold to the grid [kWh]: {economics['grid_export_kWh']:.2f}")

    print(f"Investment cost for the user [CHF]: {economics['investment_cost_user_CHF']:.2f}")
    print(f"Percentage of PV electricity directly used [%]: {economics['pv_self_consumption_share_%']:.2f}")

    print(f"Electricity bill without PV [CHF/year]: {economics['bill_without_pv_CHF_per_year']:.2f}")
    print(f"Electricity bill with PV [CHF/year]: {economics['bill_with_pv_CHF_per_year']:.2f}")
    print(f"Revenue from selling PV to grid [CHF/year]: {economics['revenue_feed_in_CHF_per_year']:.2f}")

    print(f"Annual benefit [CHF/year]: {economics['annual_benefit_CHF_per_year']:.2f}")
    print(f"Payback time [years]: {economics['payback_years']:.2f}")

    #sensitivity analysis for PV area (part g)
    areas = list(range(5, 30, 1))  # 0..100 m² in 1 m² steps
    df_sensitivity = run_sensitivity_pv_area(df_base, areas_m2=areas)
    kpis_to_plot = [
    "payback_years",
    "annual_benefit_CHF_per_year",
    "bill_with_pv_CHF_per_year",
    "revenue_feed_in_CHF_per_year",
    "pv_self_consumption_share_%",
    ]
    for kpi in kpis_to_plot:
        plt.figure()
        df_sensitivity[kpi].plot()
        plt.xlabel("PV area [m²]")
        plt.ylabel(kpi.replace("_", " "))
        plt.title(f"{kpi} vs PV area")
        plt.grid(True)
        plt.tight_layout()

    plt.show()




if __name__ == "__main__":
    main()