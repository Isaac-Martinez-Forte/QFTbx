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

/* CVS $Id: matrix_friend_declarations.inl,v 1.10 2014/01/30 17:23:47 cxsc Exp $ */

#if(CXSC_INDEX_CHECK)
template<class TA, class Tx, class Tres, class TDot, class TElement>
friend inline Tres spsl_mv_mult(const TA&, const Tx&) noexcept(false);
#else
template<class TA, class Tx, class Tres, class TDot, class TElement>
friend inline Tres spsl_mv_mult(const TA&, const Tx&) noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class TA, class Tx, class Tres, class TDot, class TElement>
friend inline Tres spsp_mv_mult(const TA&, const Tx&) noexcept(false);
#else
template<class TA, class Tx, class Tres, class TDot, class TElement>
friend inline Tres spsp_mv_mult(const TA&, const Tx&) noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class TA, class Tx, class Tres, class TDot>
friend inline Tres spf_mv_mult(const TA&, const Tx&) noexcept(false);
#else
template<class TA, class Tx, class Tres, class TDot>
friend inline Tres spf_mv_mult(const TA&, const Tx&) noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class TA, class Tx, class Tres, class TDot>
friend inline Tres fsp_mv_mult(const TA&, const Tx&) noexcept(false);
#else
template<class TA, class Tx, class Tres, class TDot>
friend inline Tres fsp_mv_mult(const TA&, const Tx&) noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class TA, class Tx, class Tres, class TDot>
friend inline Tres fsl_mv_mult(const TA&, const Tx&) noexcept(false);
#else
template<class TA, class Tx, class Tres, class TDot>
friend inline Tres fsl_mv_mult(const TA&, const Tx&) noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class TA, class TB, class Tres, class TDot, class TElement>
friend inline Tres spsp_mm_mult(const TA&, const TB&) noexcept(false);
#else
template<class TA, class TB, class Tres, class TDot, class TElement>
friend inline Tres spsp_mm_mult(const TA&, const TB&) noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class TA, class TB, class Tres, class TDot>
friend inline Tres fsp_mm_mult(const TA&, const TB&) noexcept(false);
#else
template<class TA, class TB, class Tres, class TDot>
friend inline Tres fsp_mm_mult(const TA&, const TB&) noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class TA, class TB, class Tres, class TDot>
friend inline Tres spf_mm_mult(const TA&, const TB&) noexcept(false);
#else
template<class TA, class TB, class Tres, class TDot>
friend inline Tres spf_mm_mult(const TA&, const TB&) noexcept(false);
#endif

template<class TA, class Ts, class Tres>
friend inline Tres sp_ms_div(const TA&, const Ts&);

template<class TA, class Ts, class Tres>
friend inline Tres sp_ms_mult(const TA&, const Ts&);

template<class Ts, class TA, class Tres>
friend inline Tres sp_sm_mult(const Ts&, const TA&);

#if(CXSC_INDEX_CHECK)
template<class TA, class TB, class Tres, class TElement>
friend inline Tres spsp_mm_add(const TA&, const TB&) noexcept(false);
#else
template<class TA, class TB, class Tres, class TElement>
friend inline Tres spsp_mm_add(const TA&, const TB&) noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class TA, class TB, class Tres>
friend inline Tres spf_mm_add(const TA&, const TB&) noexcept(false);
#else
template<class TA, class TB, class Tres>
friend inline Tres spf_mm_add(const TA&, const TB&) noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class TA, class TB, class Tres>
friend inline Tres fsp_mm_add(const TA&, const TB&) noexcept(false);
#else
template<class TA, class TB, class Tres>
friend inline Tres fsp_mm_add(const TA&, const TB&) noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class TA, class TB, class Tres, class TElement>
friend inline Tres spsp_mm_sub(const TA&, const TB&) noexcept(false);
#else
template<class TA, class TB, class Tres, class TElement>
friend inline Tres spsp_mm_sub(const TA&, const TB&) noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class TA, class TB, class Tres>
friend inline Tres spf_mm_sub(const TA&, const TB&) noexcept(false);
#else
template<class TA, class TB, class Tres>
friend inline Tres spf_mm_sub(const TA&, const TB&) noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class TA, class TB, class Tres>
friend inline Tres fsp_mm_sub(const TA&, const TB&) noexcept(false);
#else
template<class TA, class TB, class Tres>
friend inline Tres fsp_mm_sub(const TA&, const TB&) noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class TA, class TB, class Tres, class TElement>
friend inline Tres spsp_mm_hull(const TA&, const TB&) noexcept(false);
#else
template<class TA, class TB, class Tres, class TElement>
friend inline Tres spsp_mm_hull(const TA&, const TB&) noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class TA, class TB, class Tres>
friend inline Tres spf_mm_hull(const TA&, const TB&) noexcept(false);
#else
template<class TA, class TB, class Tres>
friend inline Tres spf_mm_hull(const TA&, const TB&) noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class TA, class TB, class Tres>
friend inline Tres fsp_mm_hull(const TA&, const TB&) noexcept(false);
#else
template<class TA, class TB, class Tres>
friend inline Tres fsp_mm_hull(const TA&, const TB&) noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class TA, class TB, class Tres, class TElement>
friend inline Tres spsp_mm_intersect(const TA&, const TB&) noexcept(false);
#else
template<class TA, class TB, class Tres, class TElement>
friend inline Tres spsp_mm_intersect(const TA&, const TB&) noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class TA, class TB, class Tres>
friend inline Tres spf_mm_intersect(const TA&, const TB&) noexcept(false);
#else
template<class TA, class TB, class Tres>
friend inline Tres spf_mm_intersect(const TA&, const TB&) noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class TA, class TB, class Tres>
friend inline Tres fsp_mm_intersect(const TA&, const TB&) noexcept(false);
#else
template<class TA, class TB, class Tres>
friend inline Tres fsp_mm_intersect(const TA&, const TB&) noexcept(false);
#endif

template<class TA, class TB>
friend inline bool spsp_mm_comp(const TA&, const TB&);

template<class TA, class TB>
friend inline bool spf_mm_comp(const TA&, const TB&);

template<class TA, class TB>
friend inline bool fsp_mm_comp(const TA&, const TB&);

template<class TA, class TB, class TType>
friend inline bool spsp_mm_less(const TA&, const TB&);

template<class TA, class TB, class TType>
friend inline bool spf_mm_less(const TA&, const TB&);

template<class TA, class TB, class TType>
friend inline bool fsp_mm_less(const TA&, const TB&);

template<class TA, class TB, class TType>
friend inline bool spsp_mm_leq(const TA&, const TB&);

template<class TA, class TB, class TType>
friend inline bool spf_mm_leq(const TA&, const TB&);

template<class TA, class TB, class TType>
friend inline bool fsp_mm_leq(const TA&, const TB&);

template<class TA, class TB, class TType>
friend inline bool spsp_mm_greater(const TA&, const TB&);

template<class TA, class TB, class TType>
friend inline bool spf_mm_greater(const TA&, const TB&);

template<class TA, class TB, class TType>
friend inline bool fsp_mm_greater(const TA&, const TB&);

template<class TA, class TB, class TType>
friend inline bool spsp_mm_geq(const TA&, const TB&);

template<class TA, class TB, class TType>
friend inline bool spf_mm_geq(const TA&, const TB&);

template<class TA, class TB, class TType>
friend inline bool fsp_mm_geq(const TA&, const TB&);

template<class TA, class Tres>
friend inline Tres sp_m_negative(const TA&);

template<class TA, class TType>
friend inline std::ostream& sp_m_output(std::ostream&, const TA&);

template<class TA, class TType>
friend inline std::istream& sp_m_input(std::istream&, TA&);

#if(CXSC_INDEX_CHECK)
template<class TA, class TB, class TElement>
friend inline TA& slsp_mm_assign(TA&, const TB&) noexcept(false);
#else
template<class TA, class TB, class TElement>
friend inline TA& slsp_mm_assign(TA&, const TB&) noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class TA, class TB, class TElement, class TType>
friend inline TA& slf_mm_assign(TA&, const TB&) noexcept(false);
#else
template<class TA, class TB, class TElement, class TType>
friend inline TA& slf_mm_assign(TA&, const TB&) noexcept(false);
#endif

template<class TA, class TB, class TType>
friend inline TA& spf_mm_assign(TA&, const TB&);

template<class TA, class Ts>
friend inline TA& sp_ms_divassign(TA&, const Ts&);

template<class TA, class Ts>
friend inline TA& sp_ms_multassign(TA&, const Ts&);

#if(CXSC_INDEX_CHECK)
template<class TA, class TB, class TDot, class TElement>
friend inline TA& spsp_mm_multassign(TA&, const TB&) noexcept(false);
#else
template<class TA, class TB, class TDot, class TElement>
friend inline TA& spsp_mm_multassign(TA&, const TB&) noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class TA, class TB, class TDot, class TFull>
friend inline TA& spf_mm_multassign(TA&, const TB&) noexcept(false);
#else
template<class TA, class TB, class TDot, class TFull>
friend inline TA& spf_mm_multassign(TA&, const TB&) noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class TA, class TB, class TDot, class TFull>
friend inline TA& fsp_mm_multassign(TA&, const TB&) noexcept(false);
#else
template<class TA, class TB, class TDot, class TFull>
friend inline TA& fsp_mm_multassign(TA&, const TB&) noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class TA, class TB>
friend inline TA& fsp_mm_addassign(TA& A, const TB& B) noexcept(false);
#else
template<class TA, class TB>
friend inline TA& fsp_mm_addassign(TA& A, const TB& B) noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class TA, class TB, class TFull>
friend inline TA& spf_mm_addassign(TA&, const TB&) noexcept(false);
#else
template<class TA, class TB, class TFull>
friend inline TA& spf_mm_addassign(TA&, const TB&) noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class TA, class TB, class TElement>
friend inline TA& spsp_mm_addassign(TA&, const TB&) noexcept(false);
#else
template<class TA, class TB, class TElement>
friend inline TA& spsp_mm_addassign(TA&, const TB&) noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class TA, class TB>
friend inline TA& spsp_mm_addassign(TA&, const TB&) noexcept(false);
#else
template<class TA, class TB>
friend inline TA& spsp_mm_addassign(TA&, const TB&) noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class TA, class TB>
friend inline TA& fsp_mm_subassign(TA& A, const TB& B) noexcept(false);
#else
template<class TA, class TB>
friend inline TA& fsp_mm_subassign(TA& A, const TB& B) noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class TA, class TB, class TFull>
friend inline TA& spf_mm_subassign(TA&, const TB&) noexcept(false);
#else
template<class TA, class TB, class TFull>
friend inline TA& spf_mm_subassign(TA&, const TB&) noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class TA, class TB, class TElement>
friend inline TA& spsp_mm_subassign(TA&, const TB&) noexcept(false);
#else
template<class TA, class TB, class TElement>
friend inline TA& spsp_mm_subassign(TA&, const TB&) noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class TA, class TB>
friend inline TA& spsp_mm_subassign(TA&, const TB&) noexcept(false);
#else
template<class TA, class TB>
friend inline TA& spsp_mm_subassign(TA&, const TB&) noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class TA, class TB, class TFull>
friend inline TA& spf_mm_hullassign(TA&, const TB&) noexcept(false);
#else
template<class TA, class TB, class TFull>
friend inline TA& spf_mm_hullassign(TA&, const TB&) noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class TA, class TB>
friend inline TA& fsp_mm_hullassign(TA& A, const TB& B)  noexcept(false);
#else
template<class TA, class TB>
friend inline TA& fsp_mm_hullassign(TA& A, const TB& B)  noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class TA, class TB, class TElement>
friend inline TA& spsp_mm_hullassign(TA&, const TB&) noexcept(false);
#else
template<class TA, class TB, class TElement>
friend inline TA& spsp_mm_hullassign(TA&, const TB&) noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class TA, class TB>
friend inline TA& spsp_mm_hullassign(TA&, const TB&) noexcept(false);
#else
template<class TA, class TB>
friend inline TA& spsp_mm_hullassign(TA&, const TB&) noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class TA, class TB>
friend inline TA& fsp_mm_intersectassign(TA& A, const TB& B)  noexcept(false);
#else
template<class TA, class TB>
friend inline TA& fsp_mm_intersectassign(TA& A, const TB& B)  noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class TA, class TB, class TFull>
friend inline TA& spf_mm_intersectassign(TA&, const TB&) noexcept(false);
#else
template<class TA, class TB, class TFull>
friend inline TA& spf_mm_intersectassign(TA&, const TB&) noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class TA, class TB, class TElement>
friend inline TA& spsp_mm_intersectassign(TA&, const TB&) noexcept(false);
#else
template<class TA, class TB, class TElement>
friend inline TA& spsp_mm_intersectassign(TA&, const TB&) noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class TA, class TB>
friend inline TA& spsp_mm_intersectassign(TA&, const TB&) noexcept(false);
#else
template<class TA, class TB>
friend inline TA& spsp_mm_intersectassign(TA&, const TB&) noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class Tx, class Ty>
friend inline Tx& svsp_vv_assign(Tx&, const Ty&) noexcept(false);
#else
template<class Tx, class Ty>
friend inline Tx& svsp_vv_assign(Tx&, const Ty&) noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class Tx, class Ty>
friend inline Tx& svsl_vv_assign(Tx&, const Ty&) noexcept(false);
#else
template<class Tx, class Ty>
friend inline Tx& svsl_vv_assign(Tx&, const Ty&) noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class Tx, class Ty>
friend inline Tx& svf_vv_assign(Tx&, const Ty&) noexcept(false);
#else
template<class Tx, class Ty>
friend inline Tx& svf_vv_assign(Tx&, const Ty&) noexcept(false);
#endif

template<class TA, class Ts, class TType>
friend inline TA& sp_ms_assign(TA&, const Ts&);

template<class TA, class Ts, class TElement, class TType>
friend inline TA& sl_ms_assign(TA&, const Ts&);

template<class Tx, class Ts>
friend inline Tx& sv_vs_assign(Tx&, const Ts&);

template<class TA>
friend inline bool sp_m_not(const TA&);

template<class Tx>
friend inline bool sv_v_not(const Tx&);

template <class TA>
friend inline void sp_m_resize(TA& A) noexcept(false);

#if(CXSC_INDEX_CHECK)
template <class TA>
friend inline void sp_m_resize(TA &A,const int &m, const int &n) noexcept(false);
#else
template <class TA>
friend inline void sp_m_resize(TA &A,const int &m, const int &n) noexcept(false);
#endif

#if(CXSC_INDEX_CHECK)
template<class TA>
friend inline void sp_m_resize(TA &A,const int &m1, const int &m2,const int &n1,const int &n2) noexcept(false);
#else
template<class TA>
friend inline void sp_m_resize(TA &A,const int &m1, const int &m2,const int &n1,const int &n2) noexcept(false);
#endif