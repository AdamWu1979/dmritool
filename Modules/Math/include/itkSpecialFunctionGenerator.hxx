/**
 *       @file  itkSpecialFunctionGenerator.hxx
 *      @brief  
 *
 *
 *     @author  Dr. Jian Cheng (JC), jian.cheng.1983@gmail.com
 *
 *   @internal
 *     Created  "12-29-2012
 *    Revision  1.0
 *    Compiler  gcc/g++
 *     Company  IDEA@UNC-CH
 *   Copyright  Copyright (c) 2012, Jian Cheng
 *
 * =====================================================================================
 */

#ifndef __itkSpecialFunctionGenerator_hxx
#define __itkSpecialFunctionGenerator_hxx


#include "itkSpecialFunctionGenerator.h"
#include <gsl/gsl_sf_gamma.h>
#include <gsl/gsl_sf_laguerre.h>
#include <gsl/gsl_sf_bessel.h>
#include <tr1/cmath>
#include <tr1/memory>
#include "utl.h"

#include "itkSpecialFunctionGenerator.h"

namespace utl
{



template < class T >
T
Lagurre (const int n, const double a, const T x )
{
  if (n==1)
    return gsl_sf_laguerre_1 (a,x); 
  else if (n==2)
    return gsl_sf_laguerre_2 (a,x); 
  else if (n==3)
    return gsl_sf_laguerre_3 (a,x); 
  else
    return gsl_sf_laguerre_n(n,a,x);
}    

double 
Gamma(const double x)
{
  utlException(std::fabs(x)<1e-8, "wrong input x, x=0");

  if (utl::IsInt(2*x) && x>0)
    return GammaHalfInteger(x);
  else
    return gsl_sf_gamma(x);
}

double 
GammaLower ( const double s, const double x )
{
  double gamma_whole = Gamma(s);
  double gamma_upper = gsl_sf_gamma_inc(s,x);
  return gamma_whole - gamma_upper;
}

double
BesselJa(const double a, const double x)
{
  int int_a = int(a);
  int int_2a = int(2.0*a);
  if (std::abs(int_a-a)<1e-8) // integer
    {
    if (a<0)
      return (int_a%2?(-1):1)*BesselJa(-a,x);
    if (int_a==0)
      return gsl_sf_bessel_J0(x);
    else if (int_a==1)
      return gsl_sf_bessel_J1(x);
    else 
      return gsl_sf_bessel_Jn(int_a, x);
    }
  else if (std::abs(int_2a-2.0*a)<1e-8) // half of an intger
    {
    utlException (a<0, "the parameter a should be positive in this routine. \
      Note: a can be negative in theory... \n\
      When a is an integer, J_a(x)=(-1)^a J_{-a}(x); When a is not an integer, J_a(x) and J_{-a}(x) are linearly independent.");
    // j_l(x) = \sqrt(\pi / (2x)) J_{l+0.5}(x)
    return std::sqrt(2.0*x/M_PI) * gsl_sf_bessel_jl(int(a-0.5),x);
    }
  else  // use std::tr1::cyl_bessel_j  for non-negative order.
    utlException(true, "a should be an integer or a half of an integer!");
  return 0;
}


template<class T> 
NDArray<T,1>
GetRotatedSHCoefficient(const NDArray<T,1>& shInput, const NDArray<T,2>& rotationMatrix)
{
  typedef NDArray<double,2> MatrixDouble;
  typedef NDArray<double,1> VectorDouble;
  static utl_shared_ptr<MatrixDouble > gradt3 = utl::ReadGrad<double>(3, DIRECTION_NODUPLICATE, CARTESIAN_TO_CARTESIAN);
  // static int rank = utl::DimToRankSH(shInput.size());
  static int rank = 10;
  static utl_shared_ptr< MatrixDouble> shMatrix = utl::ComputeSHMatrix(rank, *gradt3, CARTESIAN_TO_SPHERICAL);
  static std::vector< MatrixDouble > shMatrixInv(rank/2);
  static bool isFirstTime = true; 
  if (isFirstTime)
    {
    for ( int l = 2; l <= rank; l += 2 ) 
      {
      MatrixDouble shMatrixL;
      shMatrix->GetNColumns(utl::RankToDimSH(l-2), 2*l+1, shMatrixL);
      utl::PInverseMatrix(shMatrixL, shMatrixInv[l/2-1]);
      }
    isFirstTime = false;
    }

  int rankReal = utl::DimToRankSH(shInput.Size());
  if (rankReal>rank)
    {
    rank = rankReal;
    shMatrix = utl::ComputeSHMatrix(rank, *gradt3, CARTESIAN_TO_SPHERICAL);
    shMatrixInv = std::vector<MatrixDouble >(rank/2);
    for ( int l = 2; l <= rank; l += 2 )
      {
      MatrixDouble shMatrixL;
      shMatrix->GetNColumns(utl::RankToDimSH(l-2), 2*l+1, shMatrixL);
      utl::PInverseMatrix(shMatrixL, shMatrixInv[l/2-1]);
      }
    }

  MatrixDouble gradt3Rotated = (rotationMatrix.GetTranspose() * gradt3->GetTranspose()).GetTranspose();
  utl_shared_ptr<MatrixDouble > shMatrixRotated = utl::ComputeSHMatrix(rankReal, gradt3Rotated,CARTESIAN_TO_SPHERICAL);
  VectorDouble shRotated(shInput);
  MatrixDouble tmp;
  for ( int l = 2; l <= rankReal; l += 2 ) 
    {
    int colstart = utl::RankToDimSH(l-2);
    shMatrixRotated->GetNColumns(colstart, 2*l+1, tmp);
    VectorDouble sfValueRotatedL =  tmp* shInput.GetSubVector(utl::GetRange(colstart, colstart+2*l+1));
    VectorDouble shRotatedL = shMatrixInv[l/2-1]*sfValueRotatedL;
    shRotated.SetSubVector(utl::GetRange(colstart, colstart+shRotatedL.Size()), shRotatedL);
    }
  return shRotated;
}

template < class T >
std::vector<T>
GetSymmetricTensorSHCoef ( const T b, const T e1, const T e2, const int lMax, const T theta, const T phi )
{
  utlException(!utl::IsInt(0.5*lMax), "lMax should be even, lMax="<<lMax);
  utlException(e1<e2-1e-10, "e1 should be more than e2, e1="<<e1 << ", e2="<<e2);

  std::vector<T> coef_vec(utl::RankToDimSH(lMax),0.0);
  double a = (e1 - e2)*b; 
  double expbe2 = std::exp(-b*e2);

  for ( int l = 0; l <= lMax; l += 2 ) 
    {
    double A = GetExpLegendreCoef(a,l);
    for ( int m = -l; m <= l; m += 1 ) 
      {
      int jj = utl::GetIndexSHj(l,m);
      coef_vec[jj] = 4.0*M_PI/(2.0*l+1.0) * expbe2 * A * itk::SphericalHarmonicsGenerator<double>::RealSH(l,m,theta, phi);
      }
    }
  return coef_vec;
}   // -----  end of method SpecialFunctions<T>::<method>  -----

template < class T >
std::vector< std::vector<T> >
GetSymmetricTensorSHCoefDerivative ( const T b, const T e1, const T e2, const int lMax, const T theta, const T phi )
{
  utlException(!utl::IsInt(0.5*lMax), "lMax should be even, lMax="<<lMax);
  utlException(e1<e2-1e-10, "e1 should be more than e2, e1="<<e1 << ", e2="<<e2);

  std::vector<std::vector<T> > coef(2);
  coef[0] = std::vector<T>(utl::RankToDimSH(lMax),0.0);
  coef[1] = std::vector<T>(utl::RankToDimSH(lMax),0.0);

  double a = (e1 - e2)*b; 
  double expbe2 = std::exp(-b*e2);

  for ( int l = 0; l <= lMax; l += 2 ) 
    {
    double A = GetExpLegendreCoef(a,l);
    double dA = GetExpLegendreCoefDerivative(a,l);
    for ( int m = -l; m <= l; m += 1 ) 
      {
      int jj = utl::GetIndexSHj(l,m);
      coef[0][jj] = 4.0*M_PI/(2.0*l+1.0) * expbe2 * b * dA * itk::SphericalHarmonicsGenerator<double>::RealSH(l,m,theta, phi);
      coef[1][jj] = 4.0*M_PI/(2.0*l+1.0) * expbe2 * (-b) * (A + dA);
      }
    }
  return coef;
}   // -----  end of method SpecialFunctions<T>::<method>  -----


double 
GetExpLegendreCoef ( const double a, const int l )
{
  // coefficient for odd order is 0;
  if (!utl::IsInt(0.5*l))
    return 0;
  
  if (std::fabs(a)<1e-10)
    return l==0 ? 1.0 : 0.0;

  double result = -1000.0;
  switch ( l )
    {
    case 0 :
       result = (std::sqrt(M_PI)*std::tr1::erf(std::sqrt(a)))/(2.*std::sqrt(a));
       break;
    case 2 :
       result = (5*(-3/(2.*a*std::pow(M_E,a)) + ((3 - 2*a)*std::sqrt(M_PI)*std::tr1::erf(std::sqrt(a)))/(4.*std::pow(a,1.5))))/2. ;
       break;
    case 4 :
       result = (9*((-5*(21 + 2*a))/(16.*std::pow(a,2)*std::pow(M_E,a)) + (3*(35 + 4*(-5 + a)*a)*std::sqrt(M_PI)*std::tr1::erf(std::sqrt(a)))/(32.*std::pow(a,2.5))))/2. ;
       break;
    case 6 : 
       result = (13*(-42*std::sqrt(a)*(165 + 20*a + 4*std::pow(a,2)) - 5*(-693 + 378*a - 84*std::pow(a,2) + 8*std::pow(a,3))*std::pow(M_E,a)*std::sqrt(M_PI)*std::tr1::erf(std::sqrt(a))))/(256.*std::pow(a,3.5)*std::pow(M_E,a));
       break;
    case 8 :
       result = (17*(-6*std::sqrt(a)*(225225 + 2*a*(15015 + 2*a*(1925 + 62*a))) + 35*(19305 + 8*a*(-1287 + a*(297 + 2*(-18 + a)*a)))*std::pow(M_E,a)*std::sqrt(M_PI)*std::tr1::erf(std::sqrt(a))))/(4096.*std::pow(a,4.5)*std::pow(M_E,a)) ;
       break;
    case 10 : 
       result = (21*(-22*std::sqrt(a)*(3968055 + 556920*a + 157248*std::pow(a,2) + 7488*std::pow(a,3) + 464*std::pow(a,4)) - 63*(-692835 + 364650*a - 85800*std::pow(a,2) + 11440*std::pow(a,3) - 880*std::pow(a,4) + 32*std::pow(a,5))*std::pow(M_E,a)*std::sqrt(M_PI)*std::tr1::erf(std::sqrt(a))))/(16384.*std::pow(a,5.5)*std::pow(M_E,a)) ; 
      break;
    case 12 : 
      result = (25*(-26*std::sqrt(a)*(540571185 + 2*a*(39171825 + 4*a*(2909907 + 2*a*(82467 + a*(7469 + 122*a))))) + 231*(30421755 + 4*a*(-3968055 + a*(944775 + 4*a*(-33150 + a*(2925 + 4*(-39 + a)*a)))))*std::pow(M_E,a)*std::sqrt(M_PI)*std::tr1::erf(std::sqrt(a))))/(131072.*std::pow(a,6.5)*std::pow(M_E,a)) ; 
      break;
    case 14 : 
      result = (29*(-2*std::sqrt(a)*(677644592625 + 100391791500*a + 30786816060*std::pow(a,2) + 1928852640*std::pow(a,3) + 206187696*std::pow(a,4) + 5360576*std::pow(a,5) + 158528*std::pow(a,6)) - 429*(-1579591125 + 819047250*a - 196571340*std::pow(a,2) + 28488600*std::pow(a,3) - 2713200*std::pow(a,4) + 171360*std::pow(a,5) - 6720*std::pow(a,6) + 128*std::pow(a,7))*std::pow(M_E,a)*std::sqrt(M_PI)*std::tr1::erf(std::sqrt(a))))/(524288.*std::pow(a,7.5)*std::pow(M_E,a)) ; 
      break;
    case 16 :
      result = (33*(-34*std::sqrt(a)*(3583544051587.5e1 + 2*a*(269729122162.5e1 + 2*a*(42253133422.5e1 + 2*a*(1413077737.5e1 + 2.0*a*(82956802.5e1 + 2*a*(1343803.5e1 + 62415.0e1*a + 6196*std::pow(a,2))))))) + 6435*(94670161425 + 16*a*(-305387617.5e1 + a*(73714252.5e1 + 2*a*(-5460315.0e1 + a*(5460315 + 8*a*(-47481 + a*(2261 + (-68 + a)*a)))))))*std::pow(M_E,a)*std::sqrt(M_PI)*std::tr1::erf(std::sqrt(a))))/(1.6777216e7*std::pow(a,8.5)*std::pow(M_E,a)) ; 
      break;
    case 18 : 
      result = (3.7e1*(-11.4e1*std::sqrt(a)*(137159624664562.5e1 + 8*a*(2612564279325.0e1 + a*(831270452512.5e1 + 4*a*(14556809767.5e1 + 2*a*(907887337.5e1 + 2*a*(16773900.0e1 + a*(961273.5e1 + 2*a*(7939.0e1 + 136.3e1*a)))))))) - 1215.5e1*(-643200214387.5e1 + 2*a*(165394340842.5e1 + 8*a*(-5011949722.5e1 + 2*a*(377243527.5e1 + a*(-39025192.5e1 + 2*a*(1445377.5e1 + 4*a*(-192717 + a*(7182 + a*(-171 + 2*a)))))))))*std::pow(M_E,a)*std::sqrt(M_PI)*std::tr1::erf(std::sqrt(a))))/(6.7108864e7*std::pow(a,9.5)*std::pow(M_E,a)) ; 
      break;
    case 20 : 
      result = (41.0*(-30.0*std::sqrt(a)*(150420217177132402.5e1 + 2*a*(11570785936702492.5e1 + 8*a*(465611205861301.5e1 + 2*a*(16877165244439.5e1 + a*(2196137366923.5e1 + 2*a*(44317398625.5e1 + 4*a*(718677745.5e1 + a*(15248765.4e1 + a*(430226.5e1 + 2879.4e1*a))))))))) + 46189*(48849363650587.5e1 + 4*a*(-6262738929562.5e1 + a*(1523368928812.5e1 + 8*a*(-29016551025.0e1 + a*(3077512987.5e1 + 4*a*(-59564767.5e1 + a*(3423262.5e1 + 2*a*(-724500 + a*(21735 + 4*(-105 + a)*a)))))))))*std::pow(M_E,a)*std::sqrt(M_PI)*std::tr1::erf(std::sqrt(a))))/(5.36870912e8*std::pow(a,10.5)*std::pow(M_E,a)) ; 
      break;
    case 22 : 
      result = 45*(-46* std::sqrt(a)*(15722777246044531162.5e1 + 4*a*(609409970776919812.5e1 + a*(197983922213379802.5e1 + 8*a*(1842440222557935.0e1 + a*(247349944474132.5e1 + 4*a*(2659408317247.5e1 + a*(187282275862.5e1 + 2.0*a*(2316815046.0e1 + a*(83165764.5e1 + 4*a*(238528.5e1 + 2662.9e1*a)))))))))) -  8817.9e1*(-4101020386475512.5e1 + 2*a*(1049098238400712.5e1 + 2*a*(-127938809561062.5e1 +    2*a*(9841446889312.5e1 + 4*a*(-265985051062.5e1 + 2*a*(10639402042.5e1 + 2*a*(-322406122.5e1 + 2*a*(7428712.5e1 + a*(-256162.5e1 + 2*a*(3162.5e1 - 50.6e1*a + 4.0* std::pow(a,2)))))))))))*   std::pow(M_E,a)* std::sqrt(M_PI)* std::tr1::erf( std::sqrt(a)))  ; 
      result /= (2.147483648e9* std::pow(a,11.5)* std::pow(M_E,a));
      break;
    case 24 :
      result =(49.0*(-30.0* std::sqrt(a)*(16996322202974138186662.5e1 + 2*a*(1325954214416422128037.5e1 + 
             2*a*(216974325995414530042.5e1 + 2*a*(8249669832974417347.5e1 + 4*a*
                    (283606141073075320.5e1 + 2*a*(6398463995484151.5e1 + 2*a*(238903097649898.5e1 + 2*a*(3282810567991.5e1 + a*(136370408128.5e1 + 2*a*(1026865776.3e1 + 2*a*(9717230.9e1 + 46910.2e1*a)))))))
                   )))) + 67603.9e1*(377115570321552562.5e1 + 8.0*a*(-24071206616269312.5e1 + 
             a*(5884072728421387.5e1 + 2*a*(-456129668869875.0e1 + a*(50063012436937.5e1 + 
                      16.0*a*(-256733397112.5e1 + a*(16190394412.5e1 + a*(-792998910.0e1 + a*(30037837.5e1 + 8.0*a*(-107662.5e1 + a*(2227.5e1 + 2*(-15.0e1 + a)*a)))))))))))*std::pow(M_E,a)*std::sqrt(M_PI)*std::tr1::erf(std::sqrt(a))))/
   (3.4359738368e10*std::pow(a,12.5)*std::pow(M_E,a)) ;
      break;
    default :
      utlException(true, "l is too big. l=" << l);
       break;
    }
  return result;
}

double 
GetExpLegendreCoefDerivative ( const double a, const int l )
{
  // coefficient for odd order is 0;
  if (!utl::IsInt(0.5*l))
    return 0;

  double result = -1000.0;
  switch ( l )
    {
    case 0 :
       result = 1/(2.*a* std::pow(M_E,a)) - ( std::sqrt(M_PI)* std::tr1::erf( std::sqrt(a)))/(4.* std::pow(a,1.5)) ; 
       break;
    case 2 :
       result = (5*(3/(2.* std::pow(a,2)* std::pow(M_E,a)) + (3 - 2*a)/(4.* std::pow(a,2)* std::pow(M_E,a)) + 3/(2.*a* std::pow(M_E,a)) - (3*(3 - 2*a)* std::sqrt(M_PI)* std::tr1::erf( std::sqrt(a)))/(8.* std::pow(a,2.5)) - ( std::sqrt(M_PI)* std::tr1::erf( std::sqrt(a)))/(2.* std::pow(a,1.5))))/2. ; 
       break;
    case 4 :
       result = (9*(-5/(8.* std::pow(a,2)* std::pow(M_E,a)) + (5*(21 + 2*a))/(8.* std::pow(a,3)* std::pow(M_E,a)) + (5*(21 + 2*a))/(16.* std::pow(a,2)* std::pow(M_E,a)) + (3*(35 + 4*(-5 + a)*a))/(32.* std::pow(a,3)* std::pow(M_E,a)) + (3*(4*(-5 + a) + 4*a)* std::sqrt(M_PI)* std::tr1::erf( std::sqrt(a)))/(32.* std::pow(a,2.5)) - (15*(35 + 4*(-5 + a)*a)* std::sqrt(M_PI)* std::tr1::erf( std::sqrt(a)))/(64.* std::pow(a,3.5))))/2. ; 
       break;
    case 6 : 
       result = (-91*(-42* std::sqrt(a)*(165 + 20*a + 4* std::pow(a,2)) - 5*(-693 + 378*a - 84* std::pow(a,2) + 8* std::pow(a,3))* std::pow(M_E,a)* std::sqrt(M_PI)* std::tr1::erf( std::sqrt(a))))/(512.* std::pow(a,4.5)* std::pow(M_E,a)) - (13*(-42* std::sqrt(a)*(165 + 20*a + 4* std::pow(a,2)) - 5*(-693 + 378*a - 84* std::pow(a,2) + 8* std::pow(a,3))* std::pow(M_E,a)* std::sqrt(M_PI)* std::tr1::erf( std::sqrt(a))))/(256.* std::pow(a,3.5)* std::pow(M_E,a)) + (13*(-42* std::sqrt(a)*(20 + 8*a) - (21*(165 + 20*a + 4* std::pow(a,2)))/ std::sqrt(a) - (5*(-693 + 378*a - 84* std::pow(a,2) + 8* std::pow(a,3)))/ std::sqrt(a) - 5*(378 - 168*a + 24* std::pow(a,2))* std::pow(M_E,a)* std::sqrt(M_PI)* std::tr1::erf( std::sqrt(a)) - 5*(-693 + 378*a - 84* std::pow(a,2) + 8* std::pow(a,3))* std::pow(M_E,a)* std::sqrt(M_PI)* std::tr1::erf( std::sqrt(a))))/(256.* std::pow(a,3.5)* std::pow(M_E,a)) ;
       break;
    case 8 :
       result = (-153*(-6* std::sqrt(a)*(225225 + 2*a*(15015 + 2*a*(1925 + 62*a))) + 35*(19305 + 8*a*(-1287 + a*(297 + 2*(-18 + a)*a)))* std::pow(M_E,a)* std::sqrt(M_PI)* std::tr1::erf( std::sqrt(a))))/(8192.* std::pow(a,5.5)* std::pow(M_E,a)) - (17*(-6* std::sqrt(a)*(225225 + 2*a*(15015 + 2*a*(1925 + 62*a))) + 35*(19305 + 8*a*(-1287 + a*(297 + 2*(-18 + a)*a)))* std::pow(M_E,a)* std::sqrt(M_PI)* std::tr1::erf( std::sqrt(a))))/(4096.* std::pow(a,4.5)* std::pow(M_E,a)) + (17*(-6* std::sqrt(a)*(2*a*(124*a + 2*(1925 + 62*a)) + 2*(15015 + 2*a*(1925 + 62*a))) - (3*(225225 + 2*a*(15015 + 2*a*(1925 + 62*a))))/ std::sqrt(a) + (35*(19305 + 8*a*(-1287 + a*(297 + 2*(-18 + a)*a))))/ std::sqrt(a) + 35*(8*a*(297 + 2*(-18 + a)*a + a*(2*(-18 + a) + 2*a)) + 8*(-1287 + a*(297 + 2*(-18 + a)*a)))* std::pow(M_E,a)* std::sqrt(M_PI)* std::tr1::erf( std::sqrt(a)) + 35*(19305 + 8*a*(-1287 + a*(297 + 2*(-18 + a)*a)))* std::pow(M_E,a)* std::sqrt(M_PI)* std::tr1::erf( std::sqrt(a))))/(4096.* std::pow(a,4.5)* std::pow(M_E,a)) ;
       break;
    case 10 : 
       result = (-231*(-22* std::sqrt(a)*(3968055 + 556920*a + 157248* std::pow(a,2) + 7488* std::pow(a,3) + 464* std::pow(a,4)) - 63*(-692835 + 364650*a - 85800* std::pow(a,2) + 11440* std::pow(a,3) - 880* std::pow(a,4) + 32* std::pow(a,5))* std::pow(M_E,a)* std::sqrt(M_PI)* std::tr1::erf( std::sqrt(a))))/(32768.* std::pow(a,6.5)* std::pow(M_E,a)) - (21*(-22* std::sqrt(a)*(3968055 + 556920*a + 157248* std::pow(a,2) + 7488* std::pow(a,3) + 464* std::pow(a,4)) - 63*(-692835 + 364650*a - 85800* std::pow(a,2) + 11440* std::pow(a,3) - 880* std::pow(a,4) + 32* std::pow(a,5))* std::pow(M_E,a)* std::sqrt(M_PI)* std::tr1::erf( std::sqrt(a))))/(16384.* std::pow(a,5.5)* std::pow(M_E,a)) + (21*(-22* std::sqrt(a)*(556920 + 314496*a + 22464* std::pow(a,2) + 1856* std::pow(a,3)) - (11*(3968055 + 556920*a + 157248* std::pow(a,2) + 7488* std::pow(a,3) + 464* std::pow(a,4)))/ std::sqrt(a) - (63*(-692835 + 364650*a - 85800* std::pow(a,2) + 11440* std::pow(a,3) - 880* std::pow(a,4) + 32* std::pow(a,5)))/ std::sqrt(a) - 63*(364650 - 171600*a + 34320* std::pow(a,2) - 3520* std::pow(a,3) + 160* std::pow(a,4))* std::pow(M_E,a)* std::sqrt(M_PI)* std::tr1::erf( std::sqrt(a)) - 63*(-692835 + 364650*a - 85800* std::pow(a,2) + 11440* std::pow(a,3) - 880* std::pow(a,4) + 32* std::pow(a,5))* std::pow(M_E,a)* std::sqrt(M_PI)* std::tr1::erf( std::sqrt(a))))/(16384.* std::pow(a,5.5)* std::pow(M_E,a)) ;
      break;
    case 12 : 
      result = (-325*(-26* std::sqrt(a)*(540571185 + 2*a*(39171825 + 4*a*(2909907 + 2*a*(82467 + a*(7469 + 122*a))))) + 231*(30421755 + 4*a*(-3968055 + a*(944775 + 4*a*(-33150 + a*(2925 + 4*(-39 + a)*a)))))* std::pow(M_E,a)* std::sqrt(M_PI)* std::tr1::erf( std::sqrt(a))))/(262144.* std::pow(a,7.5)* std::pow(M_E,a)) - (25*(-26* std::sqrt(a)*(540571185 + 2*a*(39171825 + 4*a*(2909907 + 2*a*(82467 + a*(7469 + 122*a))))) + 231*(30421755 + 4*a*(-3968055 + a*(944775 + 4*a*(-33150 + a*(2925 + 4*(-39 + a)*a)))))* std::pow(M_E,a)* std::sqrt(M_PI)* std::tr1::erf( std::sqrt(a))))/(131072.* std::pow(a,6.5)* std::pow(M_E,a)) + (25*(-26* std::sqrt(a)*(2*a*(4*a*(2*a*(7469 + 244*a) + 2*(82467 + a*(7469 + 122*a))) + 4*(2909907 + 2*a*(82467 + a*(7469 + 122*a)))) + 2*(39171825 + 4*a*(2909907 + 2*a*(82467 + a*(7469 + 122*a))))) - (13*(540571185 + 2*a*(39171825 + 4*a*(2909907 + 2*a*(82467 + a*(7469 + 122*a))))))/ std::sqrt(a) + (231*(30421755 + 4*a*(-3968055 + a*(944775 + 4*a*(-33150 + a*(2925 + 4*(-39 + a)*a))))))/ std::sqrt(a) + 231*(4*a*(944775 + 4*a*(-33150 + a*(2925 + 4*(-39 + a)*a)) + a*(4*a*(2925 + 4*(-39 + a)*a + a*(4*(-39 + a) + 4*a)) + 4*(-33150 + a*(2925 + 4*(-39 + a)*a)))) + 4*(-3968055 + a*(944775 + 4*a*(-33150 + a*(2925 + 4*(-39 + a)*a)))))* std::pow(M_E,a)* std::sqrt(M_PI)* std::tr1::erf( std::sqrt(a)) + 231*(30421755 + 4*a*(-3968055 + a*(944775 + 4*a*(-33150 + a*(2925 + 4*(-39 + a)*a)))))* std::pow(M_E,a)* std::sqrt(M_PI)* std::tr1::erf( std::sqrt(a))))/(131072.* std::pow(a,6.5)* std::pow(M_E,a)) ;
      break;
    case 14 : 
      result = (-435*(-2* std::sqrt(a)*(677644592625 + 100391791500*a + 30786816060* std::pow(a,2) + 1928852640* std::pow(a,3) + 206187696* std::pow(a,4) + 5360576* std::pow(a,5) + 158528* std::pow(a,6)) - 429*(-1579591125 + 819047250*a - 196571340* std::pow(a,2) + 28488600* std::pow(a,3) - 2713200* std::pow(a,4) + 171360* std::pow(a,5) - 6720* std::pow(a,6) + 128* std::pow(a,7))* std::pow(M_E,a)* std::sqrt(M_PI)* std::tr1::erf( std::sqrt(a))))/(1.048576e6* std::pow(a,8.5)* std::pow(M_E,a)) - (29*(-2* std::sqrt(a)*(677644592625 + 100391791500*a + 30786816060* std::pow(a,2) + 1928852640* std::pow(a,3) + 206187696* std::pow(a,4) + 5360576* std::pow(a,5) + 158528* std::pow(a,6)) - 429*(-1579591125 + 819047250*a - 196571340* std::pow(a,2) + 28488600* std::pow(a,3) - 2713200* std::pow(a,4) + 171360* std::pow(a,5) - 6720* std::pow(a,6) + 128* std::pow(a,7))* std::pow(M_E,a)* std::sqrt(M_PI)* std::tr1::erf( std::sqrt(a))))/(524288.* std::pow(a,7.5)* std::pow(M_E,a)) + (29*(-2* std::sqrt(a)*(100391791500 + 61573632120*a + 5786557920* std::pow(a,2) + 824750784* std::pow(a,3) + 26802880* std::pow(a,4) + 951168* std::pow(a,5)) - (677644592625 + 100391791500*a + 30786816060* std::pow(a,2) + 1928852640* std::pow(a,3) + 206187696* std::pow(a,4) + 5360576* std::pow(a,5) + 158528* std::pow(a,6))/ std::sqrt(a) - (429*(-1579591125 + 819047250*a - 196571340* std::pow(a,2) + 28488600* std::pow(a,3) - 2713200* std::pow(a,4) + 171360* std::pow(a,5) - 6720* std::pow(a,6) + 128* std::pow(a,7)))/ std::sqrt(a) - 429*(819047250 - 393142680*a + 85465800* std::pow(a,2) - 10852800* std::pow(a,3) + 856800* std::pow(a,4) - 40320* std::pow(a,5) + 896* std::pow(a,6))* std::pow(M_E,a)* std::sqrt(M_PI)* std::tr1::erf( std::sqrt(a)) - 429*(-1579591125 + 819047250*a - 196571340* std::pow(a,2) + 28488600* std::pow(a,3) - 2713200* std::pow(a,4) + 171360* std::pow(a,5) - 6720* std::pow(a,6) + 128* std::pow(a,7))* std::pow(M_E,a)* std::sqrt(M_PI)* std::tr1::erf( std::sqrt(a))))/(524288.* std::pow(a,7.5)* std::pow(M_E,a)) ;
      break;
    case 16 :
      result = (-0.00001671910285949707*(-34.* std::sqrt(a)*(3.5835440515875e13 + 2.*a*(2.697291221625e12 + 2.*a*(4.22531334225e11 + 2.*a*(1.4130777375e10 + 2.*a*(8.29568025e8 + 2.*a*(1.3438035e7 + 624150.*a + 6196.* std::pow(a,2))))))) + 11405.740530576995* std::pow(2.718281828459045,a)*(9.4670161425e10 + 16.*a*(-3.053876175e9 + a*(7.37142525e8 + 2.*a*(-5.460315e7 + a*(5.460315e6 + 8.*a*(-47481. + a*(2261. + (-68. + a)*a)))))))* std::tr1::erf( std::sqrt(a))))/( std::pow(2.718281828459045,1.*a)* std::pow(a,9.5)) - (1.9669532775878906e-6*(-34.* std::sqrt(a)*(3.5835440515875e13 + 2.*a*(2.697291221625e12 + 2.*a*(4.22531334225e11 + 2.*a*(1.4130777375e10 + 2.*a*(8.29568025e8 + 2.*a*(1.3438035e7 + 624150.*a + 6196.* std::pow(a,2))))))) + 11405.740530576995* std::pow(2.718281828459045,a)*(9.4670161425e10 + 16.*a*(-3.053876175e9 + a*(7.37142525e8 + 2.*a*(-5.460315e7 + a*(5.460315e6 + 8.*a*(-47481. + a*(2261. + (-68. + a)*a)))))))* std::tr1::erf( std::sqrt(a))))/( std::pow(2.718281828459045,1.*a)* std::pow(a,8.5)) + (1.9669532775878906e-6*(-34.* std::sqrt(a)*(2.*a*(2.*a*(2.*a*(2.*a*(2.*a*(624150. + 12392.*a) + 2.*(1.3438035e7 + 624150.*a + 6196.* std::pow(a,2))) + 2.*(8.29568025e8 + 2.*a*(1.3438035e7 + 624150.*a + 6196.* std::pow(a,2)))) + 2.*(1.4130777375e10 + 2.*a*(8.29568025e8 + 2.*a*(1.3438035e7 + 624150.*a + 6196.* std::pow(a,2))))) + 2.*(4.22531334225e11 + 2.*a*(1.4130777375e10 + 2.*a*(8.29568025e8 + 2.*a*(1.3438035e7 + 624150.*a + 6196.* std::pow(a,2)))))) + 2.*(2.697291221625e12 + 2.*a*(4.22531334225e11 + 2.*a*(1.4130777375e10 + 2.*a*(8.29568025e8 + 2.*a*(1.3438035e7 + 624150.*a + 6196.* std::pow(a,2))))))) - (17.*(3.5835440515875e13 + 2.*a*(2.697291221625e12 + 2.*a*(4.22531334225e11 + 2.*a*(1.4130777375e10 + 2.*a*(8.29568025e8 + 2.*a*(1.3438035e7 + 624150.*a + 6196.* std::pow(a,2))))))))/ std::sqrt(a) + (6435.*(9.4670161425e10 + 16.*a*(-3.053876175e9 + a*(7.37142525e8 + 2.*a*(-5.460315e7 + a*(5.460315e6 + 8.*a*(-47481. + a*(2261. + (-68. + a)*a))))))))/ std::sqrt(a) + 11405.740530576995* std::pow(2.718281828459045,a)*(16.*a*(7.37142525e8 + 2.*a*(-5.460315e7 + a*(5.460315e6 + 8.*a*(-47481. + a*(2261. + (-68. + a)*a)))) + a*(2.*a*(5.460315e6 + 8.*a*(-47481. + a*(2261. + (-68. + a)*a)) + a*(8.*a*(2261. + (-68. + a)*a + a*(-68. + 2.*a)) + 8.*(-47481. + a*(2261. + (-68. + a)*a)))) + 2.*(-5.460315e7 + a*(5.460315e6 + 8.*a*(-47481. + a*(2261. + (-68. + a)*a)))))) + 16.*(-3.053876175e9 + a*(7.37142525e8 + 2.*a*(-5.460315e7 + a*(5.460315e6 + 8.*a*(-47481. + a*(2261. + (-68. + a)*a)))))))* std::tr1::erf( std::sqrt(a)) + 11405.740530576995* std::pow(2.718281828459045,a)*(9.4670161425e10 + 16.*a*(-3.053876175e9 + a*(7.37142525e8 + 2.*a*(-5.460315e7 + a*(5.460315e6 + 8.*a*(-47481. + a*(2261. + (-68. + a)*a)))))))* std::tr1::erf( std::sqrt(a))))/( std::pow(2.718281828459045,1.*a)* std::pow(a,8.5)); 
      // result = (-561.*(-34.* std::sqrt(a)*(3583544051587.5e1 + 2*a*(269729122162.5e1 + 2*a*(42253133422.5e1 + 2*a*(1413077737.5e1 + 2*a*(82956802.5e1 + 2*a*(1343803.5e1 + 62415.0e1*a + 619.6e1* std::pow(a,2.))))))) + 643.5e1*(9467016142.5e1 + 1.6e1*a*(-305387617.5e1 + a*(73714252.5e1 + 2*a*(-5460315.0e1 + a*(546031.5e1 + 8*a*(-4748.1e1 + a*(226.1e1 + (-68 + a)*a)))))))* std::pow(M_E,a)* std::sqrt(M_PI)* std::tr1::erf( std::sqrt(a))))/(3.3554432e7* std::pow(a,9.5)* std::pow(M_E,a)) - (33*(-34* std::sqrt(a)*(3583544051587.5e1 + 2*a*(269729122162.5e1 + 2*a*(42253133422.5e1 + 2*a*(1413077737.5e1 + 2*a*(82956802.5e1 + 2*a*(1343803.5e1 + 62415.0e1*a + 619.6e1* std::pow(a,2))))))) + 643.5e1*(9467016142.5e1 + 16*a*(-305387617.5e1 + a*(73714252.5e1 + 2*a*(-5460315.0e1 + a*(546031.5e1 + 8*a*(-4748.1e1 + a*(226.1e1 + (-6.8e1 + a)*a)))))))* std::pow(M_E,a)* std::sqrt(M_PI)* std::tr1::erf( std::sqrt(a))))/(1.6777216e7* std::pow(a,8.5)* std::pow(M_E,a)) + (33*(-34* std::sqrt(a)*(2*a*(2*a*(2*a*(2*a*(2*a*(62415.0e1 + 1239.2e1*a) + 2*(1343803.5e1 + 62415.0e1*a + 619.6e1* std::pow(a,2))) + 2*(82956802.5e1 + 2*a*(1343803.5e1 + 62415.0e1*a + 6196.* std::pow(a,2)))) + 2*(14130777375. + 2*a*(829568025. + 2*a*(13438035. + 624150.*a + 6196.* std::pow(a,2))))) + 2*(422531334225. + 2*a*(14130777375. + 2*a*(829568025. + 2*a*(13438035. + 624150.*a + 6196.* std::pow(a,2)))))) + 2*(2697291221625. + 2*a*(422531334225. + 2*a*(14130777375. + 2*a*(829568025. + 2*a*(13438035. + 624150.*a + 6196.* std::pow(a,2))))))) - (17*(35835440515875. + 2*a*(2697291221625. + 2*a*(422531334225. + 2*a*(14130777375. + 2*a*(829568025. + 2*a*(13438035. + 624150.*a + 6196.* std::pow(a,2))))))))/ std::sqrt(a) + (6435.*(94670161425. + 16.*a*(-3053876175. + a*(737142525. + 2*a*(-54603150. + a*(5460315. + 8*a*(-47481. + a*(2261. + (-68. + a)*a))))))))/ std::sqrt(a) + 6435.*(16*a*(737142525. + 2*a*(-54603150. + a*(5460315. + 8*a*(-47481. + a*(2261. + (-68. + a)*a)))) + a*(2*a*(5460315. + 8*a*(-47481. + a*(2261. + (-68 + a)*a)) + a*(8*a*(2261. + (-68. + a)*a + a*(-68 + 2*a)) + 8*(-47481. + a*(2261. + (-68 + a)*a)))) + 2*(-54603150. + a*(5460315. + 8*a*(-47481. + a*(2261. + (-68. + a)*a)))))) + 16*(-3053876175. + a*(737142525. + 2*a*(-54603150 + a*(5460315. + 8*a*(-47481. + a*(2261. + (-68 + a)*a)))))))* std::pow(M_E,a)* std::sqrt(M_PI)* std::tr1::erf( std::sqrt(a)) + 6435.*(94670161425. + 16.*a*(-3053876175 + a*(737142525. + 2.*a*(-54603150. + a*(5460315. + 8.*a*(-47481. + a*(2261. + (-68 + a)*a)))))))* std::pow(M_E,a)* std::sqrt(M_PI)* std::tr1::erf( std::sqrt(a))))/(1.6777216e7* std::pow(a,8.5)* std::pow(M_E,a)) ;
      break;
    case 18 : 
      result = (-5.237758159637451e-6*(-114.* std::sqrt(a)*(1.371596246645625e15 + 8.*a*(2.612564279325e13 + a*(8.312704525125e12 + 4.*a*(1.45568097675e11 + 2.*a*(9.078873375e9 + 2.*a*(1.67739e8 + a*(9.612735e6 + 2.*a*(79390. + 1363.*a)))))))) - 21544.176557756546* std::pow(2.718281828459045,a)*(-6.432002143875e12 + 2.*a*(1.653943408425e12 + 8.*a*(-5.0119497225e10 + 2.*a*(3.772435275e9 + a*(-3.90251925e8 + 2.*a*(1.4453775e7 + 4.*a*(-192717. + a*(7182. + a*(-171. + 2.*a)))))))))* std::tr1::erf( std::sqrt(a))))/( std::pow(2.718281828459045,1.*a)* std::pow(a,10.5)) - (5.513429641723633e-7*(-114.* std::sqrt(a)*(1.371596246645625e15 + 8.*a*(2.612564279325e13 + a*(8.312704525125e12 + 4.*a*(1.45568097675e11 + 2.*a*(9.078873375e9 + 2.*a*(1.67739e8 + a*(9.612735e6 + 2.*a*(79390. + 1363.*a)))))))) - 21544.176557756546* std::pow(2.718281828459045,a)*(-6.432002143875e12 + 2.*a*(1.653943408425e12 + 8.*a*(-5.0119497225e10 + 2.*a*(3.772435275e9 + a*(-3.90251925e8 + 2.*a*(1.4453775e7 + 4.*a*(-192717. + a*(7182. + a*(-171. + 2.*a)))))))))* std::tr1::erf( std::sqrt(a))))/( std::pow(2.718281828459045,1.*a)* std::pow(a,9.5)) + (5.513429641723633e-7*(-114.* std::sqrt(a)*(8.*a*(8.312704525125e12 + 4.*a*(1.45568097675e11 + 2.*a*(9.078873375e9 + 2.*a*(1.67739e8 + a*(9.612735e6 + 2.*a*(79390. + 1363.*a))))) + a*(4.*a*(2.*a*(2.*a*(9.612735e6 + 2.*a*(79390. + 1363.*a) + a*(2726.*a + 2.*(79390. + 1363.*a))) + 2.*(1.67739e8 + a*(9.612735e6 + 2.*a*(79390. + 1363.*a)))) + 2.*(9.078873375e9 + 2.*a*(1.67739e8 + a*(9.612735e6 + 2.*a*(79390. + 1363.*a))))) + 4.*(1.45568097675e11 + 2.*a*(9.078873375e9 + 2.*a*(1.67739e8 + a*(9.612735e6 + 2.*a*(79390. + 1363.*a))))))) + 8.*(2.612564279325e13 + a*(8.312704525125e12 + 4.*a*(1.45568097675e11 + 2.*a*(9.078873375e9 + 2.*a*(1.67739e8 + a*(9.612735e6 + 2.*a*(79390. + 1363.*a)))))))) - (57.*(1.371596246645625e15 + 8.*a*(2.612564279325e13 + a*(8.312704525125e12 + 4.*a*(1.45568097675e11 + 2.*a*(9.078873375e9 + 2.*a*(1.67739e8 + a*(9.612735e6 + 2.*a*(79390. + 1363.*a)))))))))/ std::sqrt(a) - (12155.*(-6.432002143875e12 + 2.*a*(1.653943408425e12 + 8.*a*(-5.0119497225e10 + 2.*a*(3.772435275e9 + a*(-3.90251925e8 + 2.*a*(1.4453775e7 + 4.*a*(-192717. + a*(7182. + a*(-171. + 2.*a))))))))))/ std::sqrt(a) - 21544.176557756546* std::pow(2.718281828459045,a)*(2.*a*(8.*a*(2.*a*(-3.90251925e8 + 2.*a*(1.4453775e7 + 4.*a*(-192717. + a*(7182. + a*(-171. + 2.*a)))) + a*(2.*a*(4.*a*(7182. + a*(-171. + 2.*a) + a*(-171. + 4.*a)) + 4.*(-192717. + a*(7182. + a*(-171. + 2.*a)))) + 2.*(1.4453775e7 + 4.*a*(-192717. + a*(7182. + a*(-171. + 2.*a)))))) + 2.*(3.772435275e9 + a*(-3.90251925e8 + 2.*a*(1.4453775e7 + 4.*a*(-192717. + a*(7182. + a*(-171. + 2.*a))))))) + 8.*(-5.0119497225e10 + 2.*a*(3.772435275e9 + a*(-3.90251925e8 + 2.*a*(1.4453775e7 + 4.*a*(-192717. + a*(7182. + a*(-171. + 2.*a)))))))) + 2.*(1.653943408425e12 + 8.*a*(-5.0119497225e10 + 2.*a*(3.772435275e9 + a*(-3.90251925e8 + 2.*a*(1.4453775e7 + 4.*a*(-192717. + a*(7182. + a*(-171. + 2.*a)))))))))* std::tr1::erf( std::sqrt(a)) - 21544.176557756546* std::pow(2.718281828459045,a)*(-6.432002143875e12 + 2.*a*(1.653943408425e12 + 8.*a*(-5.0119497225e10 + 2.*a*(3.772435275e9 + a*(-3.90251925e8 + 2.*a*(1.4453775e7 + 4.*a*(-192717. + a*(7182. + a*(-171. + 2.*a)))))))))* std::tr1::erf( std::sqrt(a))))/( std::pow(2.718281828459045,1.*a)* std::pow(a,9.5)); 
      // result = (-703*(-114* std::sqrt(a)*(1371596246645625 + 8*a*(26125642793250 + a*(8312704525125 + 4*a*(145568097675 + 2*a*(9078873375 + 2*a*(167739000 + a*(9612735 + 2*a*(79390 + 1363*a)))))))) - 12155*(-6432002143875 + 2*a*(1653943408425 + 8*a*(-50119497225 + 2*a*(3772435275 + a*(-390251925 + 2*a*(14453775 + 4*a*(-192717 + a*(7182 + a*(-171 + 2*a)))))))))* std::pow(M_E,a)* std::sqrt(M_PI)* std::tr1::erf( std::sqrt(a))))/(1.34217728e8* std::pow(a,10.5)* std::pow(M_E,a)) - (37*(-114* std::sqrt(a)*(1371596246645625 + 8*a*(26125642793250 + a*(8312704525125 + 4*a*(145568097675 + 2*a*(9078873375 + 2*a*(167739000 + a*(9612735 + 2*a*(79390 + 1363*a)))))))) - 12155*(-6432002143875 + 2*a*(1653943408425 + 8*a*(-50119497225 + 2*a*(3772435275 + a*(-390251925 + 2*a*(14453775 + 4*a*(-192717 + a*(7182 + a*(-171 + 2*a)))))))))* std::pow(M_E,a)* std::sqrt(M_PI)* std::tr1::erf( std::sqrt(a))))/(6.7108864e7* std::pow(a,9.5)* std::pow(M_E,a)) + (37*(-114* std::sqrt(a)*(8*a*(8312704525125 + 4*a*(145568097675 + 2*a*(9078873375 + 2*a*(167739000 + a*(9612735 + 2*a*(79390 + 1363*a))))) + a*(4*a*(2*a*(2*a*(9612735 + 2*a*(79390 + 1363*a) + a*(2726*a + 2*(79390 + 1363*a))) + 2*(167739000 + a*(9612735 + 2*a*(79390 + 1363*a)))) + 2*(9078873375 + 2*a*(167739000 + a*(9612735 + 2*a*(79390 + 1363*a))))) + 4*(145568097675 + 2*a*(9078873375 + 2*a*(167739000 + a*(9612735 + 2*a*(79390 + 1363*a))))))) + 8*(26125642793250 + a*(8312704525125 + 4*a*(145568097675 + 2*a*(9078873375 + 2*a*(167739000 + a*(9612735 + 2*a*(79390 + 1363*a)))))))) - (57*(1371596246645625 + 8*a*(26125642793250 + a*(8312704525125 + 4*a*(145568097675 + 2*a*(9078873375 + 2*a*(167739000 + a*(9612735 + 2*a*(79390 + 1363*a)))))))))/ std::sqrt(a) - (12155*(-6432002143875 + 2*a*(1653943408425 + 8*a*(-50119497225 + 2*a*(3772435275 + a*(-390251925 + 2*a*(14453775 + 4*a*(-192717 + a*(7182 + a*(-171 + 2*a))))))))))/ std::sqrt(a) - 12155*(2*a*(8*a*(2*a*(-390251925 + 2*a*(14453775 + 4*a*(-192717 + a*(7182 + a*(-171 + 2*a)))) + a*(2*a*(4*a*(7182 + a*(-171 + 2*a) + a*(-171 + 4*a)) + 4*(-192717 + a*(7182 + a*(-171 + 2*a)))) + 2*(14453775 + 4*a*(-192717 + a*(7182 + a*(-171 + 2*a)))))) + 2*(3772435275 + a*(-390251925 + 2*a*(14453775 + 4*a*(-192717 + a*(7182 + a*(-171 + 2*a))))))) + 8*(-50119497225 + 2*a*(3772435275 + a*(-390251925 + 2*a*(14453775 + 4*a*(-192717 + a*(7182 + a*(-171 + 2*a)))))))) + 2*(1653943408425 + 8*a*(-50119497225 + 2*a*(3772435275 + a*(-390251925 + 2*a*(14453775 + 4*a*(-192717 + a*(7182 + a*(-171 + 2*a)))))))))* std::pow(M_E,a)* std::sqrt(M_PI)* std::tr1::erf( std::sqrt(a)) - 12155*(-6432002143875 + 2*a*(1653943408425 + 8*a*(-50119497225 + 2*a*(3772435275 + a*(-390251925 + 2*a*(14453775 + 4*a*(-192717 + a*(7182 + a*(-171 + 2*a)))))))))* std::pow(M_E,a)* std::sqrt(M_PI)* std::tr1::erf( std::sqrt(a))))/(6.7108864e7* std::pow(a,9.5)* std::pow(M_E,a)) ; 
      break;
    case 20 : 
      result = (-8.01868736743927e-7*(-30.* std::sqrt(a)*(1.504202171771324e18 + 2.*a*(1.1570785936702493e17 + 8.*a*(4.656112058613015e15 + 2.*a*(1.68771652444395e14 + a*(2.1961373669235e13 + 2.*a*(4.43173986255e11 + 4.*a*(7.186777455e9 + a*(1.52487654e8 + a*(4.302265e6 + 28794.*a))))))))) + 81867.87091947487* std::pow(2.718281828459045,a)*(4.88493636505875e14 + 4.*a*(-6.2627389295625e13 + a*(1.5233689288125e13 + 8.*a*(-2.9016551025e11 + a*(3.0775129875e10 + 4.*a*(-5.95647675e8 + a*(3.4232625e7 + 2.*a*(-724500. + a*(21735. + 4.*(-105. + a)*a)))))))))* std::tr1::erf( std::sqrt(a))))/( std::pow(2.718281828459045,1.*a)* std::pow(a,11.5)) - (7.636845111846924e-8*(-30.* std::sqrt(a)*(1.504202171771324e18 + 2.*a*(1.1570785936702493e17 + 8.*a*(4.656112058613015e15 + 2.*a*(1.68771652444395e14 + a*(2.1961373669235e13 + 2.*a*(4.43173986255e11 + 4.*a*(7.186777455e9 + a*(1.52487654e8 + a*(4.302265e6 + 28794.*a))))))))) + 81867.87091947487* std::pow(2.718281828459045,a)*(4.88493636505875e14 + 4.*a*(-6.2627389295625e13 + a*(1.5233689288125e13 + 8.*a*(-2.9016551025e11 + a*(3.0775129875e10 + 4.*a*(-5.95647675e8 + a*(3.4232625e7 + 2.*a*(-724500. + a*(21735. + 4.*(-105. + a)*a)))))))))* std::tr1::erf( std::sqrt(a))))/( std::pow(2.718281828459045,1.*a)* std::pow(a,10.5)) + (7.636845111846924e-8*(-30.* std::sqrt(a)*(2.*a*(8.*a*(2.*a*(2.1961373669235e13 + 2.*a*(4.43173986255e11 + 4.*a*(7.186777455e9 + a*(1.52487654e8 + a*(4.302265e6 + 28794.*a)))) + a*(2.*a*(4.*a*(1.52487654e8 + a*(4.302265e6 + 28794.*a) + a*(4.302265e6 + 57588.*a)) + 4.*(7.186777455e9 + a*(1.52487654e8 + a*(4.302265e6 + 28794.*a)))) + 2.*(4.43173986255e11 + 4.*a*(7.186777455e9 + a*(1.52487654e8 + a*(4.302265e6 + 28794.*a)))))) + 2.*(1.68771652444395e14 + a*(2.1961373669235e13 + 2.*a*(4.43173986255e11 + 4.*a*(7.186777455e9 + a*(1.52487654e8 + a*(4.302265e6 + 28794.*a))))))) + 8.*(4.656112058613015e15 + 2.*a*(1.68771652444395e14 + a*(2.1961373669235e13 + 2.*a*(4.43173986255e11 + 4.*a*(7.186777455e9 + a*(1.52487654e8 + a*(4.302265e6 + 28794.*a)))))))) + 2.*(1.1570785936702493e17 + 8.*a*(4.656112058613015e15 + 2.*a*(1.68771652444395e14 + a*(2.1961373669235e13 + 2.*a*(4.43173986255e11 + 4.*a*(7.186777455e9 + a*(1.52487654e8 + a*(4.302265e6 + 28794.*a))))))))) - (15.*(1.504202171771324e18 + 2.*a*(1.1570785936702493e17 + 8.*a*(4.656112058613015e15 + 2.*a*(1.68771652444395e14 + a*(2.1961373669235e13 + 2.*a*(4.43173986255e11 + 4.*a*(7.186777455e9 + a*(1.52487654e8 + a*(4.302265e6 + 28794.*a))))))))))/ std::sqrt(a) + (46189.*(4.88493636505875e14 + 4.*a*(-6.2627389295625e13 + a*(1.5233689288125e13 + 8.*a*(-2.9016551025e11 + a*(3.0775129875e10 + 4.*a*(-5.95647675e8 + a*(3.4232625e7 + 2.*a*(-724500. + a*(21735. + 4.*(-105. + a)*a))))))))))/ std::sqrt(a) + 81867.87091947487* std::pow(2.718281828459045,a)*(4.*a*(1.5233689288125e13 + 8.*a*(-2.9016551025e11 + a*(3.0775129875e10 + 4.*a*(-5.95647675e8 + a*(3.4232625e7 + 2.*a*(-724500. + a*(21735. + 4.*(-105. + a)*a)))))) + a*(8.*a*(3.0775129875e10 + 4.*a*(-5.95647675e8 + a*(3.4232625e7 + 2.*a*(-724500. + a*(21735. + 4.*(-105. + a)*a)))) + a*(4.*a*(3.4232625e7 + 2.*a*(-724500. + a*(21735. + 4.*(-105. + a)*a)) + a*(2.*a*(21735. + 4.*(-105. + a)*a + a*(4.*(-105. + a) + 4.*a)) + 2.*(-724500. + a*(21735. + 4.*(-105. + a)*a)))) + 4.*(-5.95647675e8 + a*(3.4232625e7 + 2.*a*(-724500. + a*(21735. + 4.*(-105. + a)*a)))))) + 8.*(-2.9016551025e11 + a*(3.0775129875e10 + 4.*a*(-5.95647675e8 + a*(3.4232625e7 + 2.*a*(-724500. + a*(21735. + 4.*(-105. + a)*a)))))))) + 4.*(-6.2627389295625e13 + a*(1.5233689288125e13 + 8.*a*(-2.9016551025e11 + a*(3.0775129875e10 + 4.*a*(-5.95647675e8 + a*(3.4232625e7 + 2.*a*(-724500. + a*(21735. + 4.*(-105. + a)*a)))))))))* std::tr1::erf( std::sqrt(a)) + 81867.87091947487* std::pow(2.718281828459045,a)*(4.88493636505875e14 + 4.*a*(-6.2627389295625e13 + a*(1.5233689288125e13 + 8.*a*(-2.9016551025e11 + a*(3.0775129875e10 + 4.*a*(-5.95647675e8 + a*(3.4232625e7 + 2.*a*(-724500. + a*(21735. + 4.*(-105. + a)*a)))))))))* std::tr1::erf( std::sqrt(a))))/( std::pow(2.718281828459045,1.*a)* std::pow(a,10.5)); 
      // result = (-861*(-30* std::sqrt(a)*(1504202171771324025 + 2*a*(115707859367024925 + 8*a*(4656112058613015 + 2*a*(168771652444395 + a*(21961373669235 + 2*a*(443173986255 + 4*a*(7186777455 + a*(152487654 + a*(4302265 + 28794*a))))))))) + 46189*(488493636505875 + 4*a*(-62627389295625 + a*(15233689288125 + 8*a*(-290165510250 + a*(30775129875 + 4*a*(-595647675 + a*(34232625 + 2*a*(-724500 + a*(21735 + 4*(-105 + a)*a)))))))))* std::pow(M_E,a)* std::sqrt(M_PI)* std::tr1::erf( std::sqrt(a))))/(1.073741824e9* std::pow(a,11.5)* std::pow(M_E,a)) - (41*(-30* std::sqrt(a)*(1504202171771324025 + 2*a*(115707859367024925 + 8*a*(4656112058613015 + 2*a*(168771652444395 + a*(21961373669235 + 2*a*(443173986255 + 4*a*(7186777455 + a*(152487654 + a*(4302265 + 28794*a))))))))) + 46189*(488493636505875 + 4*a*(-62627389295625 + a*(15233689288125 + 8*a*(-290165510250 + a*(30775129875 + 4*a*(-595647675 + a*(34232625 + 2*a*(-724500 + a*(21735 + 4*(-105 + a)*a)))))))))* std::pow(M_E,a)* std::sqrt(M_PI)* std::tr1::erf( std::sqrt(a))))/(5.36870912e8* std::pow(a,10.5)* std::pow(M_E,a)) + (41*(-30* std::sqrt(a)*(2*a*(8*a*(2*a*(21961373669235 + 2*a*(443173986255 + 4*a*(7186777455 + a*(152487654 + a*(4302265 + 28794*a)))) + a*(2*a*(4*a*(152487654 + a*(4302265 + 28794*a) + a*(4302265 + 57588*a)) + 4*(7186777455 + a*(152487654 + a*(4302265 + 28794*a)))) + 2*(443173986255 + 4*a*(7186777455 + a*(152487654 + a*(4302265 + 28794*a)))))) + 2*(168771652444395 + a*(21961373669235 + 2*a*(443173986255 + 4*a*(7186777455 + a*(152487654 + a*(4302265 + 28794*a))))))) + 8*(4656112058613015 + 2*a*(168771652444395 + a*(21961373669235 + 2*a*(443173986255 + 4*a*(7186777455 + a*(152487654 + a*(4302265 + 28794*a)))))))) + 2*(115707859367024925 + 8*a*(4656112058613015 + 2*a*(168771652444395 + a*(21961373669235 + 2*a*(443173986255 + 4*a*(7186777455 + a*(152487654 + a*(4302265 + 28794*a))))))))) - (15*(1504202171771324025 + 2*a*(115707859367024925 + 8*a*(4656112058613015 + 2*a*(168771652444395 + a*(21961373669235 + 2*a*(443173986255 + 4*a*(7186777455 + a*(152487654 + a*(4302265 + 28794*a))))))))))/ std::sqrt(a) + (46189*(488493636505875 + 4*a*(-62627389295625 + a*(15233689288125 + 8*a*(-290165510250 + a*(30775129875 + 4*a*(-595647675 + a*(34232625 + 2*a*(-724500 + a*(21735 + 4*(-105 + a)*a))))))))))/ std::sqrt(a) + 46189*(4*a*(15233689288125 + 8*a*(-290165510250 + a*(30775129875 + 4*a*(-595647675 + a*(34232625 + 2*a*(-724500 + a*(21735 + 4*(-105 + a)*a)))))) + a*(8*a*(30775129875 + 4*a*(-595647675 + a*(34232625 + 2*a*(-724500 + a*(21735 + 4*(-105 + a)*a)))) + a*(4*a*(34232625 + 2*a*(-724500 + a*(21735 + 4*(-105 + a)*a)) + a*(2*a*(21735 + 4*(-105 + a)*a + a*(4*(-105 + a) + 4*a)) + 2*(-724500 + a*(21735 + 4*(-105 + a)*a)))) + 4*(-595647675 + a*(34232625 + 2*a*(-724500 + a*(21735 + 4*(-105 + a)*a)))))) + 8*(-290165510250 + a*(30775129875 + 4*a*(-595647675 + a*(34232625 + 2*a*(-724500 + a*(21735 + 4*(-105 + a)*a)))))))) + 4*(-62627389295625 + a*(15233689288125 + 8*a*(-290165510250 + a*(30775129875 + 4*a*(-595647675 + a*(34232625 + 2*a*(-724500 + a*(21735 + 4*(-105 + a)*a)))))))))* std::pow(M_E,a)* std::sqrt(M_PI)* std::tr1::erf( std::sqrt(a)) + 46189*(488493636505875 + 4*a*(-62627389295625 + a*(15233689288125 + 8*a*(-290165510250 + a*(30775129875 + 4*a*(-595647675 + a*(34232625 + 2*a*(-724500 + a*(21735 + 4*(-105 + a)*a)))))))))* std::pow(M_E,a)* std::sqrt(M_PI)* std::tr1::erf( std::sqrt(a))))/(5.36870912e8* std::pow(a,10.5)* std::pow(M_E,a)) ; 
      break;
    default :
      utlException(true, "l is too big. l=" << l);
       break;
    }
  return result;
}


}


#endif 
