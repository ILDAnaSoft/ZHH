/*! \file
 *  \brief Declares class ZinvisibleFitObjectNew
 *
 */

#ifndef __ZINVISIBLEFITOBJECTNEW_H
#define __ZINVISIBLEFITOBJECTNEW_H

#include "ParticleFitObject.h"
#include <cmath>

// Class ZinvisibleFitObjectNew
/// Class for Z->neutrinos with (px, py, pz) in kinematic fits

class ZinvisibleFitObjectNew : public ParticleFitObject {
  public:
    ZinvisibleFitObjectNew(double px, double py, double pz, 
			double Dpx=1, double Dpy=1, double Dpz=1, double m = 91.1876); 

    /// Copy constructor
    ZinvisibleFitObjectNew (const ZinvisibleFitObjectNew& rhs              ///< right hand side
		       );

    /// Assignment
    ZinvisibleFitObjectNew& operator= (const ZinvisibleFitObjectNew& rhs   ///< right hand side
				  );

    virtual ~ZinvisibleFitObjectNew();
    
    /// Return a new copy of itself
    virtual ZinvisibleFitObjectNew *copy() const;
    
    /// Assign from anther object, if of same type
    virtual ZinvisibleFitObjectNew& assign (const BaseFitObject& source   ///< The source object
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

#endif // __ZINVISIBLEFITOBJECTNEW_H

