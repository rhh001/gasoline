
#ifndef COOLING_METAL_HINCLUDED
#define COOLING_METAL_HINCLUDED

/* Global consts */
#if defined(COOLDEBUG)
#include "mdl.h"
#endif
#include "floattype.h"
#include "param.h"
#include "rpc/xdr.h"

/* Constants */
#define CL_B_gm         (6.022e23*(938.7830/931.494))/*Avegadro's Number * Mass_Hydrogen/Energy_AMU */
#define CL_k_Boltzmann  1.38066e-16
#define CL_eV_erg       1.60219e-12
#define CL_eV_per_K     (CL_k_Boltzmann/CL_eV_erg)
/*
#define CL_RT_FLOAT      float
#define CL_RT_MIN        1e-38
#define CL_RT_MIN        FLT_MIN
*/

#define CL_RT_FLOAT      double
#define CL_RT_MIN        1e-100

/*
#define CL_RT_MIN        DBL_MIN
*/
/* 
 * Work around for Dec ev6 flawed
 * treatment of sub-normal numbers 
 */
#define CL_MAX_NEG_EXP_ARG  -500.

#define CL_NMAXBYTETABLE   56000
#define MU_METAL  17.6003
#define ZSOLAR    0.0130215

typedef struct CoolingParametersStruct {
  int    bIonNonEqm;
  int    nCoolingTable;
  int    bUV;
  int    bMetal;
  char   *CoolInFile; 
  int    bUVTableUsesTime;
  int    bDoIonOutput;
  int    bLowTCool;
  int    bSelfShield;
  int    bShieldHI; /* Set to true if dust shields HI;*/ 
  double dMassFracHelium;
  double dCoolingTmin;     
  double dCoolingTmax;
  double dClump;
#ifdef  RADIATIVEBOX       
  double dLymanWernerFrac; /* Fraction of Lyman Werner radiation that escapes birth cloud.  0.5 is a good value.*/
  int bAgeFromMass; /*  Set to true to determine age of star particle from mass compared to formation mass when calculating LW radiation.  Useful in running ICs which already have stars*/
#endif        
} COOLPARAM;

typedef struct CoolingParticleStruct {
  FLOAT f_HI,f_HeI,f_HeII;
  FLOAT f_H2;	/* Abundance of ions */
#ifdef  RADIATIVEBOX
  double     dLymanWerner; /* Flux of Lyman Werner radiation at the gas particle */
#endif
} COOLPARTICLE;

typedef struct { 
  double e,Total;
  double HI,HII,HeI,HeII,HeIII;
  double H2; 
} PERBARYON;

typedef struct { 
  double   zTime;

  double   Rate_Phot_HI;
  double   Rate_Phot_HeI;
  double   Rate_Phot_HeII;
  double   Rate_Phot_H2_cosmo; /* Dissociating radiation from the cosmic background for H2*/ 

  double   Heat_Phot_HI;
  double   Heat_Phot_HeI;
  double   Heat_Phot_HeII;
  double   Heat_Phot_H2; 
} UVSPECTRUM;

typedef struct { 
  double   Rate_Phot_HI;
  double   Rate_Phot_HeI;
  double   Rate_Phot_HeII;
  double   Rate_Phot_H2_cosmo;  

  double   Heat_Phot_HI;
  double   Heat_Phot_HeI;
  double   Heat_Phot_HeII;
  double   Heat_Phot_H2;  
 
  double   Cool_Coll_HI;
  double   Cool_Coll_HeI;
  double   Cool_Coll_HeII;
  double   Cool_Diel_HeII;
  double   Cool_Coll_H2;  
 
  double   Cool_Comp;
  double   Tcmb;
  double   Cool_LowTFactor;

} RATES_NO_T;

typedef struct { 
  CL_RT_FLOAT   Rate_Coll_HI;
  CL_RT_FLOAT   Rate_Coll_HeI;
  CL_RT_FLOAT   Rate_Coll_HeII;
  CL_RT_FLOAT   Rate_Coll_e_H2;  
  CL_RT_FLOAT   Rate_Coll_HI_H2;  
  CL_RT_FLOAT   Rate_Coll_H2_H2;  
  CL_RT_FLOAT   Rate_Coll_Hm_e;           /*gas phase formation of H2 */
  CL_RT_FLOAT   Rate_Coll_HI_e;           /*--------------------*/
  CL_RT_FLOAT   Rate_Coll_HII_H2;          /*--------------------*/
  CL_RT_FLOAT   Rate_Coll_Hm_HII;        /*-------------------- */
  CL_RT_FLOAT   Rate_HI_e;          /*-------------------- */
  CL_RT_FLOAT   Rate_HI_Hm;          /*gas phase formation of H2 */
  CL_RT_FLOAT   Rate_Radr_HII;
  CL_RT_FLOAT   Rate_Radr_HeII;
  CL_RT_FLOAT   Rate_Radr_HeIII;
  CL_RT_FLOAT   Rate_Diel_HeII;
  CL_RT_FLOAT   Rate_Chtr_HeII;

  CL_RT_FLOAT   Cool_Brem_1;
  CL_RT_FLOAT   Cool_Brem_2;
  CL_RT_FLOAT   Cool_Radr_HII;
  CL_RT_FLOAT   Cool_Radr_HeII;
  CL_RT_FLOAT   Cool_Radr_HeIII;
  CL_RT_FLOAT   Cool_Line_HI;
  CL_RT_FLOAT   Cool_Line_HeI;
  CL_RT_FLOAT   Cool_Line_HeII;
  CL_RT_FLOAT   Cool_Line_H2_H;   
  CL_RT_FLOAT   Cool_Line_H2_H2; 
  CL_RT_FLOAT   Cool_Line_H2_He; 
  CL_RT_FLOAT   Cool_Line_H2_e; 
  CL_RT_FLOAT   Cool_Line_H2_HII;  
  CL_RT_FLOAT   Cool_LowT;
} RATES_T;

/* Heating Cooling Context */

typedef struct CoolingPKDStruct { 
  double     z; /* Redshift */
  double     dTime;
 /* Rates independent of Temperature */ 
  RATES_NO_T  R;
 /* Table for Temperature dependent rates */ 
  int        nTable;
  double     TMin;
  double     TMax;
  double     TlnMin;
  double     TlnMax;
  double     rDeltaTln;
  RATES_T     *RT;
  
  int         bMetal; 
  int         nzMetalTable;
  int         nnHMetalTable;
  int         nTMetalTable;  
  double      MetalTMin; 
  double      MetalTMax; 
  double      MetalTlogMin;
  double      MetalTlogMax;
  double      rDeltaTlog;
  double      MetalnHMin; 
  double      MetalnHMax; 
  double      MetalnHlogMin; 
  double      MetalnHlogMax; 
  double      rDeltanHlog;
  double      MetalzMin; 
  double      MetalzMax;  
  double      rDeltaz;
  float       ***MetalCoolln;
  float       ***MetalHeatln;  
  double      *Rate_DustForm_H2; 
  
  int        nTableRead; /* number of Tables read from files */

  int        bUV;
  int        nUV;
  UVSPECTRUM *UV;
  int        bUVTableUsesTime;
  int        bUVTableLinear;
  int        bLowTCool;
  int        bSelfShield;
  
  int        bShieldHI;
  double     dClump; /* Subgrid clumping factor for determining rate of H2 formation on dust.  10 is a good value*/
#ifdef  RADIATIVEBOX
  int        bAgeFromMass; /*Set to true to determine age of star particle from mass compared to formation mass when calculating LW radiation.  Useful in running ICs which already have stars*/
  double     dLymanWernerFrac; /*  Set to true to determine age of star particle from mass compared to formation mass when calculating LW radiation.  Useful in running ICs which already have stars*/
#endif
  double     dGmPerCcUnit;
  double     dComovingGmPerCcUnit;
  double     dExpand; /*cosmological expansion factor*/
  double     dErgPerGmUnit;
  double     dSecUnit;
  double     dErgPerGmPerSecUnit;
  double     diErgPerGmUnit;
  double     dKpcUnit;
  double     dMsolUnit;
  double     dMassFracHelium;
  double     dInitStarMass; /* Mass of star particle at formation time,  Used when calculating LW radiation*/
  void       *DerivsData;

/* Diagnostic */
  int       its;
#if defined(COOLDEBUG)
  MDL        mdl; /* For diag/debug outputs */
  struct particle *p; /* particle pointer needed for SN feedback */
#endif 
} COOL;

typedef struct {
  double   T, Tln;
  double   Coll_HI;
  double   Coll_HeI;
  double   Coll_HeII;
  double   Coll_e_H2;  
  double   Coll_HI_H2;  
  double   Coll_H2_H2; 
  double   Coll_Hm_e;           /*gas phase formation of H2 */
  double   Coll_Hm_HII;          /*------------------- */
  double   Coll_HI_e;           /*------------------- */
  double   Coll_HII_H2;          /*--------------------- */
  double   HI_e;          /*---------------------- */
  double   HI_Hm;          /*gas phase formation of H2 */
  double   Radr_HII;
  double   Radr_HeII;
  double   Diel_HeII;
  double   Chtr_HeII;
  double   Totr_HeII;
  double   Radr_HeIII; 
  double   Cool_Metal;
  double   Heat_Metal;

  double   Phot_HI;
  double   Phot_HeI;
  double   Phot_HeII;
  double   Phot_H2;  /*Photon dissociation of H2*/
  double   DustForm_H2; /* Formation of H2 on dust */
  double   CorreLength; /* The correlation length of subgrid turbulence, used when calculating shielding*/
  /*#ifdef  RADIATIVEBOX*/
  double   LymanWernerCode;
  /*#endif*/
} RATE;

typedef struct {
  double compton;
  double bremHII;
  double bremHeII;
  double bremHeIII;
  double radrecHII;
  double radrecHeII;
  double radrecHeIII;
  double collionHI; 
  double collionHeI;
  double collionHeII;
  double collion_e_H2;  
  double collion_H_H2;  
  double collion_H2_H2;
  double collion_HII_H2;
  double dielrecHeII;
  double lineHI;
  double lineHeI;
  double lineHeII;
  double lineH2;  
  double lowT;
  double NetMetalCool; 
} COOL_ERGPERSPERGM;


typedef struct clDerivsDataStruct {
  void *IntegratorContext;
  COOL *cl;
  double rho,ExternalHeating,E,ZMetal,dLymanWerner, correL;
/*  double Y_H, Y_He; */  /* will be needed -- also for temperature , Y_MetalIon, Y_eMetal */
  RATE Rate;
  PERBARYON Y;
  double     Y_H, Y_He, Y_eMax;
  double     Y_Total0, Y_Total1;
  double     dlnE;
  int        its;  /* Debug */
  int        bCool;
} clDerivsData;

COOL *CoolInit( );
void CoolFinalize( COOL *cl );

void clInitConstants( COOL *cl, double dGMPerCcunit, double dComovingGmPerCcUnit,
		      double dErgPerGmUnit, double dSecUnit, double dKpcUnit, double dMsolUnit,  double dInitStarMass, COOLPARAM CoolParam);
void clInitUV(COOL *cl, int nTableColumns, int nTableRows, double *dTableData );
void clInitRatesTable( COOL *cl, double TMin, double TMax, int nTable );
void clReadMetalTable(COOL *cl, COOLPARAM clParam);
void clRateMetalTable(COOL *cl, RATE *Rate, double T, double rho, double Y_H, double ZMetal); 
void clHHeTotal(COOL *cl, double ZMetal); 
void CoolInitRatesTable( COOL *cl, COOLPARAM CoolParam);

void clRatesTableError( COOL *cl );
void clRatesRedshift( COOL *cl, double z, double dTime );
double clHeatTotal ( COOL *cl, PERBARYON *Y, RATE *Rate, double rho, double ZMetal );
void clRates( COOL *cl, RATE *Rate, double T, double rho, double ZMetal, double correL, double  Rate_Phot_H2_stellar);
double clCoolTotal( COOL *cl, PERBARYON *Y, RATE *Rate, double rho, double ZMetal );
COOL_ERGPERSPERGM  clTestCool ( COOL *cl, PERBARYON *Y, RATE *Rate, double rho );
void clPrintCool( COOL *cl, PERBARYON *Y, RATE *Rate, double rho );
void clPrintCoolFile( COOL *cl, PERBARYON *Y, RATE *Rate, double rho, double ZMetal, FILE *fp );

void clAbunds( COOL *cl, PERBARYON *Y, RATE *Rate, double rho, double ZMetal);
double clThermalEnergy( double Y_Total, double T );
double clTemperature( double Y_Total, double E );
double clSelfShield (double yH2, double h);
double clDustShield (double yHI, double yH2, double z, double h);
double clRateCollHI( double T );
double clRateCollHeI( double T );
double clRateCollHeII( double T );
double clRateColl_e_H2( double T );
double clRateColl_HI_H2( double T );
double clRateColl_H2_H2( double T );
double clRateColl_HII_H2(double T);
double clRateColl_Hm_e(double T);
double clRateColl_HI_e(double T);
double clRateColl_Hm_HII(double T);
double clRateHI_e(double T);
double clRateHI_Hm(double T);
double clRateRadrHII( double T );
double clRateRadrHeII( double T );
double clRateDielHeII( double T );
double clRateChtrHeII(double T);
double clRateRadrHeIII( double T );
double clCoolBrem1( double T );
double clCoolBrem2( double T );
double clCoolRadrHII( double T );
double clCoolRadrHeII( double T );
double clCoolRadrHeIII( double T );
double clCoolLineHI( double T );
double clCoolLineHeI( double T );
double clCoolLineHeII( double T );
double clCoolLineH2_table( double T );  
double clCoolLineH2_HI( double T );
double clCoolLineH2_H2( double T );
double clCoolLineH2_He( double T );
double clCoolLineH2_e( double T );
double clCoolLineH2_HII( double T );
double clCoolLowT( double T );
double clRateDustFormH2(double z, double clump); 
double clEdotInstant ( COOL *cl, PERBARYON *Y, RATE *Rate, double rho, double ZMetal );
void clIntegrateEnergy(COOL *cl, PERBARYON *Y, double *E, 
		       double ExternalHeating, double rho, double ZMetal, double dt, double correL, double dLymanWerner  );
void clIntegrateEnergyDEBUG(COOL *cl, PERBARYON *Y, double *E, 
		       double ExternalHeating, double rho, double ZMetal,  double dt );


void clDerivs(void *Data, double x, double *y, double *dydx) ;

void clJacobn(void *Data, double x, double y[], double dfdx[], double **dfdy) ;
  
void CoolAddParams( COOLPARAM *CoolParam, PRM );
void CoolLogParams( COOLPARAM *CoolParam, FILE *fp );
void CoolOutputArray( COOLPARAM *CoolParam, int, int *, char * );

#define COOL_ARRAY0_EXT  "HI"
FLOAT COOL_ARRAY0(COOL *cl, COOLPARTICLE *cp, double ZMetal);

#define COOL_ARRAY1_EXT  "HeI"
FLOAT COOL_ARRAY1(COOL *cl, COOLPARTICLE *cp, double ZMetal);

#define COOL_ARRAY2_EXT  "HeII"
FLOAT COOL_ARRAY2(COOL *cl, COOLPARTICLE *cp, double ZMetal);

#define COOL_ARRAY3_EXT  "H2"
FLOAT COOL_ARRAY3(COOL *cl, COOLPARTICLE *cp, double ZMetal);

FLOAT COOL_EDOT( COOL *cl_, COOLPARTICLE *cp_, double ECode_, double rhoCode_, double ZMetal_, double *posCode_, double correL_ );
#define COOL_EDOT( cl_, cp_, ECode_, rhoCode_, ZMetal_, posCode_, correL_) (CoolCodeWorkToErgPerGmPerSec( cl_, CoolEdotInstantCode( cl_, cp_, ECode_, rhoCode_, ZMetal_, posCode_ , correL_)))

FLOAT COOL_COOLING( COOL *cl_, COOLPARTICLE *cp_, double ECode_, double rhoCode_, double ZMetal_, double *posCode_ , double correL_);
#define COOL_COOLING( cl_, cp_, ECode_, rhoCode_, ZMetal_, posCode_, correL_) (CoolCodeWorkToErgPerGmPerSec( cl_, CoolCoolingCode( cl_, cp_, ECode_, rhoCode_, ZMetal_, posCode_ , correL_)))

FLOAT COOL_HEATING( COOL *cl_, COOLPARTICLE *cp_, double ECode_, double rhoCode_, double ZMetal_, double *posCode_, double correL_ );
#define COOL_HEATING( cl_, cp_, ECode_, rhoCode_, ZMetal_, posCode_, correL_) (CoolCodeWorkToErgPerGmPerSec( cl_, CoolHeatingCode( cl_, cp_, ECode_, rhoCode_, ZMetal_, posCode_ , correL_))) 

void clSetAbundanceTotals(COOL *cl, double ZMetal, double *Y_H, double *Y_He, double *Y_eMAX);
void CoolPARTICLEtoPERBARYON(COOL *cl_, PERBARYON *Y, COOLPARTICLE *cp, double ZMetal);
void CoolPERBARYONtoPARTICLE(COOL *cl_, PERBARYON *Y, COOLPARTICLE *cp, double ZMetal);

double CoolLymanWerner(COOL *cl, double fMassStar, double dlw);

double CoolEnergyToTemperature( COOL *Cool, COOLPARTICLE *cp, double E, double ZMetal);
double CoolCodeEnergyToTemperature( COOL *Cool, COOLPARTICLE *cp, double E, double ZMetal);

/* Note: nod to cosmology (z parameter) unavoidable unless we want to access cosmo.[ch] from here */
void CoolSetTime( COOL *Cool, double dTime, double z );

double CoolCodeTimeToSeconds( COOL *Cool, double dCodeTime );

#define CoolCodeTimeToSeconds( Cool, dCodeTime ) ((Cool)->dSecUnit*(dCodeTime))

double CoolSecondsToCodeTime( COOL *Cool, double dTime );

#define CoolSecondsToCodeTime( Cool, dTime ) ((dTime)/(Cool)->dSecUnit)

double CoolCodeEnergyToErgPerGm( COOL *Cool, double dCodeEnergy );

#define CoolCodeEnergyToErgPerGm( Cool, dCodeEnergy ) ((Cool)->dErgPerGmUnit*(dCodeEnergy))

double CoolErgPerGmToCodeEnergy( COOL *Cool, double dEnergy );

#define CoolErgPerGmToCodeEnergy( Cool, dEnergy ) ((Cool)->diErgPerGmUnit*(dEnergy))

double CoolCodeWorkToErgPerGmPerSec( COOL *Cool, double dCodeWork );

#define CoolCodeWorkToErgPerGmPerSec( Cool, dCodeWork ) ((Cool)->dErgPerGmPerSecUnit*(dCodeWork))

double CoolErgPerGmPerSecToCodeWork( COOL *Cool, double dWork );

#define CoolErgPerGmPerSecToCodeWork( Cool, dWork ) ((dWork)/(Cool)->dErgPerGmPerSecUnit)

double CodeDensityToComovingGmPerCc( COOL *Cool, double dCodeDensity );

#define CodeDensityToComovingGmPerCc( Cool, dCodeDensity )  ((Cool)->dComovingGmPerCcUnit*(dCodeDensity))

void CoolIntegrateEnergy(COOL *cl, COOLPARTICLE *cp, double *E, 
			 double ExternalHeating, double rho, double ZMetal, double tStep, double correL );

void CoolIntegrateEnergyCode(COOL *cl, COOLPARTICLE *cp, double *E, 
			     double ExternalHeating, double rho, double ZMetal, double *r, double tStep, double correL );

void CoolDefaultParticleData( COOLPARTICLE *cp );

void CoolInitEnergyAndParticleData( COOL *cl, COOLPARTICLE *cp, double *E, double dDensity, double dTemp, double ZMetal);

/* Deprecated */
double CoolHeatingRate( COOL *cl, COOLPARTICLE *cp, double E, double dDensity, double ZMetal, double correL);

double CoolEdotInstantCode(COOL *cl, COOLPARTICLE *cp, double ECode, 
			   double rhoCode, double ZMetal, double *posCode, double correL );
double CoolCoolingCode(COOL *cl, COOLPARTICLE *cp, double ECode, 
		       double rhoCode, double ZMetal, double *posCode, double correL );
double CoolHeatingCode(COOL *cl, COOLPARTICLE *cp, double ECode, 
		       double rhoCode, double ZMetal, double *posCode, double correL );

void CoolCodePressureOnDensitySoundSpeed( COOL *cl, COOLPARTICLE *cp, double uPred, double fDensity, double gamma, double gammam1, double *PoverRho, double *c );

/* Note: gamma should be 5/3 for this to be consistent! */
#define CoolCodePressureOnDensitySoundSpeed( cl__, cp__, uPred__, fDensity__, gamma__, gammam1__, PoverRho__, c__ ) { \
  *(PoverRho__) = ((5./3.-1)*(uPred__)); \
  *(c__) = sqrt((5./3.)*(*(PoverRho__))); }

/*
double CoolCodePressureOnDensity( COOL *cl, COOLPARTICLE *cp, double uPred, double fDensity, double gammam1 );

#define CoolCodePressureOnDensity( cl, cp, uPred, fDensity, gammam1 ) ((gammam1)*(uPred))
*/

struct inInitCooling {
  double dGmPerCcUnit;
  double dComovingGmPerCcUnit;
  double dErgPerGmUnit;
  double dSecUnit;
  double dKpcUnit;
  double dMsolUnit;
  double dInitStarMass;
  double z;
  double dTime;
  COOLPARAM CoolParam;
};

struct inInitEnergy {
	double dTuFac;
	double z;
	double dTime;
	};

void CoolTableReadInfo( COOLPARAM *CoolParam, int cntTable, int *nTableColumns, char *suffix );

void CoolTableRead( COOL *Cool, int nData, void *vData);

#endif

