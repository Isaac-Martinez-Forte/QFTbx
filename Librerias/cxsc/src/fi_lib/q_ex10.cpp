/*
**  CXSC is a C++ library for eXtended Scientific Computing (V 2.5.4)
**
**  Copyright (C) 1990-2000 Institut fuer Angewandte Mathematik,
**                          Universitaet Karlsruhe, Germany
**            (C) 2000-2014 Wiss. Rechnen/Softwaretechnologie
**                          Universitaet Wuppertal, Germany   
**
**  This library is free software; you can redistribute it and/or
**  modify it under the terms of the GNU Library General Public
**  License as published by the Free Software Foundation; either
**  version 2 of the License, or (at your option) any later version.
**
**  This library is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
**  Library General Public License for more details.
**
**  You should have received a copy of the GNU Library General Public
**  License along with this library; if not, write to the Free
**  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/* CVS $Id: q_ex10.cpp,v 1.15 2014/01/30 17:23:55 cxsc Exp $ */

#ifndef Q_EX10_CPP
#define Q_EX10_CPP

#include "fi_lib.hpp" 

namespace fi_lib{

 using cxsc::real;

/* --------------------------------------------------------------------- */
/* - Computation of 10^x, table lookup method                          - */
/* - We use the idea of Mr. P.T.P. Tang                                - */
/* --------------------------------------------------------------------- */

 real q_ex10(real x){
  int j;
  long int n,m;
  real r,r1,r2,q,s;
  real res;


 /* Step 1: Special cases  */
 if(NANTEST(x))                                       /* Test: x=NaN */
      res=q_abortnan(INV_ARG,&x,5);
 else {
 if ((-q_ext1<x) && (x<q_ext1))                      /* |x|<2^-54 */
   res=x+1;
 else
  { if (q_extm<x) 
      res=q_abortr1(OVER_FLOW,&x,5);                 /* Overflow */
    else
      { if (x<q_extn)
	  res=0;                                     /* Result: Underflow  */ 
	else
	  {
	    /* Step 2 */
	    if (x>0) n=CUTINT((x*q_e10i)+0.5);       /* 32/lg10 =106... */
	    else     n=CUTINT((x*q_e10i)-0.5);       /* round (x)       */
	    j=n % 32;                                /* j=n mod 32      */
	    if (j<0) j+=32;                          /* We force j>=0   */
	    m=(n-j)/32;
	    r1=x-n*q_e1l1;
            r2=-(n*q_e1l2);
	    
	    /* Step 3 */   
            r=r1+r2;
            q=(((((q_exd[6]*r+q_exd[5])*r+q_exd[4])*r+q_exd[3])*r+q_exd[2])*r
                            +q_exd[1])*r+q_exd[0];
	    q=r*q;
            q=r1+(r2+q);

	    /* Step 4 */
	    s=q_exld[j]+q_extl[j];
	    res=(q_exld[j]+(q_extl[j]+s*q));
	    POWER2(res,m);
	  }
      }
    }
  }

  return(res);
 }

} // Namespace

#endif





