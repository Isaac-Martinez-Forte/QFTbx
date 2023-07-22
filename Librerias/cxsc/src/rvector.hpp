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

/* CVS $Id: rvector.hpp,v 1.34 2014/01/30 17:23:48 cxsc Exp $ */

#ifndef _CXSC_RVECTOR_HPP_INCLUDED
#define _CXSC_RVECTOR_HPP_INCLUDED

#include "xscclass.hpp"
#include "dot.hpp"
#include "idot.hpp"
#include "cidot.hpp"
#include "real.hpp"
#include "except.hpp"
#include "vector.hpp"

#include <iostream>

namespace cxsc {

class srvector;
class srvector_slice;
class rvector_slice;

//! The Data Type rvector
/*!
The vectors of C-XSC are one dimensional arrays of the corresponding scalar base type. Every vector has a lower index
bound \f$ lb \f$ and an upper index bound \f$ ub \f$ of type int. Hence, the number \f$ n \f$ of components actually stored in a vector is
\f[
n = ub - lb + 1
\f]

All matrix and vector operators which require dot product computations use higher precision dot products provided by the dotprecision classes. The precision to be used for these implicit dot products can be choosen by setting the global variable opdotprec accordingly. A value of 0 means maximum accuracy (the default, always used by all older C-XSC versions), a value of 1 means double accuracy, a value of 2 or higher means k-fold double accuracy. Lower accuracy leads to (significantly) faster computing times, but also to less exact results. For all dot products with an interval result, error bounds are computed to guarantee a correct enclosure. For all other dot products approximations without error bounds are computed.

\sa cxsc::dotprecision
*/
class rvector
{
	friend class cvector;
	friend class cvector_slice;
	friend class cmatrix;
	friend class rvector_slice;
	friend class rmatrix;
	friend class rmatrix_subv;
	friend class ivector;
	friend class imatrix;
	friend class l_ivector;
	friend class l_imatrix;
	friend class l_rvector;
	friend class l_rmatrix;
	friend class civector;
	friend class cimatrix;
//#if(CXSC_INDEX_CHECK)
	//------------ Templates --------------------------------------------------

#ifdef _CXSC_FRIEND_TPL
template <class V,class MS,class S> friend void _vmsconstr(V &v,const MS &m)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__TYPE_CAST_OF_THICK_OBJ<MS>);
#else
	noexcept(false);
#endif
template <class V,class M,class S> friend void _vmconstr(V &v,const M &m)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__TYPE_CAST_OF_THICK_OBJ<M>);
#else
	noexcept(false);
#endif
 template <class V> friend 	void _vresize(V &rv) noexcept(false);
 template <class V,class S> friend 	void _vresize(V &rv, const int &len)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__WRONG_BOUNDARIES<V>);
#else
	noexcept(false);
#endif
 template <class V,class S> friend 	void _vresize(V &rv, const int &lb, const int &ub)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__WRONG_BOUNDARIES<V>);
#else
	noexcept(false);
#endif
 template <class V1,class V2,class S> friend 	 V1 &_vvassign(V1 &rv1,const V2 &rv2) noexcept(false);
 template <class V,class S> friend 	 V & _vsassign(V &rv,const S &r) noexcept(false);
 template <class V,class VS,class S> friend 	 V & _vvsassign(V &rv,const VS &sl) noexcept(false);
 template <class VS,class V> friend 	 VS & _vsvassign(VS &sl,const V &rv)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS>);
#else
	noexcept(false);
#endif
template <class V,class M,class S> friend  V &_vmassign(V &v,const M &m)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__TYPE_CAST_OF_THICK_OBJ<M>);
#else
	noexcept(false);
#endif
template <class M,class V,class S> friend  M &_mvassign(M &m,const V &v) noexcept(false);
template <class MV,class V> friend  MV &_mvvassign(MV &v,const V &rv)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__OP_WITH_WRONG_DIM<MV>);
#else
	noexcept(false);
#endif
template <class MV,class V> friend  V _mvabs(const MV &mv) noexcept(false);

	//-------- vector-vector -----------------
 template <class DP,class V1,class V2> friend 	void _vvaccu(DP &dp, const V1 & rv1, const V2 &rv2)
#if(CXSC_INDEX_CHECK)
		noexcept(false);
#else
	noexcept(false);
#endif
 template <class DP,class VS,class V> friend 	void _vsvaccu(DP &dp, const VS & sl, const V &rv)
#if(CXSC_INDEX_CHECK)
		noexcept(false);
#else
	noexcept(false);
#endif


 template <class V1,class V2,class E> friend 	 E _vvmult(const V1 & rv1, const V2 &rv2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V1>);
#else
	noexcept(false);
#endif
 template <class VS,class V,class E> friend 	 E _vsvmult(const VS & sl, const V &rv)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V>);
#else
	noexcept(false);
#endif
 template <class V,class S> friend 	 V &_vsmultassign(V &rv,const S &r) noexcept(false);
 template <class VS,class S> friend 	 VS &_vssmultassign(VS &rv,const S &r) noexcept(false);
 template <class VS,class S> friend 	 VS &_vssdivassign(VS &rv,const S &r) noexcept(false);
 template <class V1,class V2,class E> friend 	 E _vvplus(const V1 &rv1, const V2 &rv2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V1>);
#else
	noexcept(false);
#endif
 template <class V,class VS,class E> friend 	 E _vvsplus(const V &rv,const VS &sl)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V>);
#else
	noexcept(false);
#endif
 template <class VS1,class VS2,class E> friend 	 E _vsvsplus(const VS1 &s1,const VS2 &s2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS1>);
#else
	noexcept(false);
#endif
 template <class VS1,class VS2,class E> friend 	 E _vsvsminus(const VS1 &s1,const VS2 &s2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS1>);
#else
	noexcept(false);
#endif
 template <class V1,class V2> friend 	 V1 &_vvplusassign(V1 &rv1, const V2 &rv2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V1>);
#else
	noexcept(false);
#endif
 template <class V,class VS> friend 	 V &_vvsplusassign(V &rv, const VS &sl)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V>);
#else
	noexcept(false);
#endif
 template <class VS,class V> friend 	 VS &_vsvplusassign(VS &sl, const V &rv)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS>);
#else
	noexcept(false);
#endif
 template <class VS1,class VS2> friend 	 VS1 &_vsvsplusassign(VS1 &sl1, const VS2 &sl2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS1>);
#else
	noexcept(false);
#endif
 template <class VS1,class VS2> friend 	 VS1 &_vsvsminusassign(VS1 &sl1, const VS2 &sl2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS1>);
#else
	noexcept(false);
#endif
 template <class V1,class V2> friend 	 V1 &_vvminusassign(V1 &rv1, const V2 &rv2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V1>);
#else
	noexcept(false);
#endif
 template <class V,class VS> friend 	 V &_vvsminusassign(V &rv, const VS &sl)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V>);
#else
	noexcept(false);
#endif
 template <class VS,class V> friend 	 VS &_vsvminusassign(VS &sl, const V &rv)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS>);
#else
	noexcept(false);
#endif
 template <class V> friend 	 V _vminus(const V &rv) noexcept(false);
 template <class VS,class V> friend 	 V _vsminus(const VS &sl) noexcept(false);
 template <class V1,class V2,class E> friend 	 E _vvminus(const V1 &rv1, const V2 &rv2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<E>);
#else
	noexcept(false);
#endif
 template <class V,class VS,class E> friend 	 E _vvsminus(const V &rv, const VS &sl)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<E>);
#else
	noexcept(false);
#endif
 template <class VS,class V,class E> friend 	 E _vsvminus(const VS &sl,const V &rv)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<E>);
#else
	noexcept(false);
#endif
 template <class MV1,class MV2,class E> friend 	 E _mvmvconv(const MV1 &rv1, const MV2 &rv2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<E>);
#else
	noexcept(false);
#endif
 template <class MV,class V,class E> friend 	 E _mvvconv(const MV &rv1, const V &rv2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<E>);
#else
	noexcept(false);
#endif
template <class V,class MV> friend  V &_vmvconvassign(V &rv,const MV &v)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__OP_WITH_WRONG_DIM<V>);
#else
	noexcept(false);
#endif

 template <class M,class V,class E> friend 	 E _mvmult(const M &m,const V &v)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<M>);
#else
	noexcept(false);
#endif
 template <class M,class V,class E> friend 	 E _mvimult(const M &m,const V &v)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<M>);
#else
	noexcept(false);
#endif
 template <class M,class V,class E> friend 	 E _mvcmult(const M &m,const V &v)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<M>);
#else
	noexcept(false);
#endif
 template <class M,class V,class E> friend 	 E _mvcimult(const M &m,const V &v)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<M>);
#else
	noexcept(false);
#endif
 template <class V,class M,class E> friend 	 E _vmmult(const V &v,const M &m)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<M>);
#else
	noexcept(false);
#endif
 template <class V,class M,class E> friend 	 E _vmimult(const V &v,const M &m)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<M>);
#else
	noexcept(false);
#endif
 template <class V,class M,class E> friend 	 E _vmcmult(const V &v,const M &m)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<M>);
#else
	noexcept(false);
#endif
 template <class V,class M,class E> friend 	 E _vmcimult(const V &v,const M &m)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<M>);
#else
	noexcept(false);
#endif
 template <class V,class M,class S> friend 	 V &_vmmultassign(V &v,const M &m)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<M>);
#else
	noexcept(false);
#endif
 template <class V,class M,class S> friend 	 V &_vmimultassign(V &v,const M &m)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<M>);
#else
	noexcept(false);
#endif
 template <class V,class M,class S> friend 	 V &_vmcmultassign(V &v,const M &m)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<M>);
#else
	noexcept(false);
#endif
 template <class V,class M,class S> friend 	 V &_vmcimultassign(V &v,const M &m)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<M>);
#else
	noexcept(false);
#endif
 template <class MS,class V,class E> friend 	 E _msvmult(const MS &ms,const V &v)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<MS>);
#else
	noexcept(false);
#endif
 template <class MS,class V,class E> friend 	 E _msvimult(const MS &ms,const V &v)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<MS>);
#else
	noexcept(false);
#endif
 template <class MS,class V,class E> friend 	 E _msvcmult(const MS &ms,const V &v)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<MS>);
#else
	noexcept(false);
#endif
 template <class MS,class V,class E> friend 	 E _msvcimult(const MS &ms,const V &v)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<MS>);
#else
	noexcept(false);
#endif
 template <class V,class MS,class E> friend 	 E _vmsmult(const V &v,const MS &ms)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<MS>);
#else
	noexcept(false);
#endif
 template <class V,class MS,class E> friend 	 E _vmsimult(const V &v,const MS &ms)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<MS>);
#else
	noexcept(false);
#endif
 template <class V,class MS,class E> friend 	 E _vmscmult(const V &v,const MS &ms)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<MS>);
#else
	noexcept(false);
#endif
 template <class V,class MS,class E> friend 	 E _vmscimult(const V &v,const MS &ms)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<MS>);
#else
	noexcept(false);
#endif
 template <class V,class MS,class S> friend 	 V &_vmsmultassign(V &v,const MS &ms)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<MS>);
#else
	noexcept(false);
#endif
 template <class V,class MS,class S> friend 	 V &_vmsimultassign(V &v,const MS &ms)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<MS>);
#else
	noexcept(false);
#endif
 template <class V,class MS,class S> friend 	 V &_vmscmultassign(V &v,const MS &ms)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<MS>);
#else
	noexcept(false);
#endif
 template <class V,class MS,class S> friend 	 V &_vmscimultassign(V &v,const MS &ms)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<MS>);
#else
	noexcept(false);
#endif
 template <class V,class MV,class S> friend 	 S _vmvmult(const V &rv1, const MV &rv2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<MV>);
#else
	noexcept(false);
#endif
 template <class V,class MV,class S> friend 	 S _vmvimult(const V &rv1, const MV &rv2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<MV>);
#else
	noexcept(false);
#endif
 template <class V,class MV,class S> friend 	 S _vmvcmult(const V &rv1, const MV &rv2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<MV>);
#else
	noexcept(false);
#endif
 template <class V,class MV,class S> friend 	 S _vmvcimult(const V &rv1, const MV &rv2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<MV>);
#else
	noexcept(false);
#endif
 template <class MV,class S,class E> friend 	 E _mvsmult(const MV &rv, const S &s) noexcept(false);
 template <class MV1,class MV2,class E> friend 	 E _mvmvplus(const MV1 &rv1, const MV2 &rv2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<E>);
#else
	noexcept(false);
#endif
 template <class MV,class V,class E> friend 	 E _mvvplus(const MV &rv1, const V &rv2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<E>);
#else
	noexcept(false);
#endif
 template <class MV,class V,class E> friend 	 E _mvvminus(const MV &rv1, const V &rv2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<E>);
#else
	noexcept(false);
#endif
 template <class V,class MV,class E> friend 	 E _vmvminus(const V &rv1, const MV &rv2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<E>);
#else
	noexcept(false);
#endif
 template <class MV1,class MV2,class E> friend 	 E _mvmvminus(const MV1 &rv1, const MV2 &rv2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<E>);
#else
	noexcept(false);
#endif
template <class MV,class V> friend  MV &_mvvplusassign(MV &v,const V &rv)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__OP_WITH_WRONG_DIM<MV>);
#else
	noexcept(false);
#endif
template <class MV,class V> friend  MV &_mvvminusassign(MV &v,const V &rv)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__OP_WITH_WRONG_DIM<MV>);
#else
	noexcept(false);
#endif

 template <class V1,class V2,class E> friend 	 E _vvconv(const V1 &rv1, const V2 &rv2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<E>);
#else
	noexcept(false);
#endif
 template <class V,class VS,class E> friend 	 E _vvsconv(const V &rv,const VS &sl)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<E>);
#else
	noexcept(false);
#endif
	
template <class DP,class V,class SV> friend 	void _vmvaccu(DP &dp, const V & rv1, const SV &rv2)
#if(CXSC_INDEX_CHECK)
		noexcept(false);
#else
	noexcept(false);
#endif

	//--------- vector-scalar ----------------
 template <class V,class S> friend 	 V &_vsdivassign(V &rv,const S &r) noexcept(false);
 template <class VS,class S,class E> friend 	 E _vssdiv(const VS &sl, const S &s) noexcept(false);
 template <class V,class S,class E> friend 	 E _vsmult(const V &rv, const S &s) noexcept(false);
 template <class VS,class S,class E> friend 	 E _vssmult(const VS &sl, const S &s) noexcept(false);
 template <class V1,class V2> friend 	 bool _vveq(const V1 &rv1, const V2 &rv2) noexcept(false);
 template <class VS,class V> friend 	 bool _vsveq(const VS &sl, const V &rv) noexcept(false);
 template <class V1,class V2> friend 	 bool _vvneq(const V1 &rv1, const V2 &rv2) noexcept(false);
 template <class VS,class V> friend 	 bool _vsvneq(const VS &sl, const V &rv) noexcept(false);
 template <class V1,class V2> friend 	 bool _vvless(const V1 &rv1, const V2 &rv2) noexcept(false);
 template <class VS,class V> friend 	 bool _vsvless(const VS &sl, const V &rv) noexcept(false);
 template <class V1,class V2> friend 	 bool _vvleq(const V1 &rv1, const V2 &rv2) noexcept(false);
 template <class VS,class V> friend 	 bool _vsvleq(const VS &sl, const V &rv) noexcept(false);
 template <class V,class VS> friend 	 bool _vvsless(const V &rv, const VS &sl) noexcept(false);
 template <class V,class VS> friend 	 bool _vvsleq(const V &rv, const VS &sl) noexcept(false);
 template <class V> friend 	 bool _vnot(const V &rv) noexcept(false);
 template <class V> friend 	void *_vvoid(const V &rv) noexcept(false);
 template <class V,class E> friend 	 E _vabs(const V &rv) noexcept(false);
 template <class VS,class E> friend 	 E _vsabs(const VS &sl) noexcept(false);
 template <class VS1,class VS2> friend 	 bool _vsvseq(const VS1 &sl1, const VS2 &sl2) noexcept(false);
 template <class VS1,class VS2> friend 	 bool _vsvsneq(const VS1 &sl1, const VS2 &sl2) noexcept(false);
 template <class VS1,class VS2> friend 	 bool _vsvsless(const VS1 &sl1, const VS2 &sl2) noexcept(false);
 template <class VS1,class VS2> friend 	 bool _vsvsleq(const VS1 &sl1, const VS2 &sl2) noexcept(false);
 template <class VS> friend 	 bool _vsnot(const VS &sl) noexcept(false);
 template <class VS> friend 	void *_vsvoid(const VS &sl) noexcept(false);
 template <class V> friend 	std::ostream &_vout(std::ostream &s, const V &rv) noexcept(false);
 template <class V> friend 	std::istream &_vin(std::istream &s, V &rv) noexcept(false);

template <class V,class MV2,class S> friend  V &_vmvassign(V &v,const MV2 &rv) noexcept(false);
 template <class MV,class S,class E> friend 	 E _mvsdiv(const MV &rv, const S &s) noexcept(false);
	// Interval


template <class MV,class V> friend  MV &_mvvconvassign(MV &v,const V &rv)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__OP_WITH_WRONG_DIM<MV>);
#else
	noexcept(false);
#endif
template <class MV,class V> friend  MV &_mvvsectassign(MV &v,const V &rv)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__OP_WITH_WRONG_DIM<MV>);
#else
	noexcept(false);
#endif

	//--- Interval ---- vector-vector ----------
 template <class V1,class V2> friend 	 V1 &_vvsetinf(V1 &rv1, const V2 &rv2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V1>);
#else
	noexcept(false);
#endif
 template <class V1,class V2> friend 	 V1 &_vvusetinf(V1 &rv1, const V2 &rv2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V1>);
#else
	noexcept(false);
#endif
 template <class V1,class V2> friend 	 V1 &_vvsetsup(V1 &rv1, const V2 &rv2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V1>);
#else
	noexcept(false);
#endif
 template <class V1,class V2> friend 	 V1 &_vvusetsup(V1 &rv1, const V2 &rv2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V1>);
#else
	noexcept(false);
#endif
 template <class VS,class V> friend 	 VS &_vsvsetinf(VS &sl, const V &rv)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS>);
#else
	noexcept(false);
#endif
 template <class VS,class V> friend 	 VS &_vsvusetinf(VS &sl, const V &rv)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS>);
#else
	noexcept(false);
#endif
 template <class VS,class V> friend 	 VS &_vsvsetsup(VS &sl, const V &rv)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS>);
#else
	noexcept(false);
#endif
 template <class VS,class V> friend 	 VS &_vsvusetsup(VS &sl, const V &rv)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS>);
#else
	noexcept(false);
#endif
template <class MV,class V> friend  MV &_mvvsetinf(MV &v,const V &rv)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__OP_WITH_WRONG_DIM<MV>);
#else
	noexcept(false);
#endif
template <class MV,class V> friend  MV &_mvvusetinf(MV &v,const V &rv)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__OP_WITH_WRONG_DIM<MV>);
#else
	noexcept(false);
#endif
template <class MV,class V> friend  MV &_mvvsetsup(MV &v,const V &rv)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__OP_WITH_WRONG_DIM<MV>);
#else
	noexcept(false);
#endif
template <class MV,class V> friend  MV &_mvvusetsup(MV &v,const V &rv)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__OP_WITH_WRONG_DIM<MV>);
#else
	noexcept(false);
#endif
 template <class V,class E> friend 	 E _vdiam(const V &rv) noexcept(false);
 template <class V,class E> friend 	 E _vmid(const V &rv) noexcept(false);
 template <class V,class E> friend 	 E _vinf(const V &rv) noexcept(false);
 template <class V,class E> friend 	 E _vsup(const V &rv) noexcept(false);
 template <class VS,class E> friend 	 E _vsdiam(const VS &sl) noexcept(false);
 template <class VS,class E> friend 	 E _vsmid(const VS &sl) noexcept(false);
 template <class VS,class E> friend 	 E _vsinf(const VS &sl) noexcept(false);
 template <class VS,class E> friend 	 E _vssup(const VS &sl) noexcept(false);
template <class MV,class V> friend  V _mvdiam(const MV &mv) noexcept(false);
template <class MV,class V> friend  V _mvmid(const MV &mv) noexcept(false);
template <class MV,class V> friend  V _mvinf(const MV &mv) noexcept(false);
template <class MV,class V> friend  V _mvsup(const MV &mv) noexcept(false);


 template <class V1,class V2,class E> friend 	 E _vvimult(const V1 & rv1, const V2 &rv2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V1>);
#else
	noexcept(false);
#endif
 template <class VS,class V,class E> friend 	 E _vsvimult(const VS & sl, const V &rv)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V>);
#else
	noexcept(false);
#endif

 template <class V1,class V2> friend 	 V1 &_vvconvassign(V1 &rv1, const V2 &rv2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V1>);
#else
	noexcept(false);
#endif
 template <class V1,class V2,class E> friend 	 E _vvsect(const V1 &rv1, const V2 &rv2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V1>);
#else
	noexcept(false);
#endif
 template <class V,class VS,class E> friend 	 E _vvssect(const V &rv,const VS &sl)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<E>);
#else
	noexcept(false);
#endif
 template <class V1,class V2> friend 	 V1 &_vvsectassign(V1 &rv1, const V2 &rv2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V1>);
#else
	noexcept(false);
#endif

 template <class VS1,class VS2,class E> friend 	 E _vsvsconv(const VS1 &s1,const VS2 &s2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<E>);
#else
	noexcept(false);
#endif
 
 template <class V,class VS> friend 	 V &_vvsconvassign(V &rv, const VS &sl)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V>);
#else
	noexcept(false);
#endif
 template <class VS,class V> friend 	 VS &_vsvconvassign(VS &sl, const V &rv)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS>);
#else
	noexcept(false);
#endif
 template <class VS1,class VS2> friend 	 VS1 &_vsvsconvassign(VS1 &sl1, const VS2 &sl2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS1>);
#else
	noexcept(false);
#endif

 template <class VS,class V> friend 	 VS &_vsvsectassign(VS &sl, const V &rv)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS>);
#else
	noexcept(false);
#endif
	//--- Interval ---- vector-scalar ----------
	
 template <class V,class S,class E> friend 	 E _vsdiv(const V &rv, const S &s) noexcept(false);

	
	// complex

 template <class V,class E> friend 	 E _vim(const V &rv) noexcept(false);
 template <class V,class E> friend 	 E _vre(const V &rv) noexcept(false);
template <class MV,class V> friend  V _mvim(const MV &mv) noexcept(false);
template <class MV,class V> friend  V _mvre(const MV &mv) noexcept(false);
template <class MV,class V> friend  MV &_mvvsetim(MV &v,const V &rv)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__OP_WITH_WRONG_DIM<MV>);
#else
	noexcept(false);
#endif
template <class MV,class V> friend  MV &_mvvsetre(MV &v,const V &rv)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__OP_WITH_WRONG_DIM<MV>);
#else
	noexcept(false);
#endif
 template <class V1,class V2> friend 	 V1 &_vvsetim(V1 &rv1, const V2 &rv2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V1>);
#else
	noexcept(false);
#endif
 template <class V1,class V2> friend 	 V1 &_vvsetre(V1 &rv1, const V2 &rv2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V1>);
#else
	noexcept(false);
#endif
 template <class VS,class E> friend 	 E _vsim(const VS &sl) noexcept(false);
 template <class VS,class E> friend 	 E _vsre(const VS &sl) noexcept(false);
 template <class VS,class V> friend 	 VS &_vsvsetim(VS &sl, const V &rv)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS>);
#else
	noexcept(false);
#endif
 template <class VS,class V> friend 	 VS &_vsvsetre(VS &sl, const V &rv)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS>);
#else
	noexcept(false);
#endif
	
	//--- complex ---- vector-vector ----------
 template <class V1,class V2,class E> friend 	 E _vvcmult(const V1 & rv1, const V2 &rv2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V1>);
#else
	noexcept(false);
#endif
 template <class VS,class V,class E> friend 	 E _vsvcmult(const VS & sl, const V &rv)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V>);
#else
	noexcept(false);
#endif

                             // 5.10. S.W.

	//--- complex ---- vector-scalar ----------
	
	
	// cinterval ------------------------------
	//! Returns componentwise the supremum of the real part
	friend INLINE rvector SupRe(const civector &v) noexcept(false);
	//! Returns componentwise the supremum of the imaginary part
	friend INLINE rvector SupIm(const civector &v) noexcept(false);
	//! Returns componentwise the infimum of the real part
	friend INLINE rvector InfRe(const civector &v) noexcept(false);
	//! Returns componentwise the infimum of the imaginary part
	friend INLINE rvector InfIm(const civector &v) noexcept(false);
	//! Returns componentwise the supremum of the real part
	friend INLINE rvector SupRe(const civector_slice &v) noexcept(false);
	//! Returns componentwise the supremum of the imaginary part
	friend INLINE rvector SupIm(const civector_slice &v) noexcept(false);
	//! Returns componentwise the infimum of the real part
	friend INLINE rvector InfRe(const civector_slice &v) noexcept(false);
	//! Returns componentwise the infimum of the imaginary part
	friend INLINE rvector InfIm(const civector_slice &v) noexcept(false);

	// vector-vector                      // 5.10. S.W.

 template <class V1,class V2,class E> friend 	 E _vvcimult(const V1 & rv1, const V2 &rv2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V1>);
#else
	noexcept(false);
#endif
 template <class VS,class V,class E> friend 	 E _vsvcimult(const VS & sl, const V &rv)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V>);
#else
	noexcept(false);
#endif

 template <class VS1,class VS2,class E> friend 	 E _vsvscimult(const VS1 & sl1, const VS2 &sl2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS1>);
#else
	noexcept(false);
#endif
                                                    // 5.10. S.W.

	// vector-matrix

	//--- l_real ---- vector-matrix ----------
 template <class V,class MS,class E> friend 	 E _vmslmult(const V &v,const MS &ms)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<MS>);
#else
	noexcept(false);
#endif
 template <class M,class V,class E> friend 	 E _mvlmult(const M &m,const V &v)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<M>);
#else
	noexcept(false);
#endif
 template <class MS,class V,class E> friend 	 E _msvlmult(const MS &ms,const V &v)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<MS>);
#else
	noexcept(false);
#endif
	
 template <class V,class M,class E> friend 	 E _vmlmult(const V &v,const M &m)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<M>);
#else
	noexcept(false);
#endif
	
	//--- l_real ---- vector-vector ----------
 template <class V1,class V2,class E> friend 	 E _vvlmult(const V1 & rv1, const V2 &rv2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V1>);
#else
	noexcept(false);
#endif
 template <class VS,class V,class E> friend 	 E _vsvlmult(const VS & sl, const V &rv)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V>);
#else
	noexcept(false);
#endif

	// vector-vector

 template <class V1,class V2,class E> friend 	 E _vvlimult(const V1 & rv1, const V2 &rv2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V1>);
#else
	noexcept(false);
#endif
 template <class VS,class V,class E> friend 	 E _vsvlimult(const VS & sl, const V &rv)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V>);
#else
	noexcept(false);
#endif

 template <class VS1,class VS2,class E> friend 	 E _vsvslimult(const VS1 & sl1, const VS2 &sl2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS1>);
#else
	noexcept(false);
#endif

	// vector-matrix
 template <class M,class V,class E> friend 	 E _mvlimult(const M &m,const V &v)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<M>);
#else
	noexcept(false);
#endif
 template <class MS,class V,class E> friend 	 E _msvlimult(const MS &ms,const V &v)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<MS>);
#else
	noexcept(false);
#endif
 template <class V,class M,class E> friend 	 E _vmlimult(const V &v,const M &m)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<M>);
#else
	noexcept(false);
#endif
 template <class V,class MS,class E> friend 	 E _vmslimult(const V &v,const MS &ms)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<MS>);
#else
	noexcept(false);
#endif
#endif
	
	private:
	real *dat;
	int l,u,size;

	public:
        double* to_blas_array() const { return (double*)dat; }
	//------ Konstruktoren ----------------------------------------------------
	//! Constructor of class rvector
	INLINE rvector () noexcept(false);
	//! Constructor of class rvector
	explicit INLINE rvector(const int &i) noexcept(false);
#ifdef OLD_CXSC
	//! Constructor of class rvector
	explicit INLINE rvector(const class index &i) noexcept(false); // for backwards compatibility
#endif
	//! Constructor of class rvector
	explicit INLINE rvector(const int &i1,const int &i2)
#if(CXSC_INDEX_CHECK)
	throw(ERROR_RVECTOR_WRONG_BOUNDARIES,ERROR_RVECTOR_NO_MORE_MEMORY);
#else
	noexcept(false);
#endif
	//! Constructor of class rvector
	INLINE rvector(const rmatrix_subv &) noexcept(false);
	//! Constructor of class rvector
	explicit INLINE rvector(const real &) noexcept(false);
	//! Constructor of class rvector
	explicit INLINE rvector(const rmatrix &)
#if(CXSC_INDEX_CHECK)
	noexcept(false);
#else
	noexcept(false);
#endif
	//! Constructor of class rvector
	explicit INLINE rvector(const rmatrix_slice &sl)
#if(CXSC_INDEX_CHECK)
	noexcept(false);
#else
	noexcept(false);
#endif

	//! Constructor of class rvector
        INLINE rvector(const intvector&);

	//! Constructor of class rvector
	INLINE rvector(const rvector_slice &rs) noexcept(false);
	//! Constructor of class rvector
	INLINE rvector(const rvector &v) noexcept(false);
	//! Constructor of class rvector
	INLINE rvector(const srvector &v);
	//! Constructor of class rvector
	INLINE rvector(const srvector_slice &v);
	//! Implementation of standard assigning operator
	INLINE rvector &operator =(const rvector &rv) noexcept(false);
	//! Implementation of standard assigning operator
	INLINE rvector &operator =(const rvector_slice &sl) noexcept(false);
	//! Implementation of standard assigning operator
	INLINE rvector &operator =(const srvector &rv);
	//! Implementation of standard assigning operator
	INLINE rvector &operator =(const srvector_slice &sl);
	//! Implementation of standard assigning operator
	INLINE rvector &operator =(const real &r) noexcept(false);
	//! Implementation of standard assigning operator
	INLINE rvector &operator =(const rmatrix &)
#if(CXSC_INDEX_CHECK)
	noexcept(false);
#else
	noexcept(false);
#endif
	//! Implementation of standard assigning operator
	INLINE rvector &operator =(const rmatrix_slice &)
#if(CXSC_INDEX_CHECK)
	noexcept(false);
#else
	noexcept(false);
#endif
	//! Implementation of standard assigning operator
	INLINE rvector &operator =(const rmatrix_subv &) noexcept(false);

        //! Computes permutation of vector according to permutation vector, C=Px
        INLINE rvector operator()(const intvector& p);
        //! Computes permutation of vector according to permutation matrix, C=Px
        INLINE rvector operator()(const intmatrix& P);


	//--------- Destruktor ----------------------------------------------------
	INLINE ~rvector() { delete [] dat; }

	//------ Standardfunktionen -----------------------------------------------
	
	friend INLINE real::real(const rvector &)
#if(CXSC_INDEX_CHECK)
	throw(ERROR_RVECTOR_TYPE_CAST_OF_THICK_OBJ,ERROR_RVECTOR_USE_OF_UNINITIALIZED_OBJ);
#else
	noexcept(false);
#endif
	//! Returns the lower bound of the vector
	friend INLINE int Lb(const rvector &rv) noexcept(false) { return rv.l; }
	//! Returns the upper bound of the vector
	friend INLINE int Ub(const rvector &rv) noexcept(false) { return rv.u; }
	//! Returns the dimension of the vector
        friend INLINE int VecLen(const rvector &rv) noexcept(false) { return rv.size; }
	//! Sets the lower bound of the vector
	friend INLINE rvector &SetLb(rvector &rv, const int &l) noexcept(false) { rv.l=l; rv.u=l+rv.size-1; return rv; }
	//! Sets the upper bound of the vector
	friend INLINE rvector &SetUb(rvector &rv, const int &u) noexcept(false) { rv.u=u; rv.l=u-rv.size+1; return rv; }
	//! Operator for accessing the single elements of the vector (read-only)
	INLINE real & operator [](const int &i) const
#if(CXSC_INDEX_CHECK)
	noexcept(false);
#else
	noexcept(false);
#endif
	
	//! Operator for accessing the single elements of the vector
	INLINE real & operator [](const int &i)
#if(CXSC_INDEX_CHECK)
	noexcept(false);
#else
	noexcept(false);
#endif

	//! Operator for accessing the whole vector
	INLINE rvector & operator ()() noexcept(false) { return *this; }
	//! Operator for accessing a part of the vector
	INLINE rvector_slice operator ()(const int &i)
#if(CXSC_INDEX_CHECK)
	noexcept(false);
#else
	noexcept(false);
#endif
	//! Operator for accessing a part of the vector
	INLINE rvector_slice operator ()(const int &i1,const int &i2)
#if(CXSC_INDEX_CHECK)
	noexcept(false);
#else
	noexcept(false);
#endif

	//! Implementation of addition and allocation operation
	INLINE rvector &operator +=(const srvector &rv);
	//! Implementation of addition and allocation operation
	INLINE rvector &operator +=(const srvector_slice &rv);
	//! Implementation of addition and allocation operation
	INLINE rvector &operator -=(const srvector &rv);
	//! Implementation of addition and allocation operation
	INLINE rvector &operator -=(const srvector_slice &rv);
	
	INLINE operator void*() noexcept(false);
//#else
//#endif
};

//! The Data Type rvector_slice
/*!
This data type represents a partial rvector.

\sa rvector
*/
class rvector_slice
{
	friend class rvector;
	friend class rmatrix;
	friend class ivector;
	friend class imatrix;
	friend class cvector;
	friend class cmatrix;
	friend class civector;
	friend class cimatrix;
	friend class l_rvector;
	friend class l_rmatrix;
	friend class l_ivector;
	friend class l_imatrix;
	private:
	real *dat;
	int l,u,size;
	int start,end;

	public:
//#if(CXSC_INDEX_CHECK)	

#ifdef _CXSC_FRIEND_TPL
//------------------------- Templates -------------------------------------------
 template <class VS1,class VS2> friend 	 VS1 & _vsvsassign(VS1 &sl1,const VS2 &sl2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS1>);
#else
	noexcept(false);
#endif
 template <class V,class VS,class S> friend 	 V & _vvsassign(V &rv,const VS &sl) noexcept(false);
 template <class VS,class V> friend 	 VS & _vsvassign(VS &sl,const V &rv)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS>);
#else
	noexcept(false);
#endif
 template <class VS,class S> friend 	 VS & _vssassign(VS &sl,const S &r) noexcept(false);
 template <class VS,class M,class S> friend 	 VS &_vsmmultassign(VS &v,const M &m)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<M>);
#else
	noexcept(false);
#endif
	//-------- vector-scalar ------------------
 template <class VS,class S> friend 	 VS &_vssmultassign(VS &rv,const S &r) noexcept(false);
 template <class VS,class S> friend 	 VS &_vssdivassign(VS &rv,const S &r) noexcept(false);
	
 template <class VS,class V> friend 	 bool _vsveq(const VS &sl, const V &rv) noexcept(false);
 template <class VS,class V> friend 	 bool _vsvneq(const VS &sl, const V &rv) noexcept(false);
 template <class VS,class V> friend 	 bool _vsvless(const VS &sl, const V &rv) noexcept(false);
 template <class VS,class V> friend 	 bool _vsvleq(const VS &sl, const V &rv) noexcept(false);
 template <class V,class VS> friend 	 bool _vvsless(const V &rv, const VS &sl) noexcept(false);
 template <class V,class VS> friend 	 bool _vvsleq(const V &rv, const VS &sl) noexcept(false);
 template <class VS,class E> friend 	 E _vsabs(const VS &sl) noexcept(false);
 template <class VS1,class VS2> friend 	 bool _vsvseq(const VS1 &sl1, const VS2 &sl2) noexcept(false);
 template <class VS1,class VS2> friend 	 bool _vsvsneq(const VS1 &sl1, const VS2 &sl2) noexcept(false);
 template <class VS1,class VS2> friend 	 bool _vsvsless(const VS1 &sl1, const VS2 &sl2) noexcept(false);
 template <class VS1,class VS2> friend 	 bool _vsvsleq(const VS1 &sl1, const VS2 &sl2) noexcept(false);
 template <class VS> friend 	 bool _vsnot(const VS &sl) noexcept(false);
 template <class VS> friend 	void *_vsvoid(const VS &sl) noexcept(false);
 template <class V> friend 	std::ostream &_vsout(std::ostream &s, const V &rv) noexcept(false);
 template <class V> friend 	std::istream &_vsin(std::istream &s, V &rv) noexcept(false);
	//------- vector-vector -----------------
 template <class DP,class VS,class V> friend 	void _vsvaccu(DP &dp, const VS & sl, const V &rv)
#if(CXSC_INDEX_CHECK)
		noexcept(false);
#else
	noexcept(false);
#endif
 template <class DP,class VS1,class VS2> friend 	void _vsvsaccu(DP &dp, const VS1 & sl1, const VS2 &sl2)
#if(CXSC_INDEX_CHECK)
		noexcept(false);
#else
	noexcept(false);
#endif

 template <class VS,class S,class E> friend 	 E _vssdiv(const VS &sl, const S &s) noexcept(false);
 template <class VS,class S,class E> friend 	 E _vssmult(const VS &sl, const S &s) noexcept(false);
 template <class VS,class V,class E> friend 	 E _vsvmult(const VS & sl, const V &rv)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V>);
#else
	noexcept(false);
#endif
 template <class V,class VS,class E> friend 	 E _vvsplus(const V &rv,const VS &sl)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V>);
#else
	noexcept(false);
#endif
 template <class VS1,class VS2,class E> friend 	 E _vsvsplus(const VS1 &s1,const VS2 &s2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS1>);
#else
	noexcept(false);
#endif
 template <class VS1,class VS2,class E> friend 	 E _vsvsminus(const VS1 &s1,const VS2 &s2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS1>);
#else
	noexcept(false);
#endif
 template <class V,class VS> friend 	 V &_vvsplusassign(V &rv, const VS &sl)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V>);
#else
	noexcept(false);
#endif
 template <class VS,class V> friend 	 VS &_vsvplusassign(VS &sl, const V &rv)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS>);
#else
	noexcept(false);
#endif
 template <class VS1,class VS2> friend 	 VS1 &_vsvsplusassign(VS1 &sl1, const VS2 &sl2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS1>);
#else
	noexcept(false);
#endif
 template <class VS1,class VS2> friend 	 VS1 &_vsvsminusassign(VS1 &sl1, const VS2 &sl2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS1>);
#else
	noexcept(false);
#endif
 template <class V,class VS> friend 	 V &_vvsminusassign(V &rv, const VS &sl)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V>);
#else
	noexcept(false);
#endif
 template <class VS,class V> friend 	 VS &_vsvminusassign(VS &sl, const V &rv)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS>);
#else
	noexcept(false);
#endif
 template <class VS,class V> friend 	 V _vsminus(const VS &sl) noexcept(false);
 template <class V,class VS,class E> friend 	 E _vvsminus(const V &rv, const VS &sl)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<E>);
#else
	noexcept(false);
#endif
 template <class VS,class V,class E> friend 	 E _vsvminus(const VS &sl,const V &rv)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<E>);
#else
	noexcept(false);
#endif
 template <class VS1,class VS2,class E> friend 	 E _vsvsmult(const VS1 & sl1, const VS2 &sl2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS1>);
#else
	noexcept(false);
#endif

 template <class V,class VS,class E> friend 	 E _vvsconv(const V &rv,const VS &sl)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<E>);
#else
	noexcept(false);
#endif
 template <class VS1,class VS2,class E> friend 	 E _vsvsconv(const VS1 &s1,const VS2 &s2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<E>);
#else
	noexcept(false);
#endif
 template <class V,class VS,class E> friend 	 E _vvssect(const V &rv,const VS &sl)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<E>);
#else
	noexcept(false);
#endif
 template <class VS1,class VS2,class E> friend 	 E _vsvssect(const VS1 &s1,const VS2 &s2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<E>);
#else
	noexcept(false);
#endif
	// interval -----------
 template <class V,class VS> friend 	 V &_vvssetinf(V &rv, const VS &sl)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V>);
#else
	noexcept(false);
#endif
 template <class V,class VS> friend 	 V &_vvsusetinf(V &rv, const VS &sl)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V>);
#else
	noexcept(false);
#endif
 template <class V,class VS> friend 	 V &_vvssetsup(V &rv, const VS &sl)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V>);
#else
	noexcept(false);
#endif
 template <class V,class VS> friend 	 V &_vvsusetsup(V &rv, const VS &sl)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V>);
#else
	noexcept(false);
#endif
 template <class VS1,class VS2> friend 	 VS1 &_vsvssetinf(VS1 &sl1, const VS2 &sl2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS1>);
#else
	noexcept(false);
#endif
 template <class VS1,class VS2> friend 	 VS1 &_vsvsusetinf(VS1 &sl1, const VS2 &sl2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS1>);
#else
	noexcept(false);
#endif
 template <class VS1,class VS2> friend 	 VS1 &_vsvssetsup(VS1 &sl1, const VS2 &sl2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS1>);
#else
	noexcept(false);
#endif
 template <class VS1,class VS2> friend 	 VS1 &_vsvsusetsup(VS1 &sl1, const VS2 &sl2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS1>);
#else
	noexcept(false);
#endif
	// interval  -----------------


 template <class V,class VS> friend 	 V &_vvsconvassign(V &rv, const VS &sl)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V>);
#else
	noexcept(false);
#endif
 template <class VS,class V> friend 	 VS &_vsvconvassign(VS &sl, const V &rv)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS>);
#else
	noexcept(false);
#endif
 template <class VS1,class VS2> friend 	 VS1 &_vsvsconvassign(VS1 &sl1, const VS2 &sl2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS1>);
#else
	noexcept(false);
#endif

 template <class V,class VS> friend 	 V &_vvssectassign(V &rv, const VS &sl)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V>);
#else
	noexcept(false);
#endif
 template <class VS,class V> friend 	 VS &_vsvsectassign(VS &sl, const V &rv)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS>);
#else
	noexcept(false);
#endif
 template <class VS1,class VS2> friend 	 VS1 &_vsvssectassign(VS1 &sl1, const VS2 &sl2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS1>);
#else
	noexcept(false);
#endif
	// Interval
 
	//-- Interval ------- vector-vector ------------

 template <class VS,class V,class E> friend 	 E _vsvimult(const VS & sl, const V &rv)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V>);
#else
	noexcept(false);
#endif
 template <class VS1,class VS2,class E> friend 	 E _vsvsimult(const VS1 & sl1, const VS2 &sl2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS1>);
#else
	noexcept(false);
#endif

 template <class VS1,class VS2> friend 	 VS1 &_vsvssetim(VS1 &sl1, const VS2 &sl2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS1>);
#else
	noexcept(false);
#endif
 template <class VS1,class VS2> friend 	 VS1 &_vsvssetre(VS1 &sl1, const VS2 &sl2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS1>);
#else
	noexcept(false);
#endif
 template <class V,class VS> friend 	 V &_vvssetim(V &rv, const VS &sl)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V>);
#else
	noexcept(false);
#endif
 template <class V,class VS> friend 	 V &_vvssetre(V &rv, const VS &sl)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V>);
#else
	noexcept(false);
#endif
 
	//-- complex ------- vector-vector ------------

 template <class VS,class V,class E> friend 	 E _vsvcmult(const VS & sl, const V &rv)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V>);
#else
	noexcept(false);
#endif
 template <class VS1,class VS2,class E> friend 	 E _vsvscmult(const VS1 & sl1, const VS2 &sl2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS1>);
#else
	noexcept(false);
#endif

 template <class VS,class V,class E> friend 	 E _vsvcimult(const VS & sl, const V &rv)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V>);
#else
	noexcept(false);
#endif
 template <class VS1,class VS2,class E> friend 	 E _vsvscimult(const VS1 & sl1, const VS2 &sl2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS1>);
#else
	noexcept(false);
#endif

 template <class VS,class V,class E> friend 	 E _vsvlmult(const VS & sl, const V &rv)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V>);
#else
	noexcept(false);
#endif
 template <class VS1,class VS2,class E> friend 	 E _vsvslmult(const VS1 & sl1, const VS2 &sl2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS1>);
#else
	noexcept(false);
#endif

 template <class VS,class V,class E> friend 	 E _vsvlimult(const VS & sl, const V &rv)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<V>);
#else
	noexcept(false);
#endif
 template <class VS1,class VS2,class E> friend 	 E _vsvslimult(const VS1 & sl1, const VS2 &sl2)
#if(CXSC_INDEX_CHECK)
		throw(ERROR__OP_WITH_WRONG_DIM<VS1>);
#else
	noexcept(false);
#endif

	// l_interval -- vector-matrix

	
#endif
	
	
	//--------------------- Konstruktoren -----------------------------------
	//! Constructor of class rvector_slice
	explicit INLINE rvector_slice(rvector &a, const int &lb, const int &ub) noexcept(false):dat(a.dat),l(a.l),u(a.u),size(ub-lb+1),start(lb),end(ub) { }
	//! Constructor of class rvector_slice
	explicit INLINE rvector_slice(rvector_slice &a, const int &lb, const int &ub) noexcept(false):dat(a.dat),l(a.l),u(a.u),size(ub-lb+1),start(lb),end(ub) { }
	public: 
	//! Constructor of class rvector_slice
	INLINE rvector_slice(const rvector_slice &a) noexcept(false):dat(a.dat),l(a.l),u(a.u),size(a.size),start(a.start),end(a.end) { }
	public:
	//! Implementation of standard assigning operator
	INLINE rvector_slice & operator =(const rvector_slice &sl)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__OP_WITH_WRONG_DIM<rvector>);
#else
	noexcept(false);
#endif
	//! Implementation of standard assigning operator
	INLINE rvector_slice & operator =(const rvector &rv)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__OP_WITH_WRONG_DIM<rvector>);
#else
	noexcept(false);
#endif
	//! Implementation of standard assigning operator
	INLINE rvector_slice & operator =(const srvector &rv);
	//! Implementation of standard assigning operator
	INLINE rvector_slice & operator =(const srvector_slice &rv);
	//! Implementation of standard assigning operator
	INLINE rvector_slice & operator =(const real &r) noexcept(false);
	//! Implementation of standard assigning operator
	INLINE rvector_slice & operator =(const rmatrix &m)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__OP_WITH_WRONG_DIM<rvector>,ERROR_RMATRIX_TYPE_CAST_OF_THICK_OBJ);
#else
	noexcept(false);
#endif
	//! Implementation of standard assigning operator
	INLINE rvector_slice & operator =(const rmatrix_slice &m)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__OP_WITH_WRONG_DIM<rvector>,ERROR_RMATRIX_TYPE_CAST_OF_THICK_OBJ);
#else
	noexcept(false);
#endif
	//! Implementation of standard assigning operator
	INLINE rvector_slice &operator =(const rmatrix_subv &) noexcept(false);

	//--------------------- Standardfunktionen ------------------------------

	friend INLINE real::real(const rvector_slice &sl)
#if(CXSC_INDEX_CHECK)
	throw(ERROR_RVECTOR_TYPE_CAST_OF_THICK_OBJ,ERROR_RVECTOR_USE_OF_UNINITIALIZED_OBJ);
#else
	noexcept(false);
#endif
	//! Returns the lower bound of the vector
	friend INLINE int Lb(const rvector_slice &sl) noexcept(false) { return sl.start; }
	//! Returns the upper bound of the vector
	friend INLINE int Ub(const rvector_slice &sl) noexcept(false) { return sl.end; }
	//! Returns the dimension of the vector
        friend INLINE int VecLen(const rvector_slice &sl) noexcept(false) { return sl.end-sl.start+1; }
	//! Operator for accessing the single elements of the vector
	INLINE real & operator [](const int &i)
#if(CXSC_INDEX_CHECK)
	noexcept(false);
#else
	noexcept(false);
#endif
	
	//! Operator for accessing the single elements of the vector (read-only)
	INLINE real & operator [](const int &i) const
#if(CXSC_INDEX_CHECK)
	noexcept(false);
#else
	noexcept(false);
#endif

	//! Operator for accessing the whole vector
	INLINE rvector_slice & operator ()() noexcept(false) { return *this; }
	//! Operator for accessing a part of the vector
	INLINE rvector_slice operator ()(const int &i)
#if(CXSC_INDEX_CHECK)
	noexcept(false);
#else
	noexcept(false);
#endif
	//! Operator for accessing a part of the vector
	INLINE rvector_slice operator ()(const int &i1,const int &i2)
#if(CXSC_INDEX_CHECK)
	noexcept(false);
#else
	noexcept(false);
#endif
	//! Implementation of division and allocation operation
	INLINE rvector_slice &operator /=(const real &r) noexcept(false);
	//! Implementation of multiplication and allocation operation
	INLINE rvector_slice &operator *=(const real &r) noexcept(false);
	//! Implementation of multiplication and allocation operation
	INLINE rvector_slice &operator *=(const rmatrix &m)
#if(CXSC_INDEX_CHECK)
	noexcept(false);
#else
	noexcept(false);
#endif
	//! Implementation of addition and allocation operation
	INLINE rvector_slice &operator +=(const rvector &rv)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__OP_WITH_WRONG_DIM<rvector>);
#else
	noexcept(false);
#endif
	//! Implementation of addition and allocation operation
	INLINE rvector_slice &operator +=(const rvector_slice &sl2)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__OP_WITH_WRONG_DIM<rvector>);
#else
	noexcept(false);
#endif
	//! Implementation of subtraction and allocation operation
	INLINE rvector_slice &operator -=(const rvector &rv)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__OP_WITH_WRONG_DIM<rvector>);
#else
	noexcept(false);
#endif
	//! Implementation of subtraction and allocation operation
	INLINE rvector_slice &operator -=(const rvector_slice &sl2)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__OP_WITH_WRONG_DIM<rvector>);
#else
	noexcept(false);
#endif

	//! Implementation of addition and allocation operation
	INLINE rvector_slice &operator +=(const srvector &rv);
	//! Implementation of addition and allocation operation
	INLINE rvector_slice &operator +=(const srvector_slice &rv);
	//! Implementation of addition and allocation operation
	INLINE rvector_slice &operator -=(const srvector &rv);
	//! Implementation of addition and allocation operation
	INLINE rvector_slice &operator -=(const srvector_slice &rv);


	INLINE operator void*() noexcept(false);
//#else
//#endif
};

//======================== Vector Functions =============================

	//! Deprecated typecast, which only exist for the reason of compatibility with older versions of C-XSC
	INLINE rvector _rvector(const real &r) noexcept(false); 
//	INLINE rvector _rvector(const rmatrix &m) noexcept(false);
//	INLINE rvector _rvector(const rmatrix_slice &sl) noexcept(false);

	//! Resizes the vector
	INLINE void Resize(rvector &rv) noexcept(false);
	//! Resizes the vector
	INLINE void Resize(rvector &rv, const int &len)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__WRONG_BOUNDARIES<rvector>);
#else
	noexcept(false);
#endif
	//! Resizes the vector
	INLINE void Resize(rvector &rv, const int &lb, const int &ub)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__WRONG_BOUNDARIES<rvector>);
#else
	noexcept(false);
#endif
	
	//! Returns the absolute value of the vector
	INLINE rvector abs(const rvector &rv) noexcept(false);
	//! Returns the absolute value of the vector
	INLINE rvector abs(const rvector_slice &sl) noexcept(false);
	//! Implementation of standard negation operation
	INLINE bool operator !(const rvector &rv) noexcept(false);
	//! Implementation of standard negation operation
	INLINE bool operator !(const rvector_slice &sl) noexcept(false);

//======================= Vector / Scalar ===============================

	//! Implementation of multiplication operation
	INLINE rvector operator *(const rvector &rv, const real &s) noexcept(false);
	//! Implementation of multiplication operation
	INLINE rvector operator *(const rvector_slice &sl, const real &s) noexcept(false);
	//! Implementation of multiplication operation
	INLINE rvector operator *(const real &s, const rvector &rv) noexcept(false);
	//! Implementation of multiplication operation
	INLINE rvector operator *(const real &s, const rvector_slice &sl) noexcept(false);
	//! Implementation of multiplication and allocation operation
	INLINE rvector &operator *=(rvector &rv,const real &r) noexcept(false);

	//! Implementation of division operation
	INLINE rvector operator /(const rvector &rv, const real &s) noexcept(false);
	//! Implementation of division operation
	INLINE rvector operator /(const rvector_slice &sl, const real &s) noexcept(false);
	//! Implementation of division and allocation operation
	INLINE rvector &operator /=(rvector &rv,const real &r) noexcept(false);

//======================= Vector / Vector ===============================

	//! The accurate sum of the elements of the vector added to the first argument
	void accumulate(dotprecision &dp, const rvector &);

	//! The accurate scalar product of the last two arguments added to the value of the first argument
	void accumulate(dotprecision &dp, const rvector & rv1, const rvector &rv2)
#if(CXSC_INDEX_CHECK)
	noexcept(false);
#else
	noexcept(false);
#endif

	//! The accurate scalar product of the last two arguments added to the value of the first argument (without error bound)
	void accumulate_approx(dotprecision &dp, const rvector & rv1, const rvector &rv2);


	//! The accurate scalar product of the last two arguments added to the value of the first argument
	void accumulate(dotprecision &dp, const rvector & rv1, const rmatrix_subv &rv2)
#if(CXSC_INDEX_CHECK)
	noexcept(false);
#else
	noexcept(false);
#endif

	//! The accurate scalar product of the last two arguments added to the value of the first argument (without error bound)
	void accumulate_approx(dotprecision &dp, const rvector & rv1, const rmatrix_subv &rv2);


	//! The accurate scalar product of the last two arguments added to the value of the first argument
	void accumulate(dotprecision &dp, const rmatrix_subv & rv1, const rvector &rv2)
#if(CXSC_INDEX_CHECK)
	noexcept(false);
#else
	noexcept(false);
#endif

	//! The accurate scalar product of the last two arguments added to the value of the first argument (without error bound)
	void accumulate_approx(dotprecision &dp, const rmatrix_subv & rv1, const rvector &rv2);

	//! The accurate scalar product of the last two arguments added to the value of the first argument
	void accumulate(dotprecision &dp,const rvector_slice &sl,const rvector &rv)
#if(CXSC_INDEX_CHECK)
	noexcept(false);
#else
	noexcept(false);
#endif

	//! The accurate scalar product of the last two arguments added to the value of the first argument (without error bound)
	void accumulate_approx(dotprecision &dp,const rvector_slice &sl,const rvector &rv);


	//! The accurate scalar product of the last two arguments added to the value of the first argument
	void accumulate(dotprecision &dp,const rvector &rv,const rvector_slice &sl)
#if(CXSC_INDEX_CHECK)
	noexcept(false);
#else
	noexcept(false);
#endif

	//! The accurate scalar product of the last two arguments added to the value of the first argument (without error bound)
	void accumulate_approx(dotprecision &dp,const rvector &rv,const rvector_slice &sl);


	//! The accurate scalar product of the last two arguments added to the value of the first argument
	void accumulate(dotprecision &dp, const rvector_slice & sl1, const rvector_slice &sl2)
#if(CXSC_INDEX_CHECK)
	noexcept(false);
#else
	noexcept(false);
#endif

	//! The accurate scalar product of the last two arguments added to the value of the first argument (without error bound)
	void accumulate_approx(dotprecision &dp, const rvector_slice & sl1, const rvector_slice &sl2);


	//! The accurate scalar product of the last two arguments added to the value of the first argument
	void accumulate(idotprecision &dp, const rvector & rv1, const rvector &rv2)
#if(CXSC_INDEX_CHECK)
	noexcept(false);
#else
	noexcept(false);
#endif
	//! The accurate scalar product of the last two arguments added to the value of the first argument
	void accumulate(idotprecision &dp, const rvector & rv1, const rmatrix_subv &rv2)
#if(CXSC_INDEX_CHECK)
	noexcept(false);
#else
	noexcept(false);
#endif
	//! The accurate scalar product of the last two arguments added to the value of the first argument
	void accumulate(idotprecision &dp, const rmatrix_subv & rv1, const rvector &rv2)
#if(CXSC_INDEX_CHECK)
	noexcept(false);
#else
	noexcept(false);
#endif
	//! The accurate scalar product of the last two arguments added to the value of the first argument
	void accumulate(idotprecision &dp,const rvector_slice &sl,const rvector &rv)
#if(CXSC_INDEX_CHECK)
	noexcept(false);
#else
	noexcept(false);
#endif
	//! The accurate scalar product of the last two arguments added to the value of the first argument
	void accumulate(idotprecision &dp,const rvector &rv,const rvector_slice &sl)
#if(CXSC_INDEX_CHECK)
	noexcept(false);
#else
	noexcept(false);
#endif
	//! The accurate scalar product of the last two arguments added to the value of the first argument
	void accumulate(idotprecision &dp, const rvector_slice & sl1, const rvector_slice &sl2)
#if(CXSC_INDEX_CHECK)
	noexcept(false);
#else
	noexcept(false);
#endif

	//! The accurate scalar product of the last two arguments added to the value of the first argument
	void accumulate(cdotprecision &dp, const rvector & rv1, const rvector &rv2)
#if(CXSC_INDEX_CHECK)
	noexcept(false);
#else
	noexcept(false);
#endif

	//! The accurate scalar product of the last two arguments added to the value of the first argument (without error bound)
	void accumulate_approx(cdotprecision &dp, const rvector & rv1, const rvector &rv2);

	//! The accurate scalar product of the last two arguments added to the value of the first argument
	void accumulate(cdotprecision &dp, const rvector & rv1, const rmatrix_subv &rv2)
#if(CXSC_INDEX_CHECK)
	noexcept(false);
#else
	noexcept(false);
#endif

	//! The accurate scalar product of the last two arguments added to the value of the first argument (without error bound)
	void accumulate_approx(cdotprecision &dp, const rvector & rv1, const rmatrix_subv &rv2);

	//! The accurate scalar product of the last two arguments added to the value of the first argument
	void accumulate(cdotprecision &dp, const rmatrix_subv & rv1, const rvector &rv2)
#if(CXSC_INDEX_CHECK)
	noexcept(false);
#else
	noexcept(false);
#endif

	//! The accurate scalar product of the last two arguments added to the value of the first argument (without error bound)
	void accumulate_approx(cdotprecision &dp, const rmatrix_subv & rv1, const rvector &rv2);

	//! The accurate scalar product of the last two arguments added to the value of the first argument
	void accumulate(cdotprecision &dp,const rvector_slice &sl,const rvector &rv)
#if(CXSC_INDEX_CHECK)
	noexcept(false);
#else
	noexcept(false);
#endif

	//! The accurate scalar product of the last two arguments added to the value of the first argument (without error bound)
	void accumulate_approx(cdotprecision &dp,const rvector_slice &sl,const rvector &rv);

	//! The accurate scalar product of the last two arguments added to the value of the first argument
	void accumulate(cdotprecision &dp,const rvector &rv,const rvector_slice &sl)
#if(CXSC_INDEX_CHECK)
	noexcept(false);
#else
	noexcept(false);
#endif

	//! The accurate scalar product of the last two arguments added to the value of the first argument (without error bound)
	void accumulate_approx(cdotprecision &dp,const rvector &rv,const rvector_slice &sl);

	//! The accurate scalar product of the last two arguments added to the value of the first argument
	void accumulate(cdotprecision &dp, const rvector_slice & sl1, const rvector_slice &sl2)
#if(CXSC_INDEX_CHECK)
	noexcept(false);
#else
	noexcept(false);
#endif

	//! The accurate scalar product of the last two arguments added to the value of the first argument (without error bound)
	void accumulate_approx(cdotprecision &dp, const rvector_slice & sl1, const rvector_slice &sl2);

	//! The accurate scalar product of the last two arguments added to the value of the first argument
	void accumulate(cidotprecision &dp, const rvector & rv1, const rvector &rv2)
#if(CXSC_INDEX_CHECK)
	noexcept(false);
#else
	noexcept(false);
#endif
	//! The accurate scalar product of the last two arguments added to the value of the first argument
	void accumulate(cidotprecision &dp, const rvector & rv1, const rmatrix_subv &rv2)
#if(CXSC_INDEX_CHECK)
	noexcept(false);
#else
	noexcept(false);
#endif
	//! The accurate scalar product of the last two arguments added to the value of the first argument
	void accumulate(cidotprecision &dp, const rmatrix_subv & rv1, const rvector &rv2)
#if(CXSC_INDEX_CHECK)
	noexcept(false);
#else
	noexcept(false);
#endif
	//! The accurate scalar product of the last two arguments added to the value of the first argument
	void accumulate(cidotprecision &dp,const rvector_slice &sl,const rvector &rv)
#if(CXSC_INDEX_CHECK)
	noexcept(false);
#else
	noexcept(false);
#endif
	//! The accurate scalar product of the last two arguments added to the value of the first argument
	void accumulate(cidotprecision &dp,const rvector &rv,const rvector_slice &sl)
#if(CXSC_INDEX_CHECK)
	noexcept(false);
#else
	noexcept(false);
#endif
	//! The accurate scalar product of the last two arguments added to the value of the first argument
	void accumulate(cidotprecision &dp, const rvector_slice & sl1, const rvector_slice &sl2)
#if(CXSC_INDEX_CHECK)
	noexcept(false);
#else
	noexcept(false);
#endif
	
	//! Implementation of multiplication operation
	INLINE real operator *(const rvector & rv1, const rvector &rv2)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__OP_WITH_WRONG_DIM<rvector>);
#else
	noexcept(false);
#endif
	//! Implementation of multiplication operation
	INLINE real operator *(const rvector_slice &sl, const rvector &rv)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__OP_WITH_WRONG_DIM<rvector>);
#else
	noexcept(false);
#endif
	//! Implementation of multiplication operation
	INLINE real operator *(const rvector &rv, const rvector_slice &sl)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__OP_WITH_WRONG_DIM<rvector>);
#else
	noexcept(false);
#endif
	//! Implementation of multiplication operation
	INLINE real operator *(const rvector_slice & sl1, const rvector_slice &sl2)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__OP_WITH_WRONG_DIM<rvector>);
#else
	noexcept(false);
#endif
	
	//! Implementation of positive sign operation
	INLINE const rvector &operator +(const rvector &rv) noexcept(false);
	//! Implementation of positive sign operation
	INLINE rvector operator +(const rvector_slice &sl) noexcept(false);
	//! Implementation of addition operation
	INLINE rvector operator +(const rvector &rv1, const rvector &rv2)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__OP_WITH_WRONG_DIM<rvector>);
#else
	noexcept(false);
#endif
	//! Implementation of addition operation
	INLINE rvector operator +(const rvector &rv, const rvector_slice &sl)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__OP_WITH_WRONG_DIM<rvector>);
#else
	noexcept(false);
#endif
	//! Implementation of addition operation
	INLINE rvector operator +(const rvector_slice &sl, const rvector &rv)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__OP_WITH_WRONG_DIM<rvector>);
#else
	noexcept(false);
#endif
	//! Implementation of addition operation
	INLINE rvector operator +(const rvector_slice &sl1, const rvector_slice &sl2)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__OP_WITH_WRONG_DIM<rvector>);
#else
	noexcept(false);
#endif
	//! Implementation of addition and allocation operation
	INLINE rvector & operator +=(rvector &rv1, const rvector &rv2)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__OP_WITH_WRONG_DIM<rvector>);
#else
	noexcept(false);
#endif
	//! Implementation of addition and allocation operation
	INLINE rvector &operator +=(rvector &rv, const rvector_slice &sl)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__OP_WITH_WRONG_DIM<rvector>);
#else
	noexcept(false);
#endif

	//! Implementation of negative sign operation
	INLINE rvector operator -(const rvector &rv) noexcept(false);
	//! Implementation of negative sign operation
	INLINE rvector operator -(const rvector_slice &sl) noexcept(false);
	//! Implementation of subtraction operation
	INLINE rvector operator -(const rvector &rv1, const rvector &rv2)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__OP_WITH_WRONG_DIM<rvector>);
#else
	noexcept(false);
#endif
	//! Implementation of subtraction operation
	INLINE rvector operator -(const rvector &rv, const rvector_slice &sl)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__OP_WITH_WRONG_DIM<rvector>);
#else
	noexcept(false);
#endif
	//! Implementation of subtraction operation
	INLINE rvector operator -(const rvector_slice &sl, const rvector &rv)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__OP_WITH_WRONG_DIM<rvector>);
#else
	noexcept(false);
#endif
	//! Implementation of subtraction operation
	INLINE rvector operator -(const rvector_slice &sl1, const rvector_slice &sl2)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__OP_WITH_WRONG_DIM<rvector>);
#else
	noexcept(false);
#endif
	//! Implementation of subtraction and allocation operation
	INLINE rvector & operator -=(rvector &rv1, const rvector &rv2)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__OP_WITH_WRONG_DIM<rvector>);
#else
	noexcept(false);
#endif
	//! Implementation of subtraction and allocation operation
	INLINE rvector &operator -=(rvector &rv, const rvector_slice &sl)
#if(CXSC_INDEX_CHECK)
	throw(ERROR__OP_WITH_WRONG_DIM<rvector>);
#else
	noexcept(false);
#endif

	//! Implementation of standard equality operation
	INLINE bool operator ==(const rvector &rv1, const rvector &rv2) noexcept(false);
	//! Implementation of standard equality operation
	INLINE bool operator ==(const rvector_slice &sl1, const rvector_slice &sl2) noexcept(false);
	//! Implementation of standard equality operation
	INLINE bool operator ==(const rvector_slice &sl, const rvector &rv) noexcept(false);
	//! Implementation of standard equality operation
	INLINE bool operator ==(const rvector &rv, const rvector_slice &sl) noexcept(false);
	//! Implementation of standard negated equality operation
	INLINE bool operator !=(const rvector &rv1, const rvector &rv2) noexcept(false);
	//! Implementation of standard negated equality operation
	INLINE bool operator !=(const rvector_slice &sl1, const rvector_slice &sl2) noexcept(false);
	//! Implementation of standard negated equality operation
	INLINE bool operator !=(const rvector_slice &sl, const rvector &rv) noexcept(false);
	//! Implementation of standard negated equality operation
	INLINE bool operator !=(const rvector &rv, const rvector_slice &sl) noexcept(false);
	//! Implementation of standard less-than operation
	INLINE bool operator <(const rvector &rv1, const rvector &rv2) noexcept(false);
	//! Implementation of standard less-than operation
	INLINE bool operator <(const rvector_slice &sl1, const rvector_slice &sl2) noexcept(false);
	//! Implementation of standard less-than operation
	INLINE bool operator < (const rvector_slice &sl, const rvector &rv) noexcept(false);
	//! Implementation of standard less-than operation
	INLINE bool operator < (const rvector &rv, const rvector_slice &sl) noexcept(false);
	//! Implementation of standard less-or-equal-than operation
	INLINE bool operator <=(const rvector &rv1, const rvector &rv2) noexcept(false);
	//! Implementation of standard less-or-equal-than operation
	INLINE bool operator <=(const rvector_slice &sl1, const rvector_slice &sl2) noexcept(false);
	//! Implementation of standard less-or-equal-than operation
	INLINE bool operator <=(const rvector_slice &sl, const rvector &rv) noexcept(false);
	//! Implementation of standard less-or-equal-than operation
	INLINE bool operator <=(const rvector &rv, const rvector_slice &sl) noexcept(false);
	//! Implementation of standard greater-than operation
	INLINE bool operator >(const rvector &rv1, const rvector &rv2) noexcept(false);
	//! Implementation of standard greater-than operation
	INLINE bool operator >(const rvector_slice &sl1, const rvector_slice &sl2) noexcept(false);
	//! Implementation of standard greater-than operation
	INLINE bool operator >(const rvector_slice &sl, const rvector &rv) noexcept(false);
	//! Implementation of standard greater-than operation
	INLINE bool operator >(const rvector &rv, const rvector_slice &sl) noexcept(false);
	//! Implementation of standard greater-or-equal-than operation
	INLINE bool operator >=(const rvector &rv1, const rvector &rv2) noexcept(false);
	//! Implementation of standard greater-or-equal-than operation
	INLINE bool operator >=(const rvector_slice &sl1, const rvector_slice &sl2) noexcept(false);
	//! Implementation of standard greater-or-equal-than operation
	INLINE bool operator >=(const rvector_slice &sl, const rvector &rv) noexcept(false);
	//! Implementation of standard greater-or-equal-than operation
	INLINE bool operator >=(const rvector &rv, const rvector_slice &sl) noexcept(false);

	//! Implementation of standard output method
	INLINE std::ostream &operator <<(std::ostream &s, const rvector &rv) noexcept(false);
	//! Implementation of standard output method
	INLINE std::ostream &operator <<(std::ostream &o, const rvector_slice &sl) noexcept(false);
	//! Implementation of standard input method
	INLINE std::istream &operator >>(std::istream &s, rvector &rv) noexcept(false);
	//! Implementation of standard input method
	INLINE std::istream &operator >>(std::istream &s, rvector_slice &rv) noexcept(false);


} // namespace cxsc 
	
#ifdef _CXSC_INCL_INL
#include "rvector.inl"
#include "vector.inl"
#endif

#ifdef CXSC_USE_BLAS
#define _CXSC_BLAS_RVECTOR
#include "cxsc_blas.inl"
#endif

#endif
