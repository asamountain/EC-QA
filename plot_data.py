#!/usr/bin/env python3
"""
BOQU IOT-485-EC4A Data Visualization Script
Compares Sensor Default EC vs Smart Algorithm EC across temperature range
Author: Senior Embedded Systems Engineer
"""

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import sys
from pathlib import Path

def load_data(filename='ec_data_log.csv'):
    """Load CSV data with error handling"""
    if not Path(filename).exists():
        print(f"❌ ERROR: {filename} not found!")
        print(f"   Make sure smart_logger has been running and generating data.")
        sys.exit(1)
    
    try:
        df = pd.read_csv(filename)
        print(f"✅ Loaded {len(df)} data points from {filename}")
        return df
    except Exception as e:
        print(f"❌ ERROR loading CSV: {e}")
        sys.exit(1)

def validate_data(df):
    """Check if data has required columns"""
    required_cols = ['Temperature', 'Sensor_Default_EC', 'Smart_Calc_EC']
    missing = [col for col in required_cols if col not in df.columns]
    
    if missing:
        print(f"❌ ERROR: Missing columns: {missing}")
        sys.exit(1)
    
    # Remove any NaN or invalid values
    df_clean = df.dropna(subset=required_cols)
    
    if len(df_clean) < len(df):
        print(f"⚠️  Removed {len(df) - len(df_clean)} rows with missing data")
    
    return df_clean

def calculate_statistics(df):
    """Calculate and display statistical analysis"""
    print("\n" + "="*60)
    print("📊 STATISTICAL ANALYSIS")
    print("="*60)
    
    sensor_ec = df['Sensor_Default_EC']
    smart_ec = df['Smart_Calc_EC']
    
    print(f"\n🔴 Sensor Default EC (k=0.02 fixed):")
    print(f"   Mean:     {sensor_ec.mean():.4f} mS/cm")
    print(f"   Std Dev:  {sensor_ec.std():.4f} mS/cm")
    print(f"   Min:      {sensor_ec.min():.4f} mS/cm")
    print(f"   Max:      {sensor_ec.max():.4f} mS/cm")
    print(f"   Range:    {sensor_ec.max() - sensor_ec.min():.4f} mS/cm")
    
    print(f"\n🟢 Smart Algorithm EC (Dynamic k):")
    print(f"   Mean:     {smart_ec.mean():.4f} mS/cm")
    print(f"   Std Dev:  {smart_ec.std():.4f} mS/cm")
    print(f"   Min:      {smart_ec.min():.4f} mS/cm")
    print(f"   Max:      {smart_ec.max():.4f} mS/cm")
    print(f"   Range:    {smart_ec.max() - smart_ec.min():.4f} mS/cm")
    
    improvement = ((sensor_ec.std() - smart_ec.std()) / sensor_ec.std()) * 100
    print(f"\n💡 Stability Improvement: {improvement:.2f}%")
    
    # Calculate deviation from expected 12.88 mS/cm
    expected = 12.88
    sensor_rmse = np.sqrt(((sensor_ec - expected) ** 2).mean())
    smart_rmse = np.sqrt(((smart_ec - expected) ** 2).mean())
    
    print(f"\n📏 RMSE from Expected (12.88 mS/cm):")
    print(f"   Sensor Default: {sensor_rmse:.4f} mS/cm")
    print(f"   Smart Algo:     {smart_rmse:.4f} mS/cm")
    print(f"   Improvement:    {((sensor_rmse - smart_rmse) / sensor_rmse) * 100:.2f}%")
    
    print("="*60 + "\n")
    
    return {
        'sensor_std': sensor_ec.std(),
        'smart_std': smart_ec.std(),
        'sensor_rmse': sensor_rmse,
        'smart_rmse': smart_rmse
    }

def plot_comparison(df, stats):
    """Generate comparison visualization"""
    
    # Sort by temperature for cleaner plot
    df_sorted = df.sort_values('Temperature')
    
    # Create figure with two subplots
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(14, 10))
    fig.suptitle('BOQU IOT-485-EC4A: Sensor Default vs Smart Algorithm Comparison', 
                 fontsize=16, fontweight='bold')
    
    # ===== PLOT 1: EC vs Temperature =====
    ax1.plot(df_sorted['Temperature'], df_sorted['Sensor_Default_EC'], 
             'r-', linewidth=2, label='Sensor Default (k=0.02 fixed)', alpha=0.7)
    ax1.plot(df_sorted['Temperature'], df_sorted['Smart_Calc_EC'], 
             'g-', linewidth=2, label='Smart Algorithm (Dynamic k)', alpha=0.7)
    
    # Add expected reference line
    ax1.axhline(y=12.88, color='blue', linestyle='--', linewidth=1.5, 
                label='Expected: 12.88 mS/cm @ 25°C', alpha=0.5)
    
    ax1.set_xlabel('Temperature (°C)', fontsize=12)
    ax1.set_ylabel('Conductivity (mS/cm)', fontsize=12)
    ax1.set_title('Conductivity vs Temperature', fontsize=13, fontweight='bold')
    ax1.legend(loc='best', fontsize=10)
    ax1.grid(True, alpha=0.3)
    
    # Add statistics text box
    textstr = f'Sensor Std: {stats["sensor_std"]:.4f} mS/cm\n' \
              f'Smart Std:  {stats["smart_std"]:.4f} mS/cm\n' \
              f'Sensor RMSE: {stats["sensor_rmse"]:.4f} mS/cm\n' \
              f'Smart RMSE:  {stats["smart_rmse"]:.4f} mS/cm'
    
    props = dict(boxstyle='round', facecolor='wheat', alpha=0.5)
    ax1.text(0.02, 0.98, textstr, transform=ax1.transAxes, fontsize=10,
             verticalalignment='top', bbox=props)
    
    # ===== PLOT 2: Deviation Analysis =====
    deviation = df_sorted['Sensor_Default_EC'] - df_sorted['Smart_Calc_EC']
    ax2.plot(df_sorted['Temperature'], deviation, 'b-', linewidth=2, alpha=0.7)
    ax2.axhline(y=0, color='black', linestyle='-', linewidth=1, alpha=0.3)
    ax2.fill_between(df_sorted['Temperature'], deviation, 0, alpha=0.3)
    
    ax2.set_xlabel('Temperature (°C)', fontsize=12)
    ax2.set_ylabel('Deviation (Sensor - Smart) [mS/cm]', fontsize=12)
    ax2.set_title('Algorithm Deviation Analysis', fontsize=13, fontweight='bold')
    ax2.grid(True, alpha=0.3)
    
    # Add mean deviation line
    mean_dev = deviation.mean()
    ax2.axhline(y=mean_dev, color='red', linestyle='--', linewidth=1.5, 
                label=f'Mean Deviation: {mean_dev:.4f} mS/cm', alpha=0.7)
    ax2.legend(loc='best', fontsize=10)
    
    plt.tight_layout()
    
    # Save figure
    output_file = 'ec_comparison_chart.png'
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"📈 Chart saved to: {output_file}")
    
    # Show plot
    plt.show()

def plot_coefficient_analysis(df):
    """Generate coefficient usage visualization"""
    if 'Coefficient_Used' not in df.columns:
        print("⚠️  Coefficient data not available in CSV")
        return
    
    df_sorted = df.sort_values('Temperature')
    
    fig, ax = plt.subplots(figsize=(12, 6))
    
    # Plot coefficient vs temperature
    ax.plot(df_sorted['Temperature'], df_sorted['Coefficient_Used'] * 100, 
            'purple', linewidth=2, marker='o', markersize=3, alpha=0.7)
    
    # Add temperature zone boundaries
    ax.axvline(x=5, color='gray', linestyle='--', alpha=0.3)
    ax.axvline(x=10, color='gray', linestyle='--', alpha=0.3)
    ax.axvline(x=15, color='gray', linestyle='--', alpha=0.3)
    ax.axvline(x=25, color='gray', linestyle='--', alpha=0.3)
    
    # Add zone labels
    ax.text(2.5, ax.get_ylim()[1]*0.95, '1.80%', fontsize=9, ha='center')
    ax.text(7.5, ax.get_ylim()[1]*0.95, '1.84%', fontsize=9, ha='center')
    ax.text(12.5, ax.get_ylim()[1]*0.95, '1.90%', fontsize=9, ha='center')
    ax.text(20, ax.get_ylim()[1]*0.95, '1.90%', fontsize=9, ha='center')
    
    ax.set_xlabel('Temperature (°C)', fontsize=12)
    ax.set_ylabel('Coefficient k (%)', fontsize=12)
    ax.set_title('Dynamic Temperature Coefficient Usage', fontsize=13, fontweight='bold')
    ax.grid(True, alpha=0.3)
    
    plt.tight_layout()
    
    output_file = 'coefficient_analysis.png'
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"📊 Coefficient analysis saved to: {output_file}")
    
    plt.show()

def main():
    print("\n" + "="*60)
    print("🎨 BOQU IOT-485-EC4A DATA VISUALIZATION")
    print("="*60 + "\n")
    
    # Load and validate data
    df = load_data('ec_data_log.csv')
    df = validate_data(df)
    
    if len(df) == 0:
        print("❌ ERROR: No valid data to plot!")
        sys.exit(1)
    
    # Calculate statistics
    stats = calculate_statistics(df)
    
    # Generate plots
    print("📊 Generating comparison chart...")
    plot_comparison(df, stats)
    
    print("\n📊 Generating coefficient analysis...")
    plot_coefficient_analysis(df)
    
    print("\n✅ Visualization complete!")
    print("   Charts saved: ec_comparison_chart.png, coefficient_analysis.png")

if __name__ == '__main__':
    main()
