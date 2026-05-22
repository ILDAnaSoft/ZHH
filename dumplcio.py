from pyLCIO import IOIMPL
import sys

infile = sys.argv[1]
rdr = IOIMPL.LCFactory.getInstance().createLCReader()
rdr.open(infile)

for evt in rdr:
    col = evt.getCollection("MCParticle")
    
    # Create a counter for the total energy of the event
    total_energy = 0.0
    
    for p in col:
        # Add each particle's energy to the total
        total_energy += p.getEnergy()
        
    # Print only the final sum for each event
    print(f"Total MC-Truth Energy for Event {evt.getEventNumber()}: {total_energy:.2f} GeV")
