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
canvas = TCanvas("canvas", "", 800, 600)

tree.Draw("FitErrorCode")
canvas.SaveAs("FitErrorCode_plot.png")

tree.Draw("ZMassBeforeFit")
canvas.SaveAs("ZMassBeforeFit_plot.png")

tree.Draw("HHMassBeforeFit")
canvas.SaveAs("HHMassBeforeFit_plot.png")

tree.Draw("ZMassAfterFit")
canvas.SaveAs("ZMassAfterFit_plot.png")

tree.Draw("HHMassAfterFit")
canvas.SaveAs("HHMassAfterFit_plot.png")

tree.Draw("FitProbability")
canvas.SaveAs("FitProbability_plot.png")

tree.Draw("FitChi2")
canvas.SaveAs("FitChi2_plot.png")

tree.Draw("pullJetEnergy")
canvas.SaveAs("pullJetEnergy_plot.png")