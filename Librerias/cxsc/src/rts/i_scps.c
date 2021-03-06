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

/* CVS $Id: i_scps.c,v 1.21 2014/01/30 17:24:09 cxsc Exp $ */

/****************************************************************/
/*                                                              */
/*      Filename        : i_scps.c                              */
/*                                                              */
/*      Entries         : a_intv i_scps(r,s,n,rnd)              */
/*                        a_intv r[];                           */
/*                        a_intv s[];                           */
/*                        a_intg n;                             */
/*                        a_intg rnd;                           */
/*                                                              */
/*      Arguments       : n   = size of arrays r and s          */
/*                        r   = real interval vector            */
/*                        s   = real interval vector            */
/*                        rnd = mode                            */
/*                              0  clear accus first            */
/*                              4  do not clera accus           */
/*                                                              */
/*      Function value  : Real interval dotproduct              */
/*                                                              */
/*      Description     : Dotproduct for static real interval   */
/*                        arrays.                               */
/*                                                              */
/****************************************************************/

#ifndef ALL_IN_ONE
#ifdef AIX
#include "/u/p88c/runtime/o_defs.h"
#else
#include "o_defs.h"
#endif
#define local
extern dotprecision b_acrl;
extern dotprecision b_acru;
#endif

#ifdef LINT_ARGS
local a_intv i_scps(a_intv r[],a_intv s[],a_intg n,a_intg rnd)
#else
local a_intv i_scps(r,s,n,rnd)

a_intv r[];
a_intv s[];
a_intg n;
a_intg rnd;
#endif
        {
        a_intg i;
        a_intv res;

        E_TPUSH("i_scps")

        /* force linkage of module containing global variables only */
#if VAX_VMS_C
        b_accu();
#endif

        if (rnd<3)
           {
           d_clr(&b_acrl);
           d_clr(&b_acru);
           }

        for (i=0;i<n;i++)
           i_padd(&b_acrl,&b_acru,r[i],s[i]);

        res = i_ista(b_acrl,b_acru);

        E_TPOPP("i_scps")
        return(res);
        }





