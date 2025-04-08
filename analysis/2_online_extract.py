from common import *

if __name__=="__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--name', type=str, required=True, help='name of the analysis channel')
    parser.add_argument('--cuts', type=str, required=True, help='name of the preselection cuts; llhh, vvhh, qqhh')
    parser.add_argument('--work_root',  type=str, required=True, help='directory where the output files will be stored')
    
    args = parser.parse_args()
    
    name = str(args.name)
    cuts = str(args.cuts)
    work_root = str(args.work_root)
    
    analysis = AnalysisChannel(name, zhh_cuts(cuts))
    analysis.initialize(work_root, trees=['FinalStates', 'EventObservablesLL', 'KinFitLLNMC', 'KinFitLLZHH', 'KinFitLLZZH', 'KinFitLLZZZ'])
    analysis.fetch(cuts[:2])
    
    rf = analysis.rf
    data = analysis.summary
    weights, processes = analysis.weight()
    
    