/*! \file
 *  \brief Implements clsss ZinvisibleFitObjectNew
 *  class for Z->neutrinos with (E, theta, phi) in kinematic fit
 *  Zinvisible works similar to NeutrinoFitObject, but its mass is set to 91.1876
 *  developed for ZHH->vvHH, ZinvisibleFO represents Z->vv in fit
 *
 */

#include "ZinvisibleFitObjectNew.h"
#include <cmath>

#undef NDEBUG
#include <cassert>

#include <algorithm>

using std::sqrt;
using std::sin;
using std::cos;
using std::cout; 
using std::endl;

// constructor
ZinvisibleFitObjectNew::ZinvisibleFitObjectNew(double px_, double py_, double pz_, 
					 double Dpx, double Dpy, double Dpz, double m) 
  : cachevalid(false), p2(0), p(0), dpdE(0), pt(0), pt2(0), px(0), py(0), pz(0), e(0), chi2(0)

{  //hier double m

  assert( int(NPAR) <= int(BaseDefs::MAXPAR) );
  setMass (m);  
  setParam (0, px_, false);
  setParam (1, py_, false);
  setParam (2, pz_, false);
  setError (0, Dpx);
  setError (1, Dpy);
  setError (2, Dpz);
  invalidateCache();
}

// destructor
ZinvisibleFitObjectNew::~ZinvisibleFitObjectNew() {}

ZinvisibleFitObjectNew::ZinvisibleFitObjectNew (const ZinvisibleFitObjectNew& rhs)
  : cachevalid(false), p2(0), p(0), dpdE(0), pt(0), pt2(0), px(0), py(0), pz(0), e(0), chi2(0)
{
  //std::cout << "copying ZinvisibleFitObjectNew with name" << rhs.name << std::endl;
  ZinvisibleFitObjectNew::assign (rhs);
}

ZinvisibleFitObjectNew& ZinvisibleFitObjectNew::operator= (const ZinvisibleFitObjectNew& rhs) {
  if (this != &rhs) {
    assign (rhs); // calls virtual function assign of derived class
  }
  return *this;
}

ZinvisibleFitObjectNew *ZinvisibleFitObjectNew::copy() const {
  return new ZinvisibleFitObjectNew (*this);
}
    
ZinvisibleFitObjectNew& ZinvisibleFitObjectNew::assign (const BaseFitObject& source) {
  if (const ZinvisibleFitObjectNew *psource = dynamic_cast<const ZinvisibleFitObjectNew *>(&source)) {
    if (psource != this){
      ParticleFitObject::assign (source);
      // only mutable data members, need not to be copied, if cache is invalid
    }
  }
  else {
    assert (0);
  }
  return *this;
}

const char *ZinvisibleFitObjectNew::getParamName (int ilocal) const {
  switch (ilocal) {
    case 0: return "px";
    case 1: return "py";
    case 2: return "pz";
  }
  return "undefined";
}

bool ZinvisibleFitObjectNew::updateParams (double pp[], int idim) {

  invalidateCache();
  int ipx = getGlobalParNum(0);
  int ipy = getGlobalParNum(1);
  int ipz = getGlobalParNum(2);
  assert (ipx >= 0 && ipx < idim);
  assert (ipy >= 0 && ipy < idim);
  assert (ipz >= 0 && ipz < idim);
  
  px = pp[ipx];
  py = pp[ipy];
  pz = pp[ipz];

  
  bool result = (px-par[0])*(px-par[0]) > eps2*cov[0][0] ||
                (py-par[1])*(py-par[1]) > eps2*cov[1][1] ||
                (pz-par[2])*(pz-par[2]) > eps2*cov[2][2];

  par[0] = px;
  par[1] = py;
  par[2] = pz; 
  return result;
}  

// these depend on actual parametrisation!
double ZinvisibleFitObjectNew::getPx() const {
  if (!cachevalid) updateCache();
  return px;
}
double ZinvisibleFitObjectNew::getPy() const {
  if (!cachevalid) updateCache();
  return py;
}
double ZinvisibleFitObjectNew::getPz() const {
  if (!cachevalid) updateCache();
  return pz;
}
double ZinvisibleFitObjectNew::getE() const {
  if (!cachevalid) updateCache();
  return e;
}

double ZinvisibleFitObjectNew::getP() const {
    if (!cachevalid) updateCache();
    return p; 
}

double ZinvisibleFitObjectNew::getP2() const {
  if (!cachevalid) updateCache();
   return p2; 
}
double ZinvisibleFitObjectNew::getPt() const {
  if (!cachevalid) updateCache();
  return pt;
}
double ZinvisibleFitObjectNew::getPt2() const {
  if (!cachevalid) updateCache();
  return pt2;
}

double ZinvisibleFitObjectNew::getDPx(int ilocal) const {
  assert (ilocal >= 0 && ilocal < NPAR);
  if (!cachevalid) updateCache();
  switch (ilocal) {
    case 0: return 1;
    case 1: return 0;
    case 2: return 0;
  }
  return 0; 
}

double ZinvisibleFitObjectNew::getDPy(int ilocal) const {
  assert (ilocal >= 0 && ilocal < NPAR);
  if (!cachevalid) updateCache();
  switch (ilocal) {
    case 0: return 0;
    case 1: return 1;
    case 2: return 0;
  }
  return 0; 
}

double ZinvisibleFitObjectNew::getDPz(int ilocal) const {
  assert (ilocal >= 0 && ilocal < NPAR);
  if (!cachevalid) updateCache();
  switch (ilocal) {
    case 0: return 0;
    case 1: return 0;
    case 2: return 1;
  }
  return 0; 
}

double ZinvisibleFitObjectNew::getDE(int ilocal) const {
  assert (ilocal >= 0 && ilocal < NPAR);
  switch (ilocal) {
    case 0: return px/e;  // dE/dpx
    case 1: return py/e;  // dE/dpy
    case 2: return pz/e;  // dE/dpz
  }
  return 0; 
}

double ZinvisibleFitObjectNew::getFirstDerivative_Meta_Local( int iMeta, int ilocal , int metaSet ) const {
  // iMeta = intermediate variable (i.e. E,px,py,pz)
  // ilocal = local variable (px, py, pz)
  // metaSet = which set of intermediate varlables

  assert (metaSet==0); // only defined for E,px,py,pz

  switch ( iMeta ) {
  case 0: // E
    return getDE(ilocal);
    break;
  case 1: // Px
    return getDPx(ilocal);
    break;
  case 2: // Py
    return getDPy(ilocal);
    break;
  case 3: // Pz
    return getDPz(ilocal);
    break;
  default:
    assert(0);
  }
  return -999;
}

double ZinvisibleFitObjectNew::getSecondDerivative_Meta_Local( int iMeta, int ilocal , int jlocal , int metaSet ) const {
  // iMeta = intermediate variable (i.e. E,px,py,pz)
  // ilocal, jlocal = local variable (px, py, pz)
  // metaSet = which set of intermediate varlables

  assert ( metaSet==0 );
  if (!cachevalid) updateCache();

  if ( jlocal<ilocal ) {
    int temp=jlocal;
    jlocal=ilocal;
    ilocal=temp;
  }

  // calculated by Jenny, please double check!
  switch ( iMeta ) {
  
    //double e3 = e*e*e;

  case 0:
    if      ( ilocal==0 && jlocal==0 ) return (e*e-px*px)/(e*e*e);  // d2E/dx2
    else if ( ilocal==0 && jlocal==1 ) return -px*py/(e*e*e);       // d2E/dxdy
    else if ( ilocal==0 && jlocal==2 ) return -px*pz/(e*e*e);       // d2E/dxdz
    else if ( ilocal==1 && jlocal==1 ) return (e*e-py*py)/(e*e*e);  // d2E/dy2
    else if ( ilocal==1 && jlocal==2 ) return -py*pz/(e*e*e);       // d2E/dydz
    else if ( ilocal==2 && jlocal==2 ) return (e*e-pz*pz)/(e*e*e);  // d2E/dz2
    else return 0;
    break;
  case 1:  // d2px/dpidpj == 0
    return 0;
    break;
  case 2: // d2py/dpidpj == 0
    return 0;
    break;
  case 3: // d2pz/dpidpj == 0
    return 0;
    break;
  default:
    assert(0);
  }
  return -999;
}

void ZinvisibleFitObjectNew::invalidateCache() const {
  cachevalid = false;
}
    
void ZinvisibleFitObjectNew::updateCache() const {
  px = par[0];
  py = par[1];
  pz = par[2];
  
  pt2 = px*px+py*py;
  pt = std::sqrt(pt2);
  p2 = pt2+pz*pz;
  p  = std::sqrt(p2);
  e  = std::sqrt(p2+mass*mass);
  
  cachevalid = true;
}
