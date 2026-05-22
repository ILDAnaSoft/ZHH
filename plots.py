import ROOT

# For non-interactive use, set ROOT to batch mode to avoid opening windows
ROOT.gROOT.SetBatch(True)

# Open the file safely
from ROOT import TFile
file = TFile("zhh_AIDA.root")
tree = file.Get("KinFitQQ_NMC")

print(f"Entries: {tree.GetEntries()} events.")
tree.Show(5)  # Show the 6th entries to verify the structure

tree.Scan("nJets:nLeptons")  # tree.Scan("variables", "selection (eg Z_m > 200)", "(options)", n_of_entries, first_entry)
    
# Plotting
from ROOT import TCanvas
canvas = TCanvas("canvas", "FitErrorCode", 800, 600)

tree.Draw("FitErrorCode")
canvas.SaveAs("FitErrorCode_plot.png")