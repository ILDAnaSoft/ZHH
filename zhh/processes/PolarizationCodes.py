class POLARIZATION_CODES:
    EL_PL = 0
    EL_PR = 1
    ER_PL = 2
    ER_PR = 3
    
PolarizationCodes = POLARIZATION_CODES()

def parse_polarization_code(polarization_code:int) -> tuple[int, int]:
    Pem = -1 if (polarization_code == PolarizationCodes.EL_PL or polarization_code == PolarizationCodes.EL_PR) else 1
    Pep = -1 if (polarization_code == PolarizationCodes.EL_PL or polarization_code == PolarizationCodes.ER_PL) else 1
    
    return Pem, Pep