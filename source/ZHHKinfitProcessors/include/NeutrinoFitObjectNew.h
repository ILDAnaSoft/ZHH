/*! \file
 *  \brief Declares class NeutrinoFitObjectNew
 *
 */

#ifndef __NEUTRINOFITOBJECTNEW_H
#define __NEUTRINOFITOBJECTNEW_H

#include "ParticleFitObject.h"
#include <cmath>

// Class NeutrinoFitObjectNew
/// Class for W->neutrinos with (px, py, pz) in kinematic fits

class NeutrinoFitObjectNew : public ParticleFitObject {
  public:
    NeutrinoFitObjectNew(double px, double py, double pz, 
			double Dpx=1, double Dpy=1, double Dpz=1, double m = 0.0); 

    /// Copy constructor
    NeutrinoFitObjectNew (const NeutrinoFitObjectNew& rhs              ///< right hand side
		       );

    /// Assignment
    NeutrinoFitObjectNew& operator= (const NeutrinoFitObjectNew& rhs   ///< right hand side
				  );

    virtual ~NeutrinoFitObjectNew();
    
    /// Return a new copy of itself
    virtual NeutrinoFitObjectNew *copy() const;
    
    /// Assign from anther object, if of same type
    virtual NeutrinoFitObjectNew& assign (const BaseFitObject& source   ///< The source object
                                      );
    
    /// Get name of parameter ilocal
    virtual const char *getParamName (int ilocal     ///< Local parameter number
                                     ) const;
    
    /// Read values from global vector, readjust vector; return: significant change
    virtual bool   updateParams (double p[],   ///< The parameter vector
                                 int idim      ///< Length of the vector                         
                                );  
    
    // these depend on actual parametrisation!
    virtual double getPx() const;
    virtual double getPy() const;
    virtual double getPz() const;
    virtual double getE() const;
    virtual double getPt() const;
    virtual double getP2() const;
    virtual double getPt2() const;
    virtual double getDPx(int ilocal) const;
    virtual double getDPy(int ilocal) const;
    virtual double getDPz(int ilocal) const;
    virtual double getDE(int ilocal) const;
    
    virtual void invalidateCache() const;

    virtual double getFirstDerivative_Meta_Local( int iMeta, int ilocal , int metaSet ) const;
    virtual double getSecondDerivative_Meta_Local( int iMeta, int ilocal , int jlocal , int metaSet ) const;      
    virtual int getNPar() const {return NPAR;}

  protected:
   inline  double getP() const;
    
    void updateCache() const;

    enum {NPAR=3};
  
    mutable bool cachevalid;
    
    mutable double p2, p, dpdE, pt, pt2, px, py, pz, e, chi2;

};

#endif // __NEUTRINOFITOBJECTNEW_H