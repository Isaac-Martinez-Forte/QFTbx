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

/* CVS $Id: l_loga.c,v 1.21 2014/01/30 17:24:10 cxsc Exp $ */

/****************************************************************/
/*                                                              */
/*      Filename        : l_loga.c                              */
/*                                                              */
/*      Entries         : multiprecision l_loga(x,a)            */
/*                        multiprecision x,a;                   */
/*                                                              */
/*      Arguments       : x = argument of standard function     */
/*                        a = base of logarithm                 */
/*                                                              */
/*      Description     : logarithm of base a                   */
/*                                                              */
/****************************************************************/

#ifndef ALL_IN_ONE
#ifdef AIX
#include "/u/p88c/runtime/o_defs.h"
#else
#include "o_defs.h"
#endif
#define local
#endif

#ifdef LINT_ARGS
local multiprecision l_loga(multiprecision x,multiprecision a)
#else
local multiprecision l_loga(x,a)

multiprecision x;
multiprecision a;
#endif
        {
        multiprecision res;
        a_intg rc;

        E_TPUSH("l_loga")

        l_init(&res);
        if (res==NULL)
           e_trap(ALLOCATION,2,E_TMSG,65);
        else if ((rc = Lloga(x,a,res))!=0) {
           e_trap(INV_ARG,6,E_TMLT+E_TEXT(17),&x,
                            E_TMLT+E_TEXT(7),&a,
                            E_TINT+E_TEXT(16),&rc);
           (void)b_bclr(res);
           }

        if (x->f) l_free(&x);
        if (a->f) l_free(&a);

        E_TPOPP("l_loga")
        return(res);
        }





