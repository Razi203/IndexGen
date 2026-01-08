import pandas as pd
import os
import sys

def create_excel_report():
    print("Generating Excel report...")
    
    # File paths
    clusters_csv = 'clusters.csv'
    hist_csv = 'histogram.csv'
    inter_csv = 'intersections.csv'
    output_xlsx = 'clustering_report.xlsx'

    # Check if files exist
    if not all(os.path.exists(f) for f in [clusters_csv, hist_csv, inter_csv]):
        print("Error: One or more input CSV files are missing.")
        sys.exit(1)

    # Read data
    df_clusters = pd.read_csv(clusters_csv)
    df_hist = pd.read_csv(hist_csv)
    df_inter = pd.read_csv(inter_csv)

    # Create Summary Statistics Data
    summary_data = {
        'Metric': [
            'Total Clusters',
            'Total Intersections',
            'Min Radius',
            'Max Radius',
            'Avg Radius',
            'Min Cluster Size',
            'Max Cluster Size',
            'Avg Cluster Size'
        ],
        'Value': [
            len(df_clusters),
            len(df_inter),
            df_clusters['Radius'].min(),
            df_clusters['Radius'].max(),
            df_clusters['Radius'].mean(),
            df_clusters['MemberCount'].min(),
            df_clusters['MemberCount'].max(),
            df_clusters['MemberCount'].mean()
        ]
    }
    df_summary = pd.DataFrame(summary_data)

    # Write to Excel with multiple sheets
    with pd.ExcelWriter(output_xlsx, engine='openpyxl') as writer:
        df_summary.to_excel(writer, sheet_name='Summary', index=False)
        df_clusters.to_excel(writer, sheet_name='Clusters', index=False)
        df_hist.to_excel(writer, sheet_name='Histogram', index=False)
        df_inter.to_excel(writer, sheet_name='Intersections', index=False)

    print(f"Report saved to {output_xlsx}")

if __name__ == "__main__":
    create_excel_report()
