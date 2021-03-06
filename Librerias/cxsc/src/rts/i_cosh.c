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

/* CVS $Id: i_cosh.c,v 1.21 2014/01/30 17:24:08 cxsc Exp $ */

/****************************************************************/
/*                                                              */
/*      Filename        : i_cosh.c                              */
/*                                                              */
/*      Entries         : a_intv i_cosh(a)                      */
/*                        a_intv a;                             */
/*                                                              */
/*      Arguments       : a = interval argument of function     */
/*                                                              */
/*      Description     : Interval hyperboliccosine             */
/*                                                              */
/****************************************************************/

#ifndef ALL_IN_ONE
#ifdef AIX
#include "/u/p88c/runtime/o_defs.h"
#else
#include "o_defs.h"
#endif
#define local
extern a_real *r_one_;
#endif

#ifdef LINT_ARGS
local a_intv i_cosh( a_intv a )
#else
local a_intv i_cosh( a )
a_intv a;   /* interval argument */
#endif

{
        a_btyp        rc;
        a_intv          res;
        a_real            dummy;

        E_SPUSH("i_cosh")

        /* --- point argument --- */
        if (i_point(a)) {
            /* printf("point argument!\n"); */
            rc =  i_invp(Lcosh,a.INF,&res.INF,&res.SUP);
        }
        /* --- interval argument --- */
        else if (i_iv(a)) {
            /* printf("interval argument!\n"); */

            if (r_sign(a.SUP)<0) {
                rc =  i_invp(Lcosh,a.SUP,&res.INF,&dummy);
                rc += i_invp(Lcosh,a.INF,&dummy,&res.SUP);
            }
            else if (r_sign(a.INF)>0) {
                rc =  i_invp(Lcosh,a.INF,&res.INF,&dummy);
                rc += i_invp(Lcosh,a.SUP,&dummy,&res.SUP);
            }
            else { /* 0.0 in a */
                R_ASSIGN(res.INF,*r_one_);
                rc=i_invp(Lcosh,
                          (r_gt(r_umin(a.INF),a.SUP)?a.INF:a.SUP),
                          &res.SUP,&dummy);
            }
        }
        /* --- invalid argument --- */
        else rc = 1;

        /* --- error --- */
        if (rc)
            e_trap(INV_ARG,4,E_TDBL+E_TEXT(5),&a.INF,
                             E_TDBL+E_TEXT(6),&a.SUP);

        E_SPOPP("i_cosh")
        return(res);
}





