#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include <rpc/types.h>
#include <rpc/xdr.h>

#include "pkd.h"
#include "starform.h"
#include "millerscalo.h"

#define max(A,B) ((A) > (B) ? (A) : (B))
#define min(A,B) ((A) < (B) ? (A) : (B))

#define MHYDR 1.67e-24 	/* mass of hydrogen atom in grams */
#ifdef STARFORM

#ifdef NOCOOLING
#error "STARFORM requires Cooling on"
#endif


/*
 * Star forming module for GASOLINE
 */

void stfmInitialize(STFM *pstfm)
{
    STFM stfm;
    
    stfm = (STFM) malloc(sizeof(struct stfmContext));
    assert(stfm != NULL);
    
    stfm->dPhysDenMin = 0;
    stfm->dOverDenMin = 0;
    stfm->dTempMax = 0;
    stfm->dCStar = 0;
    stfm->dSecUnit = 0;
    stfm->dGmPerCcUnit = 0;
    stfm->dGmUnit = 0;
    stfm->dStarEff = 0.0;
	stfm->dInitStarMass = 0.0;
    stfm->dMinGasMass = 0.0;
    stfm->dMinMassFrac = 0.0;
    stfm->dMaxStarMass = 0.0;
    stfm->dBHFormProb = 0.0; /* new BH params */
    stfm->bBHForm = 0;
    stfm->dInitBHMass = 0.0;
    *pstfm = stfm;
    }

void pkdStarLogInit(PKD pkd)
{
    STARLOG *pStarLog = &pkd->starLog;
    
    pStarLog->nLog = 0;
    pStarLog->nMaxLog = 1000;	/* inital size of buffer */
    pStarLog->nOrdered = 0;
    pStarLog->seTab = malloc(pStarLog->nMaxLog*sizeof(SFEVENT));
    srand(pkd->idSelf);
}

void pkdStarLogFlush(PKD pkd, char *pszFileName)
{
    FILE *fp;
    int iLog;
    XDR xdrs;
    
    if(pkd->starLog.nLog == 0)
	return;
    
    assert(pkd->starLog.nLog == pkd->starLog.nOrdered);
    
    fp = fopen(pszFileName, "a");
    assert(fp != NULL);
    xdrstdio_create(&xdrs,fp,XDR_ENCODE);
    for(iLog = 0; iLog < pkd->starLog.nLog; iLog++){
	SFEVENT *pSfEv = &(pkd->starLog.seTab[iLog]);
	xdr_int(&xdrs, &(pSfEv->iOrdStar));
	xdr_int(&xdrs, &(pSfEv->iOrdGas));
	xdr_double(&xdrs, &(pSfEv->timeForm));
	xdr_double(&xdrs, &(pSfEv->rForm[0]));
	xdr_double(&xdrs, &(pSfEv->rForm[1]));
	xdr_double(&xdrs, &(pSfEv->rForm[2]));
	xdr_double(&xdrs, &(pSfEv->vForm[0]));
	xdr_double(&xdrs, &(pSfEv->vForm[1]));
	xdr_double(&xdrs, &(pSfEv->vForm[2]));
	xdr_double(&xdrs, &(pSfEv->massForm));
	xdr_double(&xdrs, &(pSfEv->rhoForm));
	xdr_double(&xdrs, &(pSfEv->TForm));
#ifdef COOLING_MOLECULARH
	xdr_double(&xdrs, &(pSfEv->H2fracForm));
#endif
	}
    xdr_destroy(&xdrs);
    fclose(fp);
    pkd->starLog.nLog = 0;
    pkd->starLog.nOrdered = 0;
    }
    
/*
     taken from TREESPH and modified greatly.
     Uses the following formula for the star formation rate:

              d(ln(rhostar))/dt=cstar*rhogas/tdyn

*/


void stfmFormStars(STFM stfm, PKD pkd, PARTICLE *p,
		   double dTime, /* current time */
		   int *nFormed, /* number of stars formed */
		   double *dMassFormed,	/* mass of stars formed */
		   int *nDeleted) /* gas particles deleted */
{
    double tdyn;
    double tform;
    double tsound;
    double tcool;
    double E,T;
    COOL *cl = pkd->Cool;
    double dExp = 1.0/(1.0 + cl->z);
    double dCosmoFac = dExp*dExp*dExp;
    PARTICLE starp;
    double dMprob;
    double dDeltaM;
    double l_jeans2;
    int small_jeans = 0;
    int j;
    int newbh; /* tracking whether a new seed BH has formed JMB  */
 #ifdef COOLING_MOLECULARH
    double correL = 1.0;/*correlation length used for H2 shielding, CC*/
    double yH;
#endif
   
    /*  This version of the code has only three conditions unless 
	-D SFCONDITIONS is set:
	  converging flow (p->divv < 0)
	  T < dTempMax
	  density > dOverdenmin && density > dPhysDenMin
	Anil - Nov. 2003
    */

    /*
     * Is particle in convergent part of flow?
     */

#ifndef DIVVOFF
    if(p->divv >= 0.0)
	return;
#endif /*DIVVOFF*/
#ifdef JEANSSF
    T = CoolCodeEnergyToTemperature( cl, &p->CoolParticle, p->u, p->fMetals );
    l_jeans2 = M_PI*p->c*p->c/p->fDensity*dCosmoFac;
#endif
    /*
     * Determine dynamical time.
     */
    tdyn = 1.0/sqrt(4.0*M_PI*p->fDensity/dCosmoFac);

    /*
     * Determine cooling time.
     */


    T = CoolCodeEnergyToTemperature( cl, &p->CoolParticle, p->u, p->fMetals );
#ifdef  COOLING_MOLECULARH
 #ifdef NEWSHEAR
    /***** particle diffusion method ******/
    if (p->diff != 0) 
      correL = p->c * (0.25*p->fBall2)/p->diff; 
    if (correL > sqrt(0.25*p->fBall2) || p->diff == 0) 
      correL = sqrt(0.25*p->fBall2);
#else/*NEWSHEAR*/
    /***** particle shear method********/ 
    if ((p->curlv[0] != 0) && (p->curlv[1] != 0) && (p->curlv[2] != 0)) 
      correL = p->c/sqrt(p->curlv[0]*p->curlv[0] + p->curlv[1]*p->curlv[1] + p->curlv[2]*p->curlv[2]);    
#endif/*NEWSHEAR*/
#ifdef COLUMNLENGTH
    /***** From particle smoothing.  This works best for determining the correlation length.  CC 7/20/11 ******/
    correL = sqrt(0.25*p->fBall2);
#endif /*COLUMNLENGTH*/
#endif/* COOLING_MOLECULARH */

#if (0)
    E = CoolCodeEnergyToErgPerGm( cl, p->u );
#ifdef  COOLING_MOLECULARH
    tcool = E/(-CoolHeatingRate( cl, &p->CoolParticle, T, 
				 CodeDensityToComovingGmPerCc(cl,p->fDensity ), p->fMetals, correL )
	       -CoolCodeWorkToErgPerGmPerSec( cl, p->PdV ), correL);
#else
    tcool = E/(-CoolHeatingRate( cl, &p->CoolParticle, T, 
		 CodeDensityToComovingGmPerCc(cl,p->fDensity ), p->fMetals )
	       -CoolCodeWorkToErgPerGmPerSec( cl, p->PdV ));
#endif
    tcool = CoolSecondsToCodeTime( cl, tcool ); 
    printf("tcool %i: %g %g %g\n",p->iOrder,T,p->fDensity,tcool);
#endif

    tcool = p->u/(
#ifdef DENSITYU
#ifdef COOLING_MOLECULARH
		  -CoolEdotInstantCode( cl, &p->CoolParticle, p->u, p->fDensityU, p->fMetals,  p->r, correL )
#else
		  -CoolEdotInstantCode( cl, &p->CoolParticle, p->u, p->fDensityU, p->fMetals,  p->r)
#endif
#else
#ifdef  COOLING_MOLECULARH
		  -CoolEdotInstantCode( cl, &p->CoolParticle, p->u, p->fDensity, p->fMetals, p->r, correL )
#else
		  -CoolEdotInstantCode( cl, &p->CoolParticle, p->u, p->fDensity, p->fMetals,  p->r)
#endif
#endif
	-p->PdV );
#ifdef CHECKSF
    p->tOff = CoolCodeTimeToSeconds( cl, p->fTimeCoolIsOffUntil - dTime)/3.1557e7;  /* years */
    p->tcool = CoolCodeTimeToSeconds( cl, tcool)/3.1557e7;
    p->tdyn = CoolCodeTimeToSeconds( cl, tdyn)/3.1557e7;
    p->ratiosounddyn = sqrt(0.25*p->fBall2)/p->c/tdyn;
    p->l_jeans = sqrt(M_PI*p->c*p->c/p->fDensity*dCosmoFac);
    p->small_jeans = small_jeans;
#endif
#ifdef SFCONDITIONS
    if(tcool < 0.0 && T > stfm->dTempMax) return;
    /*
     * Determine sound crossing time.
     */
    tsound = sqrt(0.25*p->fBall2)/p->c;

    /* 
     * criteria that stars form if the Jean's length is less than the
     * softening
     */
    l_jeans2 = M_PI*p->c*p->c/p->fDensity*dCosmoFac;
#if (0)
/* Old code: problem -- compares physics L_J to comoving softening */
    if (l_jeans2 < p->fSoft*p->fSoft*stfm->dSoftMin*stfm->dSoftMin) 
        small_jeans = 1;
#ifdef CHECKSF
    p->small_jeans = small_jeans;
#endif /*CHECKSF*/
    if (!small_jeans && tsound <= tdyn)
        return;

#else /* old code (0) */
/* New code: physical L_J vs. physics smoothing length (with multiplier) */
    if (l_jeans2 >= 0.25*p->fBall2*dExp*dExp*stfm->dSoftMin*stfm->dSoftMin) return;

#ifdef CHECKSF
    p->small_jeans = 1;
#endif
#endif /* old code (0) */

#else /* CHECKSF */
    if(T > stfm->dTempMax) return;
#endif /*SFCONDITIONS*/

    /*
     * Determine if this particle satisfies all conditions.
     */
    
    if(p->fDensity < stfm->dOverDenMin ||
       p->fDensity/dCosmoFac < stfm->dPhysDenMin)
	return;

    if(tcool < 0.0 || tdyn > tcool || T < stfm->dTempMax)
	tform = tdyn;
    else
	tform = tcool;
#ifdef COOLING_MOLECULARH
    if (p->fMetals <= 0.1) yH = 1.0 - 4.0*((0.236 + 2.1*p->fMetals)/4.0) - p->fMetals;
    else yH = 1.0 - 4.0*((-0.446*(p->fMetals - 0.1)/0.9 + 0.446)/4.0) - p->fMetals;

    /* For non-zero values of dStarFormEfficiencyH2, set SF efficiency as a multiple of H2 fractional abundance and dStarFormEfficiencyH2, CC*/
    if (stfm->dStarFormEfficiencyH2 == 0) 
         dMprob = 1.0 - exp(-stfm->dCStar*stfm->dDeltaT/tform);
    else dMprob = 1.0 - exp(-stfm->dCStar*stfm->dDeltaT/tform*
         stfm->dStarFormEfficiencyH2*(2.0*p->CoolParticle.f_H2/yH));
#ifdef MOLECFRAC_SF_CUTOFF /*Flag to limit star formation to particles with an H2 abundance greater than a threshold value (0.1 below) */
    if (2.0*p->CoolParticle.f_H2/yH < 0.1) dMprob = 0;
#endif /* MOLECULAR_SF_CUTOFF */
#ifdef RHOSF
    /* This is an implementation of SF in which it scales linearly with density above a certain threshold (100 amu/cc below)*/
    if (p->fDensity/dCosmoFac > 100*MHYDR/stfm->dGmPerCcUnit) tform = 1.0/sqrt(4.0*M_PI*100.0*MHYDR/stfm->dGmPerCcUnit);
    dMprob = 1.0 - exp(-stfm->dCStar*stfm->dDeltaT/tform*
		       stfm->dStarFormEfficiencyH2*(2.0*p->CoolParticle.f_H2/yH));
#endif /* RHOSF */
#else  /* COOLING_MOLECULARH */  
    dMprob = 1.0 - exp(-stfm->dCStar*stfm->dDeltaT/tform);
#ifdef RHOSF
    tform = 1.0/sqrt(4.0*M_PI*stfm->dPhysDenMin);
    dMprob = 1.0 - exp(-stfm->dCStar*stfm->dDeltaT/tform);
#endif /* RHOSF */
#endif /* COOLING_MOLECULARH */    

    /*
     * Decrement mass of particle.
     */

    if (stfm->dInitStarMass > 0) 
        dDeltaM = stfm->dInitStarMass;
    else 
        dDeltaM = p->fMass*stfm->dStarEff;

    /* No negative or very tiny masses please! */
    if ( (dDeltaM > p->fMass) ) dDeltaM = p->fMass;

    if(dMprob*p->fMass < dDeltaM*(rand()/((double) RAND_MAX)))
	return;

    /* 
     * Note on number of stars formed:
     * n = log(dMinGasMass/dInitMass)/log(1-dStarEff) = max no. stars 
     * formed per gas particle, e.g. if min gas mass = 10% initial mass,
     * dStarEff = 1/3, max no. stars formed = 6 (round up so gas mass 
     * goes below min gas mass)
     */

    starp = *p; 		/* grab copy before possible deletion */

    /*
     * form star
     */

    starp.fTimeForm = dTime;
    starp.fBallMax = 0.0;
    starp.iGasOrder = starp.iOrder; /* iOrder gets reassigned in
				       NewParticle() */

    /* Seed BH Formation JMB 1/19/09*/
     newbh = 0;  /* BH tracker */
     if (stfm->bBHForm == 1 && starp.fMetals <= 1.0e-6 && stfm->dBHFormProb > (rand()/((double) RAND_MAX ))) {
       starp.fTimeForm = -1.0*starp.fTimeForm;
       newbh = 1;      
       /* Decrement mass of particle.*/
       if (stfm->dInitBHMass > 0) 
	 dDeltaM = stfm->dInitBHMass;  /* reassigning dDeltaM to be initBHmass JMB 6/16/09 */
       else 
	 dDeltaM = p->fMass*stfm->dStarEff;
       /* No negative or very tiny masses please! */
       if ( (dDeltaM > p->fMass) ) dDeltaM = p->fMass;
       p->fMass -= dDeltaM;
       assert(p->fMass >= 0.0);
       starp.fMass = dDeltaM;
       starp.fMassForm = dDeltaM;
	 }
     else {
       p->fMass -= dDeltaM;
       assert(p->fMass >= 0.0);
       starp.fMass = dDeltaM;
       starp.fMassForm = dDeltaM;
     }

    if(p->fMass < stfm->dMinGasMass) {
		(*nDeleted)++;
		pkdDeleteParticle(pkd, p);
		}


    /*
     * Log Star formation quantities
     */
    {
	STARLOG *pStarLog = &pkd->starLog;
	SFEVENT *pSfEv;
	if(pStarLog->nLog >= pStarLog->nMaxLog) {
	    /* Grow table */
	    pStarLog->nMaxLog *= 1.4;
	    pStarLog->seTab = realloc(pStarLog->seTab,
				      pStarLog->nMaxLog*sizeof(SFEVENT));
	    assert(pStarLog->seTab != NULL);
	}
	/* take care of iOrder assignment later */
	pSfEv = &(pStarLog->seTab[pStarLog->nLog]);
	pSfEv->timeForm = starp.fTimeForm;
	pSfEv->iOrdGas = starp.iOrder;
	for(j = 0; j < 3; j++) {
	    pSfEv->rForm[j] = starp.r[j];
	    pSfEv->vForm[j] = starp.v[j];
	    }
	pSfEv->massForm = starp.fMassForm;
	pSfEv->rhoForm = starp.fDensity/dCosmoFac;
	pSfEv->TForm = T;
#ifdef COOLING_MOLECULARH /* Output the H2 fractional abundance in the gas particle*/
	pSfEv->H2fracForm = 2.0*p->CoolParticle.f_H2/yH;
#endif
	pStarLog->nLog++;
	}
    
    starp.fNSNtot = 0.0;

	/* NB: It is important that the star inherit special properties of the gas
	   particle such as being a target for movies or other tracing
	   Thus: Do not remove all the TYPE properties -- just the gas specific ones */
    TYPEReset(&starp, TYPE_GAS|TYPE_TREEACTIVE|TYPE_SMOOTHACTIVE|TYPE_ACTIVE);
    if(newbh == 0) TYPESet(&starp, TYPE_STAR) ; /* if it's a BH make it a SINK  JMB  */
    else TYPESet(&starp, TYPE_SINK);
    TYPEReset(&starp, TYPE_NbrOfACTIVE); /* just a precaution */
    
    (*nFormed)++;
    *dMassFormed += dDeltaM;
    
    pkdNewParticle(pkd, starp);    

}

void pkdFormStars(PKD pkd, STFM stfm, double dTime, int *nFormed,
		  double *dMassFormed, int *nDeleted)
{
    int i;
    PARTICLE *p;
    int n = pkdLocal(pkd);
    
    *nFormed = 0;
    *nDeleted = 0;
    *dMassFormed = 0.0;
    
    for(i = 0; i < n; ++i) {
        p = &pkd->pStore[i];
        if(pkdIsGas(pkd, p))
            stfmFormStars(stfm, pkd, p, dTime, nFormed, dMassFormed, nDeleted);
        assert(p->u >= 0);
        assert(p->uPred >= 0);
        assert(p->fMass >= 0);
        }
    }

#endif

#ifdef SIMPLESF
void pkdSimpleStarForm(PKD pkd, double dRateCoeff, double dTMax, double dDenMin, double dDelta, double dTime,
					   double dInitStarMass, double dESNPerStarMass, double dtCoolingShutoff, int bdivv,
					   int *nFormed, /* number of stars formed */
					   double *dMassFormed,	/* mass of stars formed */
					   int *nDeleted) /* gas particles deleted */
{
    int i,j;
    PARTICLE *p;
    int n = pkdLocal(pkd);

    double T;
    COOL *cl = pkd->Cool;

	double mstardot;
    PARTICLE starp;
    
    *nFormed = 0;
    *nDeleted = 0;
    *dMassFormed = 0.0;

    for(i = 0; i < n; ++i) {
        p = &pkd->pStore[i];
/*		if(TYPEFilter(p,TYPE_GAS|TYPE_ACTIVE,TYPE_GAS|TYPE_ACTIVE)) { */
		if(TYPEFilter(p,TYPE_GAS,TYPE_GAS)) {
			/* Make sure cool again up to date */
			if (p->fTimeForm < dTime) p->fTimeForm = dTime;

			/* Ref: stfmFormStars(stfm, pkd, p, dTime, nFormed, dMassFormed, nDeleted); */
			/* Is particle in convergent part of flow?  */
			if (p->fDensity < dDenMin || (bdivv && p->divv >= 0.0)) continue;
			
			if ((T = CoolCodeEnergyToTemperature(pkd->Cool,&p->CoolParticle, p->u, p->fMetals)) > dTMax) continue; 
			
			mstardot = dRateCoeff*sqrt(p->fDensity)*(p->fMass-p->fMassStar); /* Predictor corrector for second order? */

			p->fMassStar += mstardot*dDelta; /* sanity checks occur later */

			/* Star formation event? */
			if (p->fMassStar > dInitStarMass) { 
				starp = *p; 		/* grab copy before possible deletion */
				starp.fESN = p->u + dESNPerStarMass; /* ESN per unit mass -- includes gas internal energy */

				if (p->fMassStar > p->fMass-0.5*dInitStarMass) {
					starp.fMass = p->fMass;
					p->fMassStar = 0;
					(*nDeleted)++;
					pkdDeleteParticle(pkd, p);
					}
				else {
					starp.fMass = dInitStarMass;
					p->fMass -= dInitStarMass;
					p->fMassStar -= dInitStarMass;
					assert(p->fMass > 0);
					}

				starp.PdV = dtCoolingShutoff; /* Max local Cooling shutoff period */

				/*
				 * Save quantities -- as per old STARFORM
				 */
				for(j = 0; j < 3; j++) {
					starp.rForm[j] = starp.r[j];
					starp.vForm[j] = starp.v[j];
					}
				starp.u = T;
				starp.iGasOrder = starp.iOrder; /* iOrder gets reassigned in NewParticle() */

				starp.fTimeForm = dTime;
				starp.fBallMax = 0.0;
    
				/* NB: It is important that the star inherit special properties of the gas
				   particle such as being a target for movies or other tracing
				   Thus: Do not remove all the TYPE properties -- just gas specific ones */
				TYPEReset(&starp, TYPE_GAS);
				TYPESet(&starp, TYPE_STAR);

				/* Energy distribution */
				TYPESet(&starp, TYPE_SMOOTHACTIVE);

				(*nFormed)++;
				*dMassFormed += starp.fMass;
				
				pkdNewParticle(pkd, starp);    
				}
			}
		}
	}

#endif