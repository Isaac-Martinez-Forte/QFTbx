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

/* CVS $Id: except.hpp,v 1.31 2014/01/30 17:23:45 cxsc Exp $ */

#ifndef _CXSC_EXCEPT_HPP_INCLUDED
#define _CXSC_EXCEPT_HPP_INCLUDED

#include <string>
#include <iostream>
#include <xscclass.hpp>

namespace cxsc {

/*
class cxsc_status {                    //declaration of class cxsc_status 
 private:
   cxsc_status();                      //constructor is private
   static cxsc_status* cxsc_statusRef; //declaration of static attribut

 public:
   static cxsc_status* getObjectReference(); //decl. of static member function
};

#ifndef NO_CXSC_STATUS
cxsc_status* __p_cxsc_status(cxsc_status::getObjectReference());
#endif
*/



//----------------------  Error Modules --------------------------------------

class ERROR_ALL
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR_ALL() noexcept(false);
		ERROR_ALL(const string &f) noexcept(false);
		virtual ~ERROR_ALL() noexcept(false);
	protected:
		string fkt;
};

class ERROR_DOT : virtual public ERROR_ALL
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR_DOT() noexcept(false);
		ERROR_DOT(const string &f) noexcept(false);
//		virtual ~ERROR_DOT() noexcept(false);
//	private:
//		string fkt;
};

class ERROR_REAL : virtual public ERROR_ALL
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR_REAL() noexcept(false);
		ERROR_REAL(const string &f) noexcept(false);
//		virtual ~ERROR_REAL() noexcept(false);
//	private:
//		string fkt;
};

class ERROR_INTERVAL : virtual public ERROR_REAL
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR_INTERVAL() noexcept(false);
		ERROR_INTERVAL(const string &f) noexcept(false);
//		virtual ~ERROR_INTERVAL() noexcept(false);
//	private:
//		string fkt;
};

class ERROR_COMPLEX : virtual public ERROR_ALL
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR_COMPLEX() noexcept(false);
		ERROR_COMPLEX(const string &f) noexcept(false);
//		virtual ~ERROR_COMPLEX() noexcept(false);
//	private:
//		string fkt;
};

class ERROR_CINTERVAL : virtual public ERROR_COMPLEX, virtual public ERROR_INTERVAL
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR_CINTERVAL() noexcept(false);
		ERROR_CINTERVAL(const string &f) noexcept(false);
//		virtual ~ERROR_CINTERVAL() noexcept(false) { }
//	private:
//		string fkt;
};

class ERROR_VECTOR : virtual public ERROR_ALL
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR_VECTOR() noexcept(false);
		ERROR_VECTOR(const string &f) noexcept(false);
//		virtual ~ERROR_VECTOR() noexcept(false) { }
//	private:
//		string fkt;
};

class ERROR_RVECTOR : virtual public ERROR_VECTOR, virtual public ERROR_REAL
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR_RVECTOR() noexcept(false);
		ERROR_RVECTOR(const string &f) noexcept(false);
//		virtual ~ERROR_RVECTOR() noexcept(false) { }
//	private:
//		string fkt;
};

class ERROR_IVECTOR : virtual public ERROR_VECTOR, virtual public ERROR_INTERVAL
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR_IVECTOR() noexcept(false);
		ERROR_IVECTOR(const string &f) noexcept(false);
//		virtual ~ERROR_IVECTOR() noexcept(false) { }
//	private:
//		string fkt;
};

class ERROR_CVECTOR : virtual public ERROR_VECTOR, virtual public ERROR_COMPLEX
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR_CVECTOR() noexcept(false);
		ERROR_CVECTOR(const string &f) noexcept(false);
//		virtual ~ERROR_CVECTOR() noexcept(false) { }
//	private:
//		string fkt;
};

class ERROR_CIVECTOR : virtual public ERROR_VECTOR, virtual public ERROR_CINTERVAL
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR_CIVECTOR() noexcept(false);
		ERROR_CIVECTOR(const string &f) noexcept(false);
//		virtual ~ERROR_CIVECTOR() noexcept(false) { }
//	private:
//		string fkt;
};

class ERROR_MATRIX : virtual public ERROR_ALL
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR_MATRIX() noexcept(false);
		ERROR_MATRIX(const string &f) noexcept(false);
//		virtual ~ERROR_MATRIX() noexcept(false) { }
//	private:
//		string fkt;
};

class ERROR_RMATRIX : virtual public ERROR_MATRIX, virtual public ERROR_REAL
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR_RMATRIX() noexcept(false);
		ERROR_RMATRIX(const string &f) noexcept(false);
//		virtual ~ERROR_RMATRIX() noexcept(false) { }
//	private:
//		string fkt;
};


class ERROR_IMATRIX : virtual public ERROR_MATRIX, virtual public ERROR_INTERVAL
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR_IMATRIX() noexcept(false);
		ERROR_IMATRIX(const string &f) noexcept(false);
//		virtual ~ERROR_IMATRIX() noexcept(false) { }
//	private:
//		string fkt;
};


class ERROR_CMATRIX : virtual public ERROR_MATRIX, virtual public ERROR_COMPLEX
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR_CMATRIX() noexcept(false);
		ERROR_CMATRIX(const string &f) noexcept(false);
//		virtual ~ERROR_CMATRIX() noexcept(false) { }
//	private:
//		string fkt;
};


class ERROR_CIMATRIX : virtual public ERROR_MATRIX, virtual public ERROR_CINTERVAL
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR_CIMATRIX() noexcept(false);
		ERROR_CIMATRIX(const string &f) noexcept(false);
//		virtual ~ERROR_CIMATRIX() noexcept(false) { }
//	private:
//		string fkt;
};


class ERROR_LREAL : virtual public ERROR_ALL
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR_LREAL() noexcept(false);
		ERROR_LREAL(const string &f) noexcept(false);
//		virtual ~ERROR_LREAL() noexcept(false) { }
//	private:
//		string fkt;
};


class ERROR_LINTERVAL : virtual public ERROR_LREAL, virtual public ERROR_INTERVAL
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR_LINTERVAL() noexcept(false);
		ERROR_LINTERVAL(const string &f) noexcept(false);
//		virtual ~ERROR_LINTERVAL() noexcept(false) { }
//	private:
//		string fkt;
};


class ERROR_LRVECTOR : virtual public ERROR_LREAL, virtual public ERROR_VECTOR
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR_LRVECTOR() noexcept(false);
		ERROR_LRVECTOR(const string &f) noexcept(false);
//		virtual ~ERROR_LRVECTOR() noexcept(false) { }
//	private:
//		string fkt;
};


class ERROR_LIVECTOR : virtual public ERROR_LINTERVAL, virtual public ERROR_VECTOR
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR_LIVECTOR() noexcept(false);
		ERROR_LIVECTOR(const string &f) noexcept(false);
//		virtual ~ERROR_LIVECTOR() noexcept(false) { }
//	private:
//		string fkt;
};


class ERROR_LRMATRIX : virtual public ERROR_LREAL, virtual public ERROR_MATRIX
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR_LRMATRIX() noexcept(false);
		ERROR_LRMATRIX(const string &f) noexcept(false);
//		virtual ~ERROR_LRMATRIX() noexcept(false) { }
//	private:
//		string fkt;
};


class ERROR_LIMATRIX : virtual public ERROR_LINTERVAL, virtual public ERROR_MATRIX
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR_LIMATRIX() noexcept(false);
		ERROR_LIMATRIX(const string &f) noexcept(false);
//		virtual ~ERROR_LIMATRIX() noexcept(false) { }
//	private:
//		string fkt;
};


class ERROR_INTVECTOR : virtual public ERROR_VECTOR
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR_INTVECTOR() noexcept(false);
		ERROR_INTVECTOR(const string &f) noexcept(false);
//		virtual ~ERROR_INTVECTOR() noexcept(false) { }
//	private:
//		string fkt;
};

class ERROR_INTMATRIX : virtual public ERROR_MATRIX
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR_INTMATRIX() noexcept(false);
		ERROR_INTMATRIX(const string &f) noexcept(false);
//		virtual ~ERROR_INTMATRIX() noexcept(false) { }
//	private:
//		string fkt;
};

class ERROR_TAYLOR : virtual public ERROR_ALL
{ // Blomquist 22.12.2008;
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR_TAYLOR() noexcept(false);
		ERROR_TAYLOR(const string &f) noexcept(false);
//		virtual ~ERROR_TAYLOR() noexcept(false) { }
//	private:
//		string fkt;
};

//-------------------------- Error Types --------------------------------


class WRONG_ROUNDING: virtual public ERROR_ALL
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		WRONG_ROUNDING() noexcept(false);
		WRONG_ROUNDING(const string &f) noexcept(false);
//		virtual ~WRONG_ROUNDING() noexcept(false) { }
//	private:
//		string fkt;
};

class NO_MORE_MEMORY: virtual public ERROR_ALL
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		NO_MORE_MEMORY() noexcept(false);
		NO_MORE_MEMORY(const string &f) noexcept(false);
//		virtual ~NO_MORE_MEMORY() noexcept(false) { }
//	private:
//		string fkt;
};

class WRONG_DOT_TYPE: virtual public ERROR_ALL
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		WRONG_DOT_TYPE() noexcept(false);
		WRONG_DOT_TYPE(const string &f) noexcept(false);
//		virtual ~WRONG_DOT_TYPE() noexcept(false) { }
//	private:
//		string fkt;
};

class NOT_AVAILABLE: virtual public ERROR_ALL
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		NOT_AVAILABLE() noexcept(false);
		NOT_AVAILABLE(const string &f) noexcept(false);
//		virtual ~NOT_AVAILABLE() noexcept(false) { }
//	private:
//		string fkt;
};

class DIV_BY_ZERO: virtual public ERROR_ALL
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		DIV_BY_ZERO() noexcept(false);
		DIV_BY_ZERO(const string &f) noexcept(false);
//		virtual ~DIV_BY_ZERO() noexcept(false) { }
//	private:
//		string fkt;
};

class EMPTY_INTERVAL: virtual public ERROR_ALL
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		EMPTY_INTERVAL() noexcept(false);
		EMPTY_INTERVAL(const string &f) noexcept(false);
//		virtual ~EMPTY_INTERVAL() noexcept(false) { }
//	private:
//		string fkt;
};

class OVERFLOW_ERROR: virtual public ERROR_ALL
{
   public:
      virtual int errnum() const noexcept(false);
      virtual string errtext() const noexcept(false);
      OVERFLOW_ERROR() noexcept(false);
      OVERFLOW_ERROR(const string &f) noexcept(false);
//      virtual ~OVERFLOW_ERROR() noexcept(false);
};

class IN_EXACT_CH_OR_IS: virtual public ERROR_ALL
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		IN_EXACT_CH_OR_IS() noexcept(false);
		IN_EXACT_CH_OR_IS(const string &f) noexcept(false);
//		virtual ~IN_EXACT_CH_OR_IS() noexcept(false) { }
//	private:
//		string fkt;
};

class WRONG_BOUNDARIES: virtual public ERROR_ALL
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		WRONG_BOUNDARIES() noexcept(false);
		WRONG_BOUNDARIES(const string &f) noexcept(false);
//		virtual ~WRONG_BOUNDARIES() noexcept(false) { }
//	private:
//		string fkt;
};


class SUB_ARRAY_TOO_BIG: virtual public ERROR_ALL
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		SUB_ARRAY_TOO_BIG() noexcept(false);
		SUB_ARRAY_TOO_BIG(const string &f) noexcept(false);
//		virtual ~SUB_ARRAY_TOO_BIG() noexcept(false) { }
//	private:
//		string fkt;
};


class RES_OR_INP_OF_TEMP_OBJ: virtual public ERROR_ALL
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		RES_OR_INP_OF_TEMP_OBJ() noexcept(false);
		RES_OR_INP_OF_TEMP_OBJ(const string &f) noexcept(false);
//		virtual ~RES_OR_INP_OF_TEMP_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

class ELEMENT_NOT_IN_VEC: virtual public ERROR_ALL
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ELEMENT_NOT_IN_VEC() noexcept(false);
		ELEMENT_NOT_IN_VEC(const string &f) noexcept(false);
//		virtual ~ELEMENT_NOT_IN_VEC() noexcept(false) { }
//	private:
//		string fkt;
};

class ROW_OR_COL_NOT_IN_MAT: virtual public ERROR_ALL
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ROW_OR_COL_NOT_IN_MAT() noexcept(false);
		ROW_OR_COL_NOT_IN_MAT(const string &f) noexcept(false);
//		virtual ~ROW_OR_COL_NOT_IN_MAT() noexcept(false) { }
//	private:
//		string fkt;
};

class WRONG_ROW_OR_COL : virtual public ERROR_MATRIX
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		WRONG_ROW_OR_COL() noexcept(false);
		WRONG_ROW_OR_COL(const string &f) noexcept(false);
//		virtual ~WRONG_ROW_OR_COL() noexcept(false) { }
//	private:
//		string fkt;
};

class TYPE_CAST_OF_THICK_OBJ: virtual public ERROR_ALL
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		TYPE_CAST_OF_THICK_OBJ() noexcept(false);
		TYPE_CAST_OF_THICK_OBJ(const string &f) noexcept(false);
//		virtual ~TYPE_CAST_OF_THICK_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

class OP_WITH_WRONG_DIM: virtual public ERROR_ALL
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		OP_WITH_WRONG_DIM() noexcept(false);
		OP_WITH_WRONG_DIM(const string &f) noexcept(false);
//		virtual ~OP_WITH_WRONG_DIM() noexcept(false) { }
//	private:
//		string fkt;
};

class WRONG_STAGPREC: virtual public ERROR_ALL
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		WRONG_STAGPREC() noexcept(false);
		WRONG_STAGPREC(const string &f) noexcept(false);
//		virtual ~WRONG_STAGPREC() noexcept(false) { }
//	private:
//		string fkt;
};


class ELEMENT_NOT_IN_LONG: virtual public ERROR_ALL
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ELEMENT_NOT_IN_LONG() noexcept(false);
		ELEMENT_NOT_IN_LONG(const string &f) noexcept(false);
//		virtual ~ELEMENT_NOT_IN_LONG() noexcept(false) {!././compcxsc  }
//	private:
//		string fkt;
};

class STD_FKT_OUT_OF_DEF: virtual public ERROR_ALL
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		STD_FKT_OUT_OF_DEF() noexcept(false);
		STD_FKT_OUT_OF_DEF(const string &f) noexcept(false);
//		virtual ~STD_FKT_OUT_OF_DEF() noexcept(false) { }
//	private:
//		string fkt;
};

class FAK_OVERFLOW: virtual public OVERFLOW_ERROR
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		FAK_OVERFLOW() noexcept(false);
		FAK_OVERFLOW(const string &f) noexcept(false);
//		virtual ~FAK_OVERFLOW() noexcept(false) { }
//	private:
//		string fkt;
};

class USE_OF_UNINITIALIZED_OBJ: virtual public ERROR_ALL
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		USE_OF_UNINITIALIZED_OBJ() noexcept(false);
		USE_OF_UNINITIALIZED_OBJ(const string &f) noexcept(false);
//		virtual ~USE_OF_UNINITIALIZED_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

//class CONTINUE_NOT_POSSIBLE
//{
//	public:
//		virtual int errnum() const noexcept(false);
//		virtual string errtext() const noexcept(false);
//		CONTINUE_NOT_POSSIBLE() noexcept(false);
//// virtual ~}() noexcept(false) { }
//		CONTINUE_NOT_POSSIBLE(const string &f) noexcept(false):fkt(f) { //}private:
//	string fkt;//
//};


//------------- Occuring Errors --------------------------


template <class T>
class ERROR__ELEMENT_NOT_IN_VEC: virtual public ELEMENT_NOT_IN_VEC
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__ELEMENT_NOT_IN_VEC() noexcept(false);
		ERROR__ELEMENT_NOT_IN_VEC(const string &f) noexcept(false);
//		virtual ~ERROR__ELEMENT_NOT_IN_VEC() noexcept(false) { }
//	private:
//		string fkt;
};

template <class T>
class ERROR__TYPE_CAST_OF_THICK_OBJ: virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false);
		ERROR__TYPE_CAST_OF_THICK_OBJ(const string &f) noexcept(false);
//		virtual ~ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <class T>
class ERROR__USE_OF_UNINITIALIZED_OBJ: virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false);
		ERROR__USE_OF_UNINITIALIZED_OBJ(const string &f) noexcept(false);
//		virtual ~ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <class T>
class ERROR__OP_WITH_WRONG_DIM: virtual public OP_WITH_WRONG_DIM
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__OP_WITH_WRONG_DIM() noexcept(false);
		ERROR__OP_WITH_WRONG_DIM(const string &f) noexcept(false);
//		virtual ~ERROR__OP_WITH_WRONG_DIM() noexcept(false) { }
//	private:
//		string fkt;
};

template <class T>
class ERROR__WRONG_BOUNDARIES: virtual public WRONG_BOUNDARIES
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__WRONG_BOUNDARIES() noexcept(false);
		ERROR__WRONG_BOUNDARIES(const string &f) noexcept(false);
//		virtual ~ERROR__WRONG_BOUNDARIES() noexcept(false) { }
//	private:
//		string fkt;
};

template <class T>
class ERROR__NO_MORE_MEMORY: virtual public NO_MORE_MEMORY
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__NO_MORE_MEMORY() noexcept(false);
		ERROR__NO_MORE_MEMORY(const string &f) noexcept(false);
//		virtual ~ERROR__NO_MORE_MEMORY() noexcept(false) { }
//	private:
//		string fkt;
};

template <class T>
class ERROR__SUB_ARRAY_TOO_BIG: virtual public SUB_ARRAY_TOO_BIG
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__SUB_ARRAY_TOO_BIG() noexcept(false);
		ERROR__SUB_ARRAY_TOO_BIG(const string &f) noexcept(false);
//		virtual ~ERROR__SUB_ARRAY_TOO_BIG() noexcept(false) { }
//	private:
//		string fkt;
};

template <class T>
class ERROR__ROW_OR_COL_NOT_IN_MAT: virtual public ROW_OR_COL_NOT_IN_MAT
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false);
		ERROR__ROW_OR_COL_NOT_IN_MAT(const string &f) noexcept(false);
//		virtual ~ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false) { }
//	private:
//		string fkt;
};

template <class T>
class ERROR__WRONG_ROW_OR_COL: virtual public WRONG_ROW_OR_COL
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__WRONG_ROW_OR_COL<T>() noexcept(false);
		ERROR__WRONG_ROW_OR_COL<T>(const string &f) noexcept(false);
//		virtual ~ERROR__WRONG_ROW_OR_COL() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_BOUNDARIES<rvector>: virtual public ERROR_RVECTOR, virtual public WRONG_BOUNDARIES
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__WRONG_BOUNDARIES() noexcept(false);
		ERROR__WRONG_BOUNDARIES(const string &f) noexcept(false);
//		virtual ~ERROR_RVECTOR_WRONG_BOUNDARIES() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__NO_MORE_MEMORY<rvector>: virtual public ERROR_RVECTOR, virtual public NO_MORE_MEMORY
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__NO_MORE_MEMORY() noexcept(false);
		ERROR__NO_MORE_MEMORY(const string &f) noexcept(false);
//		virtual ~ERROR__NO_MORE_MEMORY() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__SUB_ARRAY_TOO_BIG<rvector>: virtual public ERROR_RVECTOR, virtual public SUB_ARRAY_TOO_BIG
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__SUB_ARRAY_TOO_BIG() noexcept(false);
		ERROR__SUB_ARRAY_TOO_BIG(const string &f) noexcept(false);
//		virtual ~ERROR__SUB_ARRAY_TOO_BIG() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__ELEMENT_NOT_IN_VEC<rvector>: virtual public ERROR_RVECTOR, virtual public ELEMENT_NOT_IN_VEC
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__ELEMENT_NOT_IN_VEC() noexcept(false);
		ERROR__ELEMENT_NOT_IN_VEC(const string &f) noexcept(false);
//		virtual ~ERROR__ELEMENT_NOT_IN_VEC() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__TYPE_CAST_OF_THICK_OBJ<rvector>: virtual public ERROR_RVECTOR, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false);
		ERROR__TYPE_CAST_OF_THICK_OBJ(const string &f) noexcept(false);
//		virtual ~ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__TYPE_CAST_OF_THICK_OBJ<rmatrix>: virtual public ERROR_RMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false);
		ERROR__TYPE_CAST_OF_THICK_OBJ(const string &f) noexcept(false);
//		virtual ~ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__USE_OF_UNINITIALIZED_OBJ<rvector>: virtual public ERROR_RVECTOR, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false);
		ERROR__USE_OF_UNINITIALIZED_OBJ(const string &f) noexcept(false);
//		virtual ~ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__USE_OF_UNINITIALIZED_OBJ<rmatrix>: virtual public ERROR_RMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false);
		ERROR__USE_OF_UNINITIALIZED_OBJ(const string &f) noexcept(false);
//		virtual ~ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__OP_WITH_WRONG_DIM<rvector>: virtual public ERROR_RVECTOR, virtual public OP_WITH_WRONG_DIM
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__OP_WITH_WRONG_DIM() noexcept(false);
		ERROR__OP_WITH_WRONG_DIM(const string &f) noexcept(false);
//		virtual ~ERROR__OP_WITH_WRONG_DIM() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__OP_WITH_WRONG_DIM<rmatrix>: virtual public ERROR_RMATRIX, virtual public OP_WITH_WRONG_DIM
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__OP_WITH_WRONG_DIM() noexcept(false);
		ERROR__OP_WITH_WRONG_DIM(const string &f) noexcept(false);
//		virtual ~ERROR__OP_WITH_WRONG_DIM() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_BOUNDARIES<rmatrix>: virtual public ERROR_RMATRIX, virtual public WRONG_BOUNDARIES
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__WRONG_BOUNDARIES() noexcept(false);
		ERROR__WRONG_BOUNDARIES(const string &f) noexcept(false);
//		virtual ~ERROR__WRONG_BOUNDARIES() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__NO_MORE_MEMORY<rmatrix>: virtual public ERROR_RMATRIX, virtual public NO_MORE_MEMORY
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__NO_MORE_MEMORY() noexcept(false);
		ERROR__NO_MORE_MEMORY(const string &f) noexcept(false);
//		virtual ~ERROR__NO_MORE_MEMORY() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__SUB_ARRAY_TOO_BIG<rmatrix>: virtual public ERROR_RMATRIX, virtual public SUB_ARRAY_TOO_BIG
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__SUB_ARRAY_TOO_BIG() noexcept(false);
		ERROR__SUB_ARRAY_TOO_BIG(const string &f) noexcept(false);
//		virtual ~ERROR__SUB_ARRAY_TOO_BIG() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__ROW_OR_COL_NOT_IN_MAT<rmatrix>: virtual public ERROR_RMATRIX, virtual public ROW_OR_COL_NOT_IN_MAT
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false);
		ERROR__ROW_OR_COL_NOT_IN_MAT(const string &f) noexcept(false);
//		virtual ~ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_ROW_OR_COL<rmatrix>: virtual public ERROR_RMATRIX, virtual public WRONG_ROW_OR_COL
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__WRONG_ROW_OR_COL() noexcept(false);
		ERROR__WRONG_ROW_OR_COL(const string &f) noexcept(false);
//		virtual ~ERROR__WRONG_ROW_OR_COL() noexcept(false) { }
//	private:
//		string fkt;
};


template <>
class ERROR__WRONG_BOUNDARIES<ivector>: virtual public ERROR_IVECTOR, virtual public WRONG_BOUNDARIES
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__WRONG_BOUNDARIES() noexcept(false);
		ERROR__WRONG_BOUNDARIES(const string &f) noexcept(false);
//		virtual ~ERROR__WRONG_BOUNDARIES() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__NO_MORE_MEMORY<ivector>: virtual public ERROR_IVECTOR, virtual public NO_MORE_MEMORY
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__NO_MORE_MEMORY() noexcept(false);
		ERROR__NO_MORE_MEMORY(const string &f) noexcept(false);
//		virtual ~ERROR__NO_MORE_MEMORY() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__SUB_ARRAY_TOO_BIG<ivector>: virtual public ERROR_IVECTOR, virtual public SUB_ARRAY_TOO_BIG
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__SUB_ARRAY_TOO_BIG() noexcept(false);
		ERROR__SUB_ARRAY_TOO_BIG(const string &f) noexcept(false);
//		virtual ~ERROR__SUB_ARRAY_TOO_BIG() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__ELEMENT_NOT_IN_VEC<ivector>: virtual public ERROR_IVECTOR, virtual public ELEMENT_NOT_IN_VEC
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__ELEMENT_NOT_IN_VEC() noexcept(false);
		ERROR__ELEMENT_NOT_IN_VEC(const string &f) noexcept(false);
//		virtual ~ERROR__ELEMENT_NOT_IN_VEC() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__TYPE_CAST_OF_THICK_OBJ<ivector>: virtual public ERROR_IVECTOR, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false);
		ERROR__TYPE_CAST_OF_THICK_OBJ(const string &f) noexcept(false);
//		virtual ~ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__TYPE_CAST_OF_THICK_OBJ<imatrix>: virtual public ERROR_IMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false);
		ERROR__TYPE_CAST_OF_THICK_OBJ(const string &f) noexcept(false);
//		virtual ~ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__USE_OF_UNINITIALIZED_OBJ<ivector>: virtual public ERROR_IVECTOR, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false);
		ERROR__USE_OF_UNINITIALIZED_OBJ(const string &f) noexcept(false);
//		virtual ~ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__USE_OF_UNINITIALIZED_OBJ<imatrix>: virtual public ERROR_IMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false);
		ERROR__USE_OF_UNINITIALIZED_OBJ(const string &f) noexcept(false);
//		virtual ~ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__OP_WITH_WRONG_DIM<ivector>: virtual public ERROR_IVECTOR, virtual public OP_WITH_WRONG_DIM
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__OP_WITH_WRONG_DIM() noexcept(false);
		ERROR__OP_WITH_WRONG_DIM(const string &f) noexcept(false);
//		virtual ~ERROR__OP_WITH_WRONG_DIM() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__OP_WITH_WRONG_DIM<imatrix>: virtual public ERROR_IMATRIX, virtual public OP_WITH_WRONG_DIM
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__OP_WITH_WRONG_DIM() noexcept(false);
		ERROR__OP_WITH_WRONG_DIM(const string &f) noexcept(false);
//		virtual ~ERROR__OP_WITH_WRONG_DIM() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_BOUNDARIES<imatrix>: virtual public ERROR_IMATRIX, virtual public WRONG_BOUNDARIES
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__WRONG_BOUNDARIES() noexcept(false);
		ERROR__WRONG_BOUNDARIES(const string &f) noexcept(false);
//		virtual ~ERROR__WRONG_BOUNDARIES() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__NO_MORE_MEMORY<imatrix>: virtual public ERROR_IMATRIX, virtual public NO_MORE_MEMORY
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__NO_MORE_MEMORY() noexcept(false);
		ERROR__NO_MORE_MEMORY(const string &f) noexcept(false);
//		virtual ~ERROR__NO_MORE_MEMORY() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__SUB_ARRAY_TOO_BIG<imatrix>: virtual public ERROR_IMATRIX, virtual public SUB_ARRAY_TOO_BIG
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__SUB_ARRAY_TOO_BIG() noexcept(false);
		ERROR__SUB_ARRAY_TOO_BIG(const string &f) noexcept(false);
//		virtual ~ERROR__SUB_ARRAY_TOO_BIG() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__ROW_OR_COL_NOT_IN_MAT<imatrix>: virtual public ERROR_IMATRIX, virtual public ROW_OR_COL_NOT_IN_MAT
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false);
		ERROR__ROW_OR_COL_NOT_IN_MAT(const string &f) noexcept(false);
//		virtual ~ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_ROW_OR_COL<imatrix>: virtual public ERROR_IMATRIX, virtual public WRONG_ROW_OR_COL
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__WRONG_ROW_OR_COL() noexcept(false);
		ERROR__WRONG_ROW_OR_COL(const string &f) noexcept(false);
//		virtual ~ERROR__WRONG_ROW_OR_COL() noexcept(false) { }
//	private:
//		string fkt;
};


template <>
class ERROR__WRONG_BOUNDARIES<rvector_slice>: virtual public ERROR_RVECTOR, virtual public WRONG_BOUNDARIES
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__WRONG_BOUNDARIES() noexcept(false);
		ERROR__WRONG_BOUNDARIES(const string &f) noexcept(false);
//		virtual ~ERROR_RVECTOR_WRONG_BOUNDARIES() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__NO_MORE_MEMORY<rvector_slice>: virtual public ERROR_RVECTOR, virtual public NO_MORE_MEMORY
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__NO_MORE_MEMORY() noexcept(false);
		ERROR__NO_MORE_MEMORY(const string &f) noexcept(false);
//		virtual ~ERROR__NO_MORE_MEMORY() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__SUB_ARRAY_TOO_BIG<rvector_slice>: virtual public ERROR_RVECTOR, virtual public SUB_ARRAY_TOO_BIG
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__SUB_ARRAY_TOO_BIG() noexcept(false);
		ERROR__SUB_ARRAY_TOO_BIG(const string &f) noexcept(false);
//		virtual ~ERROR__SUB_ARRAY_TOO_BIG() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__ELEMENT_NOT_IN_VEC<rvector_slice>: virtual public ERROR_RVECTOR, virtual public ELEMENT_NOT_IN_VEC
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__ELEMENT_NOT_IN_VEC() noexcept(false);
		ERROR__ELEMENT_NOT_IN_VEC(const string &f) noexcept(false);
//		virtual ~ERROR__ELEMENT_NOT_IN_VEC() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__TYPE_CAST_OF_THICK_OBJ<rvector_slice>: virtual public ERROR_RVECTOR, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false);
		ERROR__TYPE_CAST_OF_THICK_OBJ(const string &f) noexcept(false);
//		virtual ~ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__TYPE_CAST_OF_THICK_OBJ<rmatrix_slice>: virtual public ERROR_RMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false);
		ERROR__TYPE_CAST_OF_THICK_OBJ(const string &f) noexcept(false);
//		virtual ~ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__USE_OF_UNINITIALIZED_OBJ<rvector_slice>: virtual public ERROR_RVECTOR, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false);
		ERROR__USE_OF_UNINITIALIZED_OBJ(const string &f) noexcept(false);
//		virtual ~ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__USE_OF_UNINITIALIZED_OBJ<rmatrix_slice>: virtual public ERROR_RMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false);
		ERROR__USE_OF_UNINITIALIZED_OBJ(const string &f) noexcept(false);
//		virtual ~ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__OP_WITH_WRONG_DIM<rvector_slice>: virtual public ERROR_RVECTOR, virtual public OP_WITH_WRONG_DIM
{
	public:
                virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__OP_WITH_WRONG_DIM() noexcept(false);
		ERROR__OP_WITH_WRONG_DIM(const string &f) noexcept(false);
//		virtual ~ERROR__OP_WITH_WRONG_DIM() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__OP_WITH_WRONG_DIM<rmatrix_slice>: virtual public ERROR_RMATRIX, virtual public OP_WITH_WRONG_DIM
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__OP_WITH_WRONG_DIM() noexcept(false);
		ERROR__OP_WITH_WRONG_DIM(const string &f) noexcept(false);
//		virtual ~ERROR__OP_WITH_WRONG_DIM() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_BOUNDARIES<rmatrix_slice>: virtual public ERROR_RMATRIX, virtual public WRONG_BOUNDARIES
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__WRONG_BOUNDARIES() noexcept(false);
		ERROR__WRONG_BOUNDARIES(const string &f) noexcept(false);
//		virtual ~ERROR__WRONG_BOUNDARIES() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__NO_MORE_MEMORY<rmatrix_slice>: virtual public ERROR_RMATRIX, virtual public NO_MORE_MEMORY
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__NO_MORE_MEMORY() noexcept(false);
		ERROR__NO_MORE_MEMORY(const string &f) noexcept(false);
//		virtual ~ERROR__NO_MORE_MEMORY() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__SUB_ARRAY_TOO_BIG<rmatrix_slice>: virtual public ERROR_RMATRIX, virtual public SUB_ARRAY_TOO_BIG
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__SUB_ARRAY_TOO_BIG() noexcept(false);
		ERROR__SUB_ARRAY_TOO_BIG(const string &f) noexcept(false);
//		virtual ~ERROR__SUB_ARRAY_TOO_BIG() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__ROW_OR_COL_NOT_IN_MAT<rmatrix_slice>: virtual public ERROR_RMATRIX, virtual public ROW_OR_COL_NOT_IN_MAT
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false);
		ERROR__ROW_OR_COL_NOT_IN_MAT(const string &f) noexcept(false);
//		virtual ~ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_ROW_OR_COL<rmatrix_slice>: virtual public ERROR_RMATRIX, virtual public WRONG_ROW_OR_COL
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__WRONG_ROW_OR_COL() noexcept(false);
		ERROR__WRONG_ROW_OR_COL(const string &f) noexcept(false);
//		virtual ~ERROR__WRONG_ROW_OR_COL() noexcept(false) { }
//	private:
//		string fkt;
};


template <>
class ERROR__WRONG_BOUNDARIES<ivector_slice>: virtual public ERROR_IVECTOR, virtual public WRONG_BOUNDARIES
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__WRONG_BOUNDARIES() noexcept(false);
		ERROR__WRONG_BOUNDARIES(const string &f) noexcept(false);
//		virtual ~ERROR__WRONG_BOUNDARIES() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__NO_MORE_MEMORY<ivector_slice>: virtual public ERROR_IVECTOR, virtual public NO_MORE_MEMORY
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__NO_MORE_MEMORY() noexcept(false);
		ERROR__NO_MORE_MEMORY(const string &f) noexcept(false);
//		virtual ~ERROR__NO_MORE_MEMORY() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__SUB_ARRAY_TOO_BIG<ivector_slice>: virtual public ERROR_IVECTOR, virtual public SUB_ARRAY_TOO_BIG
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		ERROR__SUB_ARRAY_TOO_BIG() noexcept(false);
		ERROR__SUB_ARRAY_TOO_BIG(const string &f) noexcept(false);
//		virtual ~ERROR__SUB_ARRAY_TOO_BIG() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__ELEMENT_NOT_IN_VEC<ivector_slice>: virtual public ERROR_IVECTOR, virtual public ELEMENT_NOT_IN_VEC
{
	public:
		virtual int errnum() const noexcept(false) ;//  { return 8203; }
		virtual string errtext() const noexcept(false) ;//  { return fkt+": ERROR__ELEMENT_NOT_IN_VEC"; }
		ERROR__ELEMENT_NOT_IN_VEC() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__ELEMENT_NOT_IN_VEC(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__ELEMENT_NOT_IN_VEC() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__TYPE_CAST_OF_THICK_OBJ<ivector_slice>: virtual public ERROR_IVECTOR, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 8206; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__TYPE_CAST_OF_THICK_OBJ"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false) ;//  fkt="<unknown function>"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__TYPE_CAST_OF_THICK_OBJ<imatrix_slice>: virtual public ERROR_IMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false) ;//  { return 12206; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__TYPE_CAST_OF_THICK_OBJ"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__USE_OF_UNINITIALIZED_OBJ<ivector_slice>: virtual public ERROR_IVECTOR, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false) ;//  { return 8208; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__USE_OF_UNINITIALIZED_OBJ"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__USE_OF_UNINITIALIZED_OBJ<imatrix_slice>: virtual public ERROR_IMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 12208; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__USE_OF_UNINITIALIZED_OBJ"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__OP_WITH_WRONG_DIM<ivector_slice>: virtual public ERROR_IVECTOR, virtual public OP_WITH_WRONG_DIM
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 8207; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__OP_WITH_WRONG_DIM"; }
		ERROR__OP_WITH_WRONG_DIM() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__OP_WITH_WRONG_DIM(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__OP_WITH_WRONG_DIM() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__OP_WITH_WRONG_DIM<imatrix_slice>: virtual public ERROR_IMATRIX, virtual public OP_WITH_WRONG_DIM
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 12207; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__OP_WITH_WRONG_DIM"; }
		ERROR__OP_WITH_WRONG_DIM() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__OP_WITH_WRONG_DIM(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__OP_WITH_WRONG_DIM() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_BOUNDARIES<imatrix_slice>: virtual public ERROR_IMATRIX, virtual public WRONG_BOUNDARIES
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 12200; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__WRONG_BOUNDARIES"; }
		ERROR__WRONG_BOUNDARIES() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_BOUNDARIES(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__WRONG_BOUNDARIES() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__NO_MORE_MEMORY<imatrix_slice>: virtual public ERROR_IMATRIX, virtual public NO_MORE_MEMORY
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 12002; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__NO_MORE_MEMORY"; }
		ERROR__NO_MORE_MEMORY() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__NO_MORE_MEMORY(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__NO_MORE_MEMORY() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__SUB_ARRAY_TOO_BIG<imatrix_slice>: virtual public ERROR_IMATRIX, virtual public SUB_ARRAY_TOO_BIG
{
	public:
		virtual int errnum() const noexcept(false)  ;// {  return 12201; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__SUB_ARRAY_TOO_BIG"; }
		ERROR__SUB_ARRAY_TOO_BIG() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__SUB_ARRAY_TOO_BIG(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__SUB_ARRAY_TOO_BIG() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__ROW_OR_COL_NOT_IN_MAT<imatrix_slice>: virtual public ERROR_IMATRIX, virtual public ROW_OR_COL_NOT_IN_MAT
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 12204; }
		virtual string errtext() const noexcept(false)  ;//  return fkt+": ERROR__ROW_OR_COL_NOT_IN_MAT"; }
		ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__ROW_OR_COL_NOT_IN_MAT(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_ROW_OR_COL<imatrix_slice>: virtual public ERROR_IMATRIX, virtual public WRONG_ROW_OR_COL
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 12205; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__WRONG_ROW_OR_COL"; }
		ERROR__WRONG_ROW_OR_COL() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_ROW_OR_COL(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__WRONG_ROW_OR_COL() noexcept(false) { }
//	private:
//		string fkt;
};


template <>
class ERROR__TYPE_CAST_OF_THICK_OBJ<rmatrix_subv>: virtual public ERROR_RMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11206; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__TYPE_CAST_OF_THICK_OBJ"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__USE_OF_UNINITIALIZED_OBJ<rmatrix_subv>: virtual public ERROR_RMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11208; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__USE_OF_UNINITIALIZED_OBJ"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__OP_WITH_WRONG_DIM<rmatrix_subv>: virtual public ERROR_RMATRIX, virtual public OP_WITH_WRONG_DIM
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11207; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__OP_WITH_WRONG_DIM"; }
		ERROR__OP_WITH_WRONG_DIM() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__OP_WITH_WRONG_DIM(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__OP_WITH_WRONG_DIM() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_BOUNDARIES<rmatrix_subv>: virtual public ERROR_RMATRIX, virtual public WRONG_BOUNDARIES
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11200; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__WRONG_BOUNDARIES"; }
		ERROR__WRONG_BOUNDARIES() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_BOUNDARIES(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__WRONG_BOUNDARIES() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__NO_MORE_MEMORY<rmatrix_subv>: virtual public ERROR_RMATRIX, virtual public NO_MORE_MEMORY
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11002; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__NO_MORE_MEMORY"; }
		ERROR__NO_MORE_MEMORY() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__NO_MORE_MEMORY(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__NO_MORE_MEMORY() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__SUB_ARRAY_TOO_BIG<rmatrix_subv>: virtual public ERROR_RMATRIX, virtual public SUB_ARRAY_TOO_BIG
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11201; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__SUB_ARRAY_TOO_BIG"; }
		ERROR__SUB_ARRAY_TOO_BIG() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__SUB_ARRAY_TOO_BIG(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__SUB_ARRAY_TOO_BIG() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__ROW_OR_COL_NOT_IN_MAT<rmatrix_subv>: virtual public ERROR_RMATRIX, virtual public ROW_OR_COL_NOT_IN_MAT
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11204; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__ROW_OR_COL_NOT_IN_MAT"; }
		ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__ROW_OR_COL_NOT_IN_MAT(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_ROW_OR_COL<rmatrix_subv>: virtual public ERROR_RMATRIX, virtual public WRONG_ROW_OR_COL
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11205; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__WRONG_ROW_OR_COL"; }
		ERROR__WRONG_ROW_OR_COL() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_ROW_OR_COL(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__WRONG_ROW_OR_COL() noexcept(false) { }
//	private:
//		string fkt;
};


template <>
class ERROR__TYPE_CAST_OF_THICK_OBJ<imatrix_subv>: virtual public ERROR_IMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 12206; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__TYPE_CAST_OF_THICK_OBJ"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__USE_OF_UNINITIALIZED_OBJ<imatrix_subv>: virtual public ERROR_IMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 12208; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__USE_OF_UNINITIALIZED_OBJ"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__OP_WITH_WRONG_DIM<imatrix_subv>: virtual public ERROR_IMATRIX, virtual public OP_WITH_WRONG_DIM
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 12207; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__OP_WITH_WRONG_DIM"; }
		ERROR__OP_WITH_WRONG_DIM() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__OP_WITH_WRONG_DIM(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__OP_WITH_WRONG_DIM() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_BOUNDARIES<imatrix_subv>: virtual public ERROR_IMATRIX, virtual public WRONG_BOUNDARIES
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 12200; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__WRONG_BOUNDARIES"; }
		ERROR__WRONG_BOUNDARIES() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_BOUNDARIES(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__WRONG_BOUNDARIES() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__NO_MORE_MEMORY<imatrix_subv>: virtual public ERROR_IMATRIX, virtual public NO_MORE_MEMORY
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 12002; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__NO_MORE_MEMORY"; }
		ERROR__NO_MORE_MEMORY() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__NO_MORE_MEMORY(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__NO_MORE_MEMORY() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__SUB_ARRAY_TOO_BIG<imatrix_subv>: virtual public ERROR_IMATRIX, virtual public SUB_ARRAY_TOO_BIG
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 12201; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__SUB_ARRAY_TOO_BIG"; }
		ERROR__SUB_ARRAY_TOO_BIG() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__SUB_ARRAY_TOO_BIG(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__SUB_ARRAY_TOO_BIG() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__ROW_OR_COL_NOT_IN_MAT<imatrix_subv>: virtual public ERROR_IMATRIX, virtual public ROW_OR_COL_NOT_IN_MAT
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 12204; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__ROW_OR_COL_NOT_IN_MAT"; }
		ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__ROW_OR_COL_NOT_IN_MAT(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_ROW_OR_COL<imatrix_subv>: virtual public ERROR_IMATRIX, virtual public WRONG_ROW_OR_COL
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 12205; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__WRONG_ROW_OR_COL"; }
		ERROR__WRONG_ROW_OR_COL() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_ROW_OR_COL(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__WRONG_ROW_OR_COL() noexcept(false) { }
//	private:
//		string fkt;
};



template <>
class ERROR__WRONG_BOUNDARIES<intvector>: virtual public ERROR_INTVECTOR, virtual public WRONG_BOUNDARIES
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7200; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR_INTVECTOR_WRONG_BOUNDARIES"; }
		ERROR__WRONG_BOUNDARIES() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_BOUNDARIES(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR_INTVECTOR_WRONG_BOUNDARIES() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__NO_MORE_MEMORY<intvector>: virtual public ERROR_INTVECTOR, virtual public NO_MORE_MEMORY
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7002; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__NO_MORE_MEMORY"; }
		ERROR__NO_MORE_MEMORY() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__NO_MORE_MEMORY(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__NO_MORE_MEMORY() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__SUB_ARRAY_TOO_BIG<intvector>: virtual public ERROR_INTVECTOR, virtual public SUB_ARRAY_TOO_BIG
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7201; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__SUB_ARRAY_TOO_BIG"; }
		ERROR__SUB_ARRAY_TOO_BIG() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__SUB_ARRAY_TOO_BIG(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__SUB_ARRAY_TOO_BIG() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__ELEMENT_NOT_IN_VEC<intvector>: virtual public ERROR_INTVECTOR, virtual public ELEMENT_NOT_IN_VEC
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7203; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__ELEMENT_NOT_IN_VEC"; }
		ERROR__ELEMENT_NOT_IN_VEC() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__ELEMENT_NOT_IN_VEC(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__ELEMENT_NOT_IN_VEC() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__TYPE_CAST_OF_THICK_OBJ<intvector>: virtual public ERROR_INTVECTOR, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7206; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__TYPE_CAST_OF_THICK_OBJ"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__TYPE_CAST_OF_THICK_OBJ<intmatrix>: virtual public ERROR_INTMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11206; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__TYPE_CAST_OF_THICK_OBJ"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__USE_OF_UNINITIALIZED_OBJ<intvector>: virtual public ERROR_INTVECTOR, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7208; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__USE_OF_UNINITIALIZED_OBJ"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__USE_OF_UNINITIALIZED_OBJ<intmatrix>: virtual public ERROR_INTMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11208; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__USE_OF_UNINITIALIZED_OBJ"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__OP_WITH_WRONG_DIM<intvector>: virtual public ERROR_INTVECTOR, virtual public OP_WITH_WRONG_DIM
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7207; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__OP_WITH_WRONG_DIM"; }
		ERROR__OP_WITH_WRONG_DIM() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__OP_WITH_WRONG_DIM(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__OP_WITH_WRONG_DIM() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__OP_WITH_WRONG_DIM<intmatrix>: virtual public ERROR_INTMATRIX, virtual public OP_WITH_WRONG_DIM
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11207; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__OP_WITH_WRONG_DIM"; }
		ERROR__OP_WITH_WRONG_DIM() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__OP_WITH_WRONG_DIM(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__OP_WITH_WRONG_DIM() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_BOUNDARIES<intmatrix>: virtual public ERROR_INTMATRIX, virtual public WRONG_BOUNDARIES
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11200; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__WRONG_BOUNDARIES"; }
		ERROR__WRONG_BOUNDARIES() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_BOUNDARIES(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__WRONG_BOUNDARIES() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__NO_MORE_MEMORY<intmatrix>: virtual public ERROR_INTMATRIX, virtual public NO_MORE_MEMORY
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11002; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__NO_MORE_MEMORY"; }
		ERROR__NO_MORE_MEMORY() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__NO_MORE_MEMORY(const string &f)  noexcept(false) ;//{ fkt=f; }
//		virtual ~ERROR__NO_MORE_MEMORY() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__SUB_ARRAY_TOO_BIG<intmatrix>: virtual public ERROR_INTMATRIX, virtual public SUB_ARRAY_TOO_BIG
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11201; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__SUB_ARRAY_TOO_BIG"; }
		ERROR__SUB_ARRAY_TOO_BIG() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__SUB_ARRAY_TOO_BIG(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__SUB_ARRAY_TOO_BIG() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__ROW_OR_COL_NOT_IN_MAT<intmatrix>: virtual public ERROR_INTMATRIX, virtual public ROW_OR_COL_NOT_IN_MAT
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11204; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__ROW_OR_COL_NOT_IN_MAT"; }
		ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__ROW_OR_COL_NOT_IN_MAT(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_ROW_OR_COL<intmatrix>: virtual public ERROR_INTMATRIX, virtual public WRONG_ROW_OR_COL
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11205; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__WRONG_ROW_OR_COL"; }
		ERROR__WRONG_ROW_OR_COL() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_ROW_OR_COL(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__WRONG_ROW_OR_COL() noexcept(false) { }
//	private:
//		string fkt;
};


template <>
class ERROR__WRONG_BOUNDARIES<intvector_slice>: virtual public ERROR_INTVECTOR, virtual public WRONG_BOUNDARIES
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7200; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR_INTVECTOR_WRONG_BOUNDARIES"; }
		ERROR__WRONG_BOUNDARIES() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_BOUNDARIES(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR_INTVECTOR_WRONG_BOUNDARIES() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__NO_MORE_MEMORY<intvector_slice>: virtual public ERROR_INTVECTOR, virtual public NO_MORE_MEMORY
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7002; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__NO_MORE_MEMORY"; }
		ERROR__NO_MORE_MEMORY() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__NO_MORE_MEMORY(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__NO_MORE_MEMORY() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__SUB_ARRAY_TOO_BIG<intvector_slice>: virtual public ERROR_INTVECTOR, virtual public SUB_ARRAY_TOO_BIG
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7201; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__SUB_ARRAY_TOO_BIG"; }
		ERROR__SUB_ARRAY_TOO_BIG() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__SUB_ARRAY_TOO_BIG(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__SUB_ARRAY_TOO_BIG() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__ELEMENT_NOT_IN_VEC<intvector_slice>: virtual public ERROR_INTVECTOR, virtual public ELEMENT_NOT_IN_VEC
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7203; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__ELEMENT_NOT_IN_VEC"; }
		ERROR__ELEMENT_NOT_IN_VEC() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__ELEMENT_NOT_IN_VEC(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__ELEMENT_NOT_IN_VEC() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__TYPE_CAST_OF_THICK_OBJ<intvector_slice>: virtual public ERROR_INTVECTOR, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7206; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__TYPE_CAST_OF_THICK_OBJ"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__TYPE_CAST_OF_THICK_OBJ<intmatrix_slice>: virtual public ERROR_INTMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11206; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__TYPE_CAST_OF_THICK_OBJ"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__USE_OF_UNINITIALIZED_OBJ<intvector_slice>: virtual public ERROR_INTVECTOR, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7208; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__USE_OF_UNINITIALIZED_OBJ"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__USE_OF_UNINITIALIZED_OBJ<intmatrix_slice>: virtual public ERROR_INTMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11208; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__USE_OF_UNINITIALIZED_OBJ"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__OP_WITH_WRONG_DIM<intvector_slice>: virtual public ERROR_INTVECTOR, virtual public OP_WITH_WRONG_DIM
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7207; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__OP_WITH_WRONG_DIM"; }
		ERROR__OP_WITH_WRONG_DIM() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__OP_WITH_WRONG_DIM(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__OP_WITH_WRONG_DIM() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__OP_WITH_WRONG_DIM<intmatrix_slice>: virtual public ERROR_INTMATRIX, virtual public OP_WITH_WRONG_DIM
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11207; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__OP_WITH_WRONG_DIM"; }
		ERROR__OP_WITH_WRONG_DIM() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__OP_WITH_WRONG_DIM(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__OP_WITH_WRONG_DIM() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_BOUNDARIES<intmatrix_slice>: virtual public ERROR_INTMATRIX, virtual public WRONG_BOUNDARIES
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11200; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__WRONG_BOUNDARIES"; }
		ERROR__WRONG_BOUNDARIES() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_BOUNDARIES(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__WRONG_BOUNDARIES() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__NO_MORE_MEMORY<intmatrix_slice>: virtual public ERROR_INTMATRIX, virtual public NO_MORE_MEMORY
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11002; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__NO_MORE_MEMORY"; }
		ERROR__NO_MORE_MEMORY() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__NO_MORE_MEMORY(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__NO_MORE_MEMORY() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__SUB_ARRAY_TOO_BIG<intmatrix_slice>: virtual public ERROR_INTMATRIX, virtual public SUB_ARRAY_TOO_BIG
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11201; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__SUB_ARRAY_TOO_BIG"; }
		ERROR__SUB_ARRAY_TOO_BIG() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__SUB_ARRAY_TOO_BIG(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__SUB_ARRAY_TOO_BIG() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__ROW_OR_COL_NOT_IN_MAT<intmatrix_slice>: virtual public ERROR_INTMATRIX, virtual public ROW_OR_COL_NOT_IN_MAT
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11204; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__ROW_OR_COL_NOT_IN_MAT"; }
		ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__ROW_OR_COL_NOT_IN_MAT(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_ROW_OR_COL<intmatrix_slice>: virtual public ERROR_INTMATRIX, virtual public WRONG_ROW_OR_COL
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11205; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__WRONG_ROW_OR_COL"; }
		ERROR__WRONG_ROW_OR_COL() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_ROW_OR_COL(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__WRONG_ROW_OR_COL() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__TYPE_CAST_OF_THICK_OBJ<intmatrix_subv>: virtual public ERROR_INTMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11206; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__TYPE_CAST_OF_THICK_OBJ"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__USE_OF_UNINITIALIZED_OBJ<intmatrix_subv>: virtual public ERROR_INTMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11208; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__USE_OF_UNINITIALIZED_OBJ"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__OP_WITH_WRONG_DIM<intmatrix_subv>: virtual public ERROR_INTMATRIX, virtual public OP_WITH_WRONG_DIM
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11207; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__OP_WITH_WRONG_DIM"; }
		ERROR__OP_WITH_WRONG_DIM() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__OP_WITH_WRONG_DIM(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__OP_WITH_WRONG_DIM() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_BOUNDARIES<intmatrix_subv>: virtual public ERROR_INTMATRIX, virtual public WRONG_BOUNDARIES
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11200; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__WRONG_BOUNDARIES"; }
		ERROR__WRONG_BOUNDARIES() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_BOUNDARIES(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__WRONG_BOUNDARIES() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__NO_MORE_MEMORY<intmatrix_subv>: virtual public ERROR_INTMATRIX, virtual public NO_MORE_MEMORY
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11002; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__NO_MORE_MEMORY"; }
		ERROR__NO_MORE_MEMORY() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__NO_MORE_MEMORY(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__NO_MORE_MEMORY() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__SUB_ARRAY_TOO_BIG<intmatrix_subv>: virtual public ERROR_INTMATRIX, virtual public SUB_ARRAY_TOO_BIG
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11201; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__SUB_ARRAY_TOO_BIG"; }
		ERROR__SUB_ARRAY_TOO_BIG() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__SUB_ARRAY_TOO_BIG(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__SUB_ARRAY_TOO_BIG() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__ROW_OR_COL_NOT_IN_MAT<intmatrix_subv>: virtual public ERROR_INTMATRIX, virtual public ROW_OR_COL_NOT_IN_MAT
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11204; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__ROW_OR_COL_NOT_IN_MAT"; }
		ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__ROW_OR_COL_NOT_IN_MAT(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_ROW_OR_COL<intmatrix_subv>: virtual public ERROR_INTMATRIX, virtual public WRONG_ROW_OR_COL
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11205; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__WRONG_ROW_OR_COL"; }
		ERROR__WRONG_ROW_OR_COL() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_ROW_OR_COL(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__WRONG_ROW_OR_COL() noexcept(false) { }
//	private:
//		string fkt;
};

// ===

template <>
class ERROR__WRONG_BOUNDARIES<cvector>: virtual public ERROR_CVECTOR, virtual public WRONG_BOUNDARIES
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7200; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR_CVECTOR_WRONG_BOUNDARIES"; }
		ERROR__WRONG_BOUNDARIES() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_BOUNDARIES(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR_CVECTOR_WRONG_BOUNDARIES() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__NO_MORE_MEMORY<cvector>: virtual public ERROR_CVECTOR, virtual public NO_MORE_MEMORY
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7002; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__NO_MORE_MEMORY"; }
		ERROR__NO_MORE_MEMORY() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__NO_MORE_MEMORY(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__NO_MORE_MEMORY() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__SUB_ARRAY_TOO_BIG<cvector>: virtual public ERROR_CVECTOR, virtual public SUB_ARRAY_TOO_BIG
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7201; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__SUB_ARRAY_TOO_BIG"; }
		ERROR__SUB_ARRAY_TOO_BIG() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__SUB_ARRAY_TOO_BIG(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__SUB_ARRAY_TOO_BIG() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__ELEMENT_NOT_IN_VEC<cvector>: virtual public ERROR_CVECTOR, virtual public ELEMENT_NOT_IN_VEC
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7203; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__ELEMENT_NOT_IN_VEC"; }
		ERROR__ELEMENT_NOT_IN_VEC() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__ELEMENT_NOT_IN_VEC(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__ELEMENT_NOT_IN_VEC() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__TYPE_CAST_OF_THICK_OBJ<cvector>: virtual public ERROR_CVECTOR, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7206; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__TYPE_CAST_OF_THICK_OBJ"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__TYPE_CAST_OF_THICK_OBJ<cmatrix>: virtual public ERROR_CMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11206; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__TYPE_CAST_OF_THICK_OBJ"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__USE_OF_UNINITIALIZED_OBJ<cvector>: virtual public ERROR_CVECTOR, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7208; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__USE_OF_UNINITIALIZED_OBJ"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__USE_OF_UNINITIALIZED_OBJ<cmatrix>: virtual public ERROR_CMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11208; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__USE_OF_UNINITIALIZED_OBJ"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__OP_WITH_WRONG_DIM<cvector>: virtual public ERROR_CVECTOR, virtual public OP_WITH_WRONG_DIM
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7207; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__OP_WITH_WRONG_DIM"; }
		ERROR__OP_WITH_WRONG_DIM() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__OP_WITH_WRONG_DIM(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__OP_WITH_WRONG_DIM() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__OP_WITH_WRONG_DIM<cmatrix>: virtual public ERROR_CMATRIX, virtual public OP_WITH_WRONG_DIM
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11207; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__OP_WITH_WRONG_DIM"; }
		ERROR__OP_WITH_WRONG_DIM() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__OP_WITH_WRONG_DIM(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__OP_WITH_WRONG_DIM() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_BOUNDARIES<cmatrix>: virtual public ERROR_CMATRIX, virtual public WRONG_BOUNDARIES
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11200; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__WRONG_BOUNDARIES"; }
		ERROR__WRONG_BOUNDARIES() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_BOUNDARIES(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__WRONG_BOUNDARIES() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__NO_MORE_MEMORY<cmatrix>: virtual public ERROR_CMATRIX, virtual public NO_MORE_MEMORY
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11002; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__NO_MORE_MEMORY"; }
		ERROR__NO_MORE_MEMORY() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__NO_MORE_MEMORY(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__NO_MORE_MEMORY() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__SUB_ARRAY_TOO_BIG<cmatrix>: virtual public ERROR_CMATRIX, virtual public SUB_ARRAY_TOO_BIG
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11201; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__SUB_ARRAY_TOO_BIG"; }
		ERROR__SUB_ARRAY_TOO_BIG() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__SUB_ARRAY_TOO_BIG(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__SUB_ARRAY_TOO_BIG() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__ROW_OR_COL_NOT_IN_MAT<cmatrix>: virtual public ERROR_CMATRIX, virtual public ROW_OR_COL_NOT_IN_MAT
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11204; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__ROW_OR_COL_NOT_IN_MAT"; }
		ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__ROW_OR_COL_NOT_IN_MAT(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_ROW_OR_COL<cmatrix>: virtual public ERROR_CMATRIX, virtual public WRONG_ROW_OR_COL
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11205; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__WRONG_ROW_OR_COL"; }
		ERROR__WRONG_ROW_OR_COL() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_ROW_OR_COL(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__WRONG_ROW_OR_COL() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_BOUNDARIES<cvector_slice>: virtual public ERROR_CVECTOR, virtual public WRONG_BOUNDARIES
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7200; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR_CVECTOR_WRONG_BOUNDARIES"; }
		ERROR__WRONG_BOUNDARIES() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_BOUNDARIES(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR_CVECTOR_WRONG_BOUNDARIES() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__NO_MORE_MEMORY<cvector_slice>: virtual public ERROR_CVECTOR, virtual public NO_MORE_MEMORY
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7002; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__NO_MORE_MEMORY"; }
		ERROR__NO_MORE_MEMORY() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__NO_MORE_MEMORY(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__NO_MORE_MEMORY() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__SUB_ARRAY_TOO_BIG<cvector_slice>: virtual public ERROR_CVECTOR, virtual public SUB_ARRAY_TOO_BIG
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7201; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__SUB_ARRAY_TOO_BIG"; }
		ERROR__SUB_ARRAY_TOO_BIG() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__SUB_ARRAY_TOO_BIG(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__SUB_ARRAY_TOO_BIG() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__ELEMENT_NOT_IN_VEC<cvector_slice>: virtual public ERROR_CVECTOR, virtual public ELEMENT_NOT_IN_VEC
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7203; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__ELEMENT_NOT_IN_VEC"; }
		ERROR__ELEMENT_NOT_IN_VEC() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__ELEMENT_NOT_IN_VEC(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__ELEMENT_NOT_IN_VEC() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__TYPE_CAST_OF_THICK_OBJ<cvector_slice>: virtual public ERROR_CVECTOR, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7206; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__TYPE_CAST_OF_THICK_OBJ"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__TYPE_CAST_OF_THICK_OBJ<cmatrix_slice>: virtual public ERROR_CMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11206; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__TYPE_CAST_OF_THICK_OBJ"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__USE_OF_UNINITIALIZED_OBJ<cvector_slice>: virtual public ERROR_CVECTOR, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7208; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__USE_OF_UNINITIALIZED_OBJ"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__USE_OF_UNINITIALIZED_OBJ<cmatrix_slice>: virtual public ERROR_CMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11208; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__USE_OF_UNINITIALIZED_OBJ"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__OP_WITH_WRONG_DIM<cvector_slice>: virtual public ERROR_CVECTOR, virtual public OP_WITH_WRONG_DIM
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7207; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__OP_WITH_WRONG_DIM"; }
		ERROR__OP_WITH_WRONG_DIM() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__OP_WITH_WRONG_DIM(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__OP_WITH_WRONG_DIM() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__OP_WITH_WRONG_DIM<cmatrix_slice>: virtual public ERROR_CMATRIX, virtual public OP_WITH_WRONG_DIM
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11207; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__OP_WITH_WRONG_DIM"; }
		ERROR__OP_WITH_WRONG_DIM() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__OP_WITH_WRONG_DIM(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__OP_WITH_WRONG_DIM() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_BOUNDARIES<cmatrix_slice>: virtual public ERROR_CMATRIX, virtual public WRONG_BOUNDARIES
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11200; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__WRONG_BOUNDARIES"; }
		ERROR__WRONG_BOUNDARIES() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_BOUNDARIES(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__WRONG_BOUNDARIES() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__NO_MORE_MEMORY<cmatrix_slice>: virtual public ERROR_CMATRIX, virtual public NO_MORE_MEMORY
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11002; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__NO_MORE_MEMORY"; }
		ERROR__NO_MORE_MEMORY() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__NO_MORE_MEMORY(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__NO_MORE_MEMORY() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__SUB_ARRAY_TOO_BIG<cmatrix_slice>: virtual public ERROR_CMATRIX, virtual public SUB_ARRAY_TOO_BIG
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11201; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__SUB_ARRAY_TOO_BIG"; }
		ERROR__SUB_ARRAY_TOO_BIG() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__SUB_ARRAY_TOO_BIG(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__SUB_ARRAY_TOO_BIG() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__ROW_OR_COL_NOT_IN_MAT<cmatrix_slice>: virtual public ERROR_CMATRIX, virtual public ROW_OR_COL_NOT_IN_MAT
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11204; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__ROW_OR_COL_NOT_IN_MAT"; }
		ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__ROW_OR_COL_NOT_IN_MAT(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_ROW_OR_COL<cmatrix_slice>: virtual public ERROR_CMATRIX, virtual public WRONG_ROW_OR_COL
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11205; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__WRONG_ROW_OR_COL"; }
		ERROR__WRONG_ROW_OR_COL() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_ROW_OR_COL(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__WRONG_ROW_OR_COL() noexcept(false) { }
//	private:
//		string fkt;
};


template <>
class ERROR__TYPE_CAST_OF_THICK_OBJ<cmatrix_subv>: virtual public ERROR_CMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11206; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__TYPE_CAST_OF_THICK_OBJ"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__USE_OF_UNINITIALIZED_OBJ<cmatrix_subv>: virtual public ERROR_CMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11208; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__USE_OF_UNINITIALIZED_OBJ"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__OP_WITH_WRONG_DIM<cmatrix_subv>: virtual public ERROR_CMATRIX, virtual public OP_WITH_WRONG_DIM
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11207; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__OP_WITH_WRONG_DIM"; }
		ERROR__OP_WITH_WRONG_DIM() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__OP_WITH_WRONG_DIM(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__OP_WITH_WRONG_DIM() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_BOUNDARIES<cmatrix_subv>: virtual public ERROR_CMATRIX, virtual public WRONG_BOUNDARIES
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11200; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__WRONG_BOUNDARIES"; }
		ERROR__WRONG_BOUNDARIES() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_BOUNDARIES(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__WRONG_BOUNDARIES() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__NO_MORE_MEMORY<cmatrix_subv>: virtual public ERROR_CMATRIX, virtual public NO_MORE_MEMORY
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11002; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__NO_MORE_MEMORY"; }
		ERROR__NO_MORE_MEMORY() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__NO_MORE_MEMORY(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__NO_MORE_MEMORY() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__SUB_ARRAY_TOO_BIG<cmatrix_subv>: virtual public ERROR_CMATRIX, virtual public SUB_ARRAY_TOO_BIG
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11201; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__SUB_ARRAY_TOO_BIG"; }
		ERROR__SUB_ARRAY_TOO_BIG() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__SUB_ARRAY_TOO_BIG(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__SUB_ARRAY_TOO_BIG() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__ROW_OR_COL_NOT_IN_MAT<cmatrix_subv>: virtual public ERROR_CMATRIX, virtual public ROW_OR_COL_NOT_IN_MAT
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11204; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__ROW_OR_COL_NOT_IN_MAT"; }
		ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__ROW_OR_COL_NOT_IN_MAT(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_ROW_OR_COL<cmatrix_subv>: virtual public ERROR_CMATRIX, virtual public WRONG_ROW_OR_COL
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11205; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__WRONG_ROW_OR_COL"; }
		ERROR__WRONG_ROW_OR_COL() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_ROW_OR_COL(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__WRONG_ROW_OR_COL() noexcept(false) { }
//	private:
//		string fkt;
};


template <>
class ERROR__WRONG_BOUNDARIES<civector>: virtual public ERROR_CIVECTOR, virtual public WRONG_BOUNDARIES
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7200; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR_CIVECTOR_WRONG_BOUNDARIES"; }
		ERROR__WRONG_BOUNDARIES() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_BOUNDARIES(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR_CIVECTOR_WRONG_BOUNDARIES() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__NO_MORE_MEMORY<civector>: virtual public ERROR_CIVECTOR, virtual public NO_MORE_MEMORY
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7002; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__NO_MORE_MEMORY"; }
		ERROR__NO_MORE_MEMORY() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__NO_MORE_MEMORY(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__NO_MORE_MEMORY() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__SUB_ARRAY_TOO_BIG<civector>: virtual public ERROR_CIVECTOR, virtual public SUB_ARRAY_TOO_BIG
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7201; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__SUB_ARRAY_TOO_BIG"; }
		ERROR__SUB_ARRAY_TOO_BIG() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__SUB_ARRAY_TOO_BIG(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__SUB_ARRAY_TOO_BIG() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__ELEMENT_NOT_IN_VEC<civector>: virtual public ERROR_CIVECTOR, virtual public ELEMENT_NOT_IN_VEC
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7203; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__ELEMENT_NOT_IN_VEC"; }
		ERROR__ELEMENT_NOT_IN_VEC() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__ELEMENT_NOT_IN_VEC(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__ELEMENT_NOT_IN_VEC() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__TYPE_CAST_OF_THICK_OBJ<civector>: virtual public ERROR_CIVECTOR, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7206; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__TYPE_CAST_OF_THICK_OBJ"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__TYPE_CAST_OF_THICK_OBJ<cimatrix>: virtual public ERROR_CIMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11206; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__TYPE_CAST_OF_THICK_OBJ"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__USE_OF_UNINITIALIZED_OBJ<civector>: virtual public ERROR_CIVECTOR, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7208; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__USE_OF_UNINITIALIZED_OBJ"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__USE_OF_UNINITIALIZED_OBJ<cimatrix>: virtual public ERROR_CIMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11208; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__USE_OF_UNINITIALIZED_OBJ"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__OP_WITH_WRONG_DIM<civector>: virtual public ERROR_CIVECTOR, virtual public OP_WITH_WRONG_DIM
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7207; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__OP_WITH_WRONG_DIM"; }
		ERROR__OP_WITH_WRONG_DIM() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__OP_WITH_WRONG_DIM(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__OP_WITH_WRONG_DIM() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__OP_WITH_WRONG_DIM<cimatrix>: virtual public ERROR_CIMATRIX, virtual public OP_WITH_WRONG_DIM
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11207; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__OP_WITH_WRONG_DIM"; }
		ERROR__OP_WITH_WRONG_DIM() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__OP_WITH_WRONG_DIM(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__OP_WITH_WRONG_DIM() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_BOUNDARIES<cimatrix>: virtual public ERROR_CIMATRIX, virtual public WRONG_BOUNDARIES
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11200; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__WRONG_BOUNDARIES"; }
		ERROR__WRONG_BOUNDARIES() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_BOUNDARIES(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__WRONG_BOUNDARIES() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__NO_MORE_MEMORY<cimatrix>: virtual public ERROR_CIMATRIX, virtual public NO_MORE_MEMORY
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11002; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__NO_MORE_MEMORY"; }
		ERROR__NO_MORE_MEMORY() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__NO_MORE_MEMORY(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__NO_MORE_MEMORY() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__SUB_ARRAY_TOO_BIG<cimatrix>: virtual public ERROR_CIMATRIX, virtual public SUB_ARRAY_TOO_BIG
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11201; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__SUB_ARRAY_TOO_BIG"; }
		ERROR__SUB_ARRAY_TOO_BIG() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__SUB_ARRAY_TOO_BIG(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__SUB_ARRAY_TOO_BIG() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__ROW_OR_COL_NOT_IN_MAT<cimatrix>: virtual public ERROR_CIMATRIX, virtual public ROW_OR_COL_NOT_IN_MAT
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11204; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__ROW_OR_COL_NOT_IN_MAT"; }
		ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__ROW_OR_COL_NOT_IN_MAT(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_ROW_OR_COL<cimatrix>: virtual public ERROR_CIMATRIX, virtual public WRONG_ROW_OR_COL
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11205; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__WRONG_ROW_OR_COL"; }
		ERROR__WRONG_ROW_OR_COL() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_ROW_OR_COL(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__WRONG_ROW_OR_COL() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_BOUNDARIES<civector_slice>: virtual public ERROR_CIVECTOR, virtual public WRONG_BOUNDARIES
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7200; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR_CIVECTOR_WRONG_BOUNDARIES"; }
		ERROR__WRONG_BOUNDARIES() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_BOUNDARIES(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR_CIVECTOR_WRONG_BOUNDARIES() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__NO_MORE_MEMORY<civector_slice>: virtual public ERROR_CIVECTOR, virtual public NO_MORE_MEMORY
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7002; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__NO_MORE_MEMORY"; }
		ERROR__NO_MORE_MEMORY() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__NO_MORE_MEMORY(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__NO_MORE_MEMORY() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__SUB_ARRAY_TOO_BIG<civector_slice>: virtual public ERROR_CIVECTOR, virtual public SUB_ARRAY_TOO_BIG
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7201; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__SUB_ARRAY_TOO_BIG"; }
		ERROR__SUB_ARRAY_TOO_BIG() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__SUB_ARRAY_TOO_BIG(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__SUB_ARRAY_TOO_BIG() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__ELEMENT_NOT_IN_VEC<civector_slice>: virtual public ERROR_CIVECTOR, virtual public ELEMENT_NOT_IN_VEC
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7203; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__ELEMENT_NOT_IN_VEC"; }
		ERROR__ELEMENT_NOT_IN_VEC() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__ELEMENT_NOT_IN_VEC(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__ELEMENT_NOT_IN_VEC() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__TYPE_CAST_OF_THICK_OBJ<civector_slice>: virtual public ERROR_CIVECTOR, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7206; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__TYPE_CAST_OF_THICK_OBJ"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__TYPE_CAST_OF_THICK_OBJ<cimatrix_slice>: virtual public ERROR_CIMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11206; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__TYPE_CAST_OF_THICK_OBJ"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__USE_OF_UNINITIALIZED_OBJ<civector_slice>: virtual public ERROR_CIVECTOR, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7208; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__USE_OF_UNINITIALIZED_OBJ"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__USE_OF_UNINITIALIZED_OBJ<cimatrix_slice>: virtual public ERROR_CIMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11208; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__USE_OF_UNINITIALIZED_OBJ"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__OP_WITH_WRONG_DIM<civector_slice>: virtual public ERROR_CIVECTOR, virtual public OP_WITH_WRONG_DIM
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7207; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__OP_WITH_WRONG_DIM"; }
		ERROR__OP_WITH_WRONG_DIM() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__OP_WITH_WRONG_DIM(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__OP_WITH_WRONG_DIM() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__OP_WITH_WRONG_DIM<cimatrix_slice>: virtual public ERROR_CIMATRIX, virtual public OP_WITH_WRONG_DIM
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11207; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__OP_WITH_WRONG_DIM"; }
		ERROR__OP_WITH_WRONG_DIM() noexcept(false) ;// { fkt="<unknown function>"; }
		ERROR__OP_WITH_WRONG_DIM(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__OP_WITH_WRONG_DIM() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_BOUNDARIES<cimatrix_slice>: virtual public ERROR_CIMATRIX, virtual public WRONG_BOUNDARIES
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11200; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__WRONG_BOUNDARIES"; }
		ERROR__WRONG_BOUNDARIES() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_BOUNDARIES(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__WRONG_BOUNDARIES() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__NO_MORE_MEMORY<cimatrix_slice>: virtual public ERROR_CIMATRIX, virtual public NO_MORE_MEMORY
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11002; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__NO_MORE_MEMORY"; }
		ERROR__NO_MORE_MEMORY() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__NO_MORE_MEMORY(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__NO_MORE_MEMORY() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__SUB_ARRAY_TOO_BIG<cimatrix_slice>: virtual public ERROR_CIMATRIX, virtual public SUB_ARRAY_TOO_BIG
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11201; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__SUB_ARRAY_TOO_BIG"; }
		ERROR__SUB_ARRAY_TOO_BIG() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__SUB_ARRAY_TOO_BIG(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__SUB_ARRAY_TOO_BIG() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__ROW_OR_COL_NOT_IN_MAT<cimatrix_slice>: virtual public ERROR_CIMATRIX, virtual public ROW_OR_COL_NOT_IN_MAT
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11204; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__ROW_OR_COL_NOT_IN_MAT"; }
		ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__ROW_OR_COL_NOT_IN_MAT(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_ROW_OR_COL<cimatrix_slice>: virtual public ERROR_CIMATRIX, virtual public WRONG_ROW_OR_COL
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11205; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__WRONG_ROW_OR_COL"; }
		ERROR__WRONG_ROW_OR_COL() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_ROW_OR_COL(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__WRONG_ROW_OR_COL() noexcept(false) { }
//	private:
//		string fkt;
};


template <>
class ERROR__TYPE_CAST_OF_THICK_OBJ<cimatrix_subv>: virtual public ERROR_CIMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11206; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__TYPE_CAST_OF_THICK_OBJ"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__USE_OF_UNINITIALIZED_OBJ<cimatrix_subv>: virtual public ERROR_CIMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11208; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__USE_OF_UNINITIALIZED_OBJ"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__OP_WITH_WRONG_DIM<cimatrix_subv>: virtual public ERROR_CIMATRIX, virtual public OP_WITH_WRONG_DIM
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11207; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__OP_WITH_WRONG_DIM"; }
		ERROR__OP_WITH_WRONG_DIM() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__OP_WITH_WRONG_DIM(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__OP_WITH_WRONG_DIM() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_BOUNDARIES<cimatrix_subv>: virtual public ERROR_CIMATRIX, virtual public WRONG_BOUNDARIES
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11200; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__WRONG_BOUNDARIES"; }
		ERROR__WRONG_BOUNDARIES() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_BOUNDARIES(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__WRONG_BOUNDARIES() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__NO_MORE_MEMORY<cimatrix_subv>: virtual public ERROR_CIMATRIX, virtual public NO_MORE_MEMORY
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11002; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__NO_MORE_MEMORY"; }
		ERROR__NO_MORE_MEMORY() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__NO_MORE_MEMORY(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__NO_MORE_MEMORY() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__SUB_ARRAY_TOO_BIG<cimatrix_subv>: virtual public ERROR_CIMATRIX, virtual public SUB_ARRAY_TOO_BIG
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11201; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__SUB_ARRAY_TOO_BIG"; }
		ERROR__SUB_ARRAY_TOO_BIG() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__SUB_ARRAY_TOO_BIG(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__SUB_ARRAY_TOO_BIG() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__ROW_OR_COL_NOT_IN_MAT<cimatrix_subv>: virtual public ERROR_CIMATRIX, virtual public ROW_OR_COL_NOT_IN_MAT
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11204; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__ROW_OR_COL_NOT_IN_MAT"; }
		ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__ROW_OR_COL_NOT_IN_MAT(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_ROW_OR_COL<cimatrix_subv>: virtual public ERROR_CIMATRIX, virtual public WRONG_ROW_OR_COL
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11205; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__WRONG_ROW_OR_COL"; }
		ERROR__WRONG_ROW_OR_COL() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_ROW_OR_COL(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__WRONG_ROW_OR_COL() noexcept(false) { }
//	private:
//		string fkt;
};



template <>
class ERROR__WRONG_BOUNDARIES<l_rvector>: virtual public ERROR_LRVECTOR, virtual public WRONG_BOUNDARIES
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7200; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR_LRVECTOR_WRONG_BOUNDARIES"; }
		ERROR__WRONG_BOUNDARIES() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_BOUNDARIES(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR_LRVECTOR_WRONG_BOUNDARIES() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__NO_MORE_MEMORY<l_rvector>: virtual public ERROR_LRVECTOR, virtual public NO_MORE_MEMORY
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7002; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__NO_MORE_MEMORY"; }
		ERROR__NO_MORE_MEMORY() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__NO_MORE_MEMORY(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__NO_MORE_MEMORY() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__SUB_ARRAY_TOO_BIG<l_rvector>: virtual public ERROR_LRVECTOR, virtual public SUB_ARRAY_TOO_BIG
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7201; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__SUB_ARRAY_TOO_BIG"; }
		ERROR__SUB_ARRAY_TOO_BIG() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__SUB_ARRAY_TOO_BIG(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__SUB_ARRAY_TOO_BIG() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__ELEMENT_NOT_IN_VEC<l_rvector>: virtual public ERROR_LRVECTOR, virtual public ELEMENT_NOT_IN_VEC
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7203; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__ELEMENT_NOT_IN_VEC"; }
		ERROR__ELEMENT_NOT_IN_VEC() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__ELEMENT_NOT_IN_VEC(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__ELEMENT_NOT_IN_VEC() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__TYPE_CAST_OF_THICK_OBJ<l_rvector>: virtual public ERROR_LRVECTOR, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7206; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__TYPE_CAST_OF_THICK_OBJ"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__TYPE_CAST_OF_THICK_OBJ<l_rmatrix>: virtual public ERROR_LRMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11206; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__TYPE_CAST_OF_THICK_OBJ"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__USE_OF_UNINITIALIZED_OBJ<l_rvector>: virtual public ERROR_LRVECTOR, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7208; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__USE_OF_UNINITIALIZED_OBJ"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__USE_OF_UNINITIALIZED_OBJ<l_rmatrix>: virtual public ERROR_LRMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11208; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__USE_OF_UNINITIALIZED_OBJ"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__OP_WITH_WRONG_DIM<l_rvector>: virtual public ERROR_LRVECTOR, virtual public OP_WITH_WRONG_DIM
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7207; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__OP_WITH_WRONG_DIM"; }
		ERROR__OP_WITH_WRONG_DIM() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__OP_WITH_WRONG_DIM(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__OP_WITH_WRONG_DIM() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__OP_WITH_WRONG_DIM<l_rmatrix>: virtual public ERROR_LRMATRIX, virtual public OP_WITH_WRONG_DIM
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11207; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__OP_WITH_WRONG_DIM"; }
		ERROR__OP_WITH_WRONG_DIM() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__OP_WITH_WRONG_DIM(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__OP_WITH_WRONG_DIM() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_BOUNDARIES<l_rmatrix>: virtual public ERROR_LRMATRIX, virtual public WRONG_BOUNDARIES
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11200; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__WRONG_BOUNDARIES"; }
		ERROR__WRONG_BOUNDARIES() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_BOUNDARIES(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__WRONG_BOUNDARIES() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__NO_MORE_MEMORY<l_rmatrix>: virtual public ERROR_LRMATRIX, virtual public NO_MORE_MEMORY
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11002; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__NO_MORE_MEMORY"; }
		ERROR__NO_MORE_MEMORY() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__NO_MORE_MEMORY(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__NO_MORE_MEMORY() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__SUB_ARRAY_TOO_BIG<l_rmatrix>: virtual public ERROR_LRMATRIX, virtual public SUB_ARRAY_TOO_BIG
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11201; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__SUB_ARRAY_TOO_BIG"; }
		ERROR__SUB_ARRAY_TOO_BIG() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__SUB_ARRAY_TOO_BIG(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__SUB_ARRAY_TOO_BIG() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__ROW_OR_COL_NOT_IN_MAT<l_rmatrix>: virtual public ERROR_LRMATRIX, virtual public ROW_OR_COL_NOT_IN_MAT
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11204; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__ROW_OR_COL_NOT_IN_MAT"; }
		ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__ROW_OR_COL_NOT_IN_MAT(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_ROW_OR_COL<l_rmatrix>: virtual public ERROR_LRMATRIX, virtual public WRONG_ROW_OR_COL
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11205; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__WRONG_ROW_OR_COL"; }
		ERROR__WRONG_ROW_OR_COL() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_ROW_OR_COL(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__WRONG_ROW_OR_COL() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_BOUNDARIES<l_rvector_slice>: virtual public ERROR_LRVECTOR, virtual public WRONG_BOUNDARIES
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7200; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR_LRVECTOR_WRONG_BOUNDARIES"; }
		ERROR__WRONG_BOUNDARIES() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_BOUNDARIES(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR_LRVECTOR_WRONG_BOUNDARIES() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__NO_MORE_MEMORY<l_rvector_slice>: virtual public ERROR_LRVECTOR, virtual public NO_MORE_MEMORY
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7002; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__NO_MORE_MEMORY"; }
		ERROR__NO_MORE_MEMORY() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__NO_MORE_MEMORY(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__NO_MORE_MEMORY() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__SUB_ARRAY_TOO_BIG<l_rvector_slice>: virtual public ERROR_LRVECTOR, virtual public SUB_ARRAY_TOO_BIG
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7201; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__SUB_ARRAY_TOO_BIG"; }
		ERROR__SUB_ARRAY_TOO_BIG() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__SUB_ARRAY_TOO_BIG(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__SUB_ARRAY_TOO_BIG() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__ELEMENT_NOT_IN_VEC<l_rvector_slice>: virtual public ERROR_LRVECTOR, virtual public ELEMENT_NOT_IN_VEC
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7203; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__ELEMENT_NOT_IN_VEC"; }
		ERROR__ELEMENT_NOT_IN_VEC() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__ELEMENT_NOT_IN_VEC(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__ELEMENT_NOT_IN_VEC() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__TYPE_CAST_OF_THICK_OBJ<l_rvector_slice>: virtual public ERROR_LRVECTOR, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7206; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__TYPE_CAST_OF_THICK_OBJ"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__TYPE_CAST_OF_THICK_OBJ<l_rmatrix_slice>: virtual public ERROR_LRMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11206; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__TYPE_CAST_OF_THICK_OBJ"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__USE_OF_UNINITIALIZED_OBJ<l_rvector_slice>: virtual public ERROR_LRVECTOR, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7208; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__USE_OF_UNINITIALIZED_OBJ"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__USE_OF_UNINITIALIZED_OBJ<l_rmatrix_slice>: virtual public ERROR_LRMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11208; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__USE_OF_UNINITIALIZED_OBJ"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__OP_WITH_WRONG_DIM<l_rvector_slice>: virtual public ERROR_LRVECTOR, virtual public OP_WITH_WRONG_DIM
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7207; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__OP_WITH_WRONG_DIM"; }
		ERROR__OP_WITH_WRONG_DIM() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__OP_WITH_WRONG_DIM(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__OP_WITH_WRONG_DIM() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__OP_WITH_WRONG_DIM<l_rmatrix_slice>: virtual public ERROR_LRMATRIX, virtual public OP_WITH_WRONG_DIM
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11207; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__OP_WITH_WRONG_DIM"; }
		ERROR__OP_WITH_WRONG_DIM() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__OP_WITH_WRONG_DIM(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__OP_WITH_WRONG_DIM() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_BOUNDARIES<l_rmatrix_slice>: virtual public ERROR_LRMATRIX, virtual public WRONG_BOUNDARIES
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11200; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__WRONG_BOUNDARIES"; }
		ERROR__WRONG_BOUNDARIES() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_BOUNDARIES(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__WRONG_BOUNDARIES() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__NO_MORE_MEMORY<l_rmatrix_slice>: virtual public ERROR_LRMATRIX, virtual public NO_MORE_MEMORY
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11002; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__NO_MORE_MEMORY"; }
		ERROR__NO_MORE_MEMORY() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__NO_MORE_MEMORY(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__NO_MORE_MEMORY() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__SUB_ARRAY_TOO_BIG<l_rmatrix_slice>: virtual public ERROR_LRMATRIX, virtual public SUB_ARRAY_TOO_BIG
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11201; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__SUB_ARRAY_TOO_BIG"; }
		ERROR__SUB_ARRAY_TOO_BIG() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__SUB_ARRAY_TOO_BIG(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__SUB_ARRAY_TOO_BIG() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__ROW_OR_COL_NOT_IN_MAT<l_rmatrix_slice>: virtual public ERROR_LRMATRIX, virtual public ROW_OR_COL_NOT_IN_MAT
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11204; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__ROW_OR_COL_NOT_IN_MAT"; }
		ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__ROW_OR_COL_NOT_IN_MAT(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_ROW_OR_COL<l_rmatrix_slice>: virtual public ERROR_LRMATRIX, virtual public WRONG_ROW_OR_COL
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11205; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__WRONG_ROW_OR_COL"; }
		ERROR__WRONG_ROW_OR_COL() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_ROW_OR_COL(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__WRONG_ROW_OR_COL() noexcept(false) { }
//	private:
//		string fkt;
};


template <>
class ERROR__TYPE_CAST_OF_THICK_OBJ<l_rmatrix_subv>: virtual public ERROR_LRMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11206; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__TYPE_CAST_OF_THICK_OBJ"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__USE_OF_UNINITIALIZED_OBJ<l_rmatrix_subv>: virtual public ERROR_LRMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11208; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__USE_OF_UNINITIALIZED_OBJ"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__OP_WITH_WRONG_DIM<l_rmatrix_subv>: virtual public ERROR_LRMATRIX, virtual public OP_WITH_WRONG_DIM
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11207; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__OP_WITH_WRONG_DIM"; }
		ERROR__OP_WITH_WRONG_DIM() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__OP_WITH_WRONG_DIM(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__OP_WITH_WRONG_DIM() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_BOUNDARIES<l_rmatrix_subv>: virtual public ERROR_LRMATRIX, virtual public WRONG_BOUNDARIES
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11200; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__WRONG_BOUNDARIES"; }
		ERROR__WRONG_BOUNDARIES() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_BOUNDARIES(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__WRONG_BOUNDARIES() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__NO_MORE_MEMORY<l_rmatrix_subv>: virtual public ERROR_LRMATRIX, virtual public NO_MORE_MEMORY
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11002; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__NO_MORE_MEMORY"; }
		ERROR__NO_MORE_MEMORY() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__NO_MORE_MEMORY(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__NO_MORE_MEMORY() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__SUB_ARRAY_TOO_BIG<l_rmatrix_subv>: virtual public ERROR_LRMATRIX, virtual public SUB_ARRAY_TOO_BIG
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11201; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__SUB_ARRAY_TOO_BIG"; }
		ERROR__SUB_ARRAY_TOO_BIG() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__SUB_ARRAY_TOO_BIG(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__SUB_ARRAY_TOO_BIG() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__ROW_OR_COL_NOT_IN_MAT<l_rmatrix_subv>: virtual public ERROR_LRMATRIX, virtual public ROW_OR_COL_NOT_IN_MAT
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11204; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__ROW_OR_COL_NOT_IN_MAT"; }
		ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__ROW_OR_COL_NOT_IN_MAT(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_ROW_OR_COL<l_rmatrix_subv>: virtual public ERROR_LRMATRIX, virtual public WRONG_ROW_OR_COL
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11205; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__WRONG_ROW_OR_COL"; }
		ERROR__WRONG_ROW_OR_COL() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_ROW_OR_COL(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__WRONG_ROW_OR_COL() noexcept(false) { }
//	private:
//		string fkt;
};


template <>
class ERROR__WRONG_BOUNDARIES<l_ivector>: virtual public ERROR_LIVECTOR, virtual public WRONG_BOUNDARIES
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7200; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR_LIVECTOR_WRONG_BOUNDARIES"; }
		ERROR__WRONG_BOUNDARIES() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_BOUNDARIES(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR_LIVECTOR_WRONG_BOUNDARIES() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__NO_MORE_MEMORY<l_ivector>: virtual public ERROR_LIVECTOR, virtual public NO_MORE_MEMORY
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7002; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__NO_MORE_MEMORY"; }
		ERROR__NO_MORE_MEMORY() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__NO_MORE_MEMORY(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__NO_MORE_MEMORY() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__SUB_ARRAY_TOO_BIG<l_ivector>: virtual public ERROR_LIVECTOR, virtual public SUB_ARRAY_TOO_BIG
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7201; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__SUB_ARRAY_TOO_BIG"; }
		ERROR__SUB_ARRAY_TOO_BIG() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__SUB_ARRAY_TOO_BIG(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__SUB_ARRAY_TOO_BIG() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__ELEMENT_NOT_IN_VEC<l_ivector>: virtual public ERROR_LIVECTOR, virtual public ELEMENT_NOT_IN_VEC
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7203; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__ELEMENT_NOT_IN_VEC"; }
		ERROR__ELEMENT_NOT_IN_VEC() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__ELEMENT_NOT_IN_VEC(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__ELEMENT_NOT_IN_VEC() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__TYPE_CAST_OF_THICK_OBJ<l_ivector>: virtual public ERROR_LIVECTOR, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7206; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__TYPE_CAST_OF_THICK_OBJ"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__TYPE_CAST_OF_THICK_OBJ<l_imatrix>: virtual public ERROR_LIMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11206; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__TYPE_CAST_OF_THICK_OBJ"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__USE_OF_UNINITIALIZED_OBJ<l_ivector>: virtual public ERROR_LIVECTOR, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7208; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__USE_OF_UNINITIALIZED_OBJ"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__USE_OF_UNINITIALIZED_OBJ<l_imatrix>: virtual public ERROR_LIMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11208; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__USE_OF_UNINITIALIZED_OBJ"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__OP_WITH_WRONG_DIM<l_ivector>: virtual public ERROR_LIVECTOR, virtual public OP_WITH_WRONG_DIM
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7207; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__OP_WITH_WRONG_DIM"; }
		ERROR__OP_WITH_WRONG_DIM() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__OP_WITH_WRONG_DIM(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__OP_WITH_WRONG_DIM() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__OP_WITH_WRONG_DIM<l_imatrix>: virtual public ERROR_LIMATRIX, virtual public OP_WITH_WRONG_DIM
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11207; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__OP_WITH_WRONG_DIM"; }
		ERROR__OP_WITH_WRONG_DIM() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__OP_WITH_WRONG_DIM(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__OP_WITH_WRONG_DIM() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_BOUNDARIES<l_imatrix>: virtual public ERROR_LIMATRIX, virtual public WRONG_BOUNDARIES
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11200; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__WRONG_BOUNDARIES"; }
		ERROR__WRONG_BOUNDARIES() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_BOUNDARIES(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__WRONG_BOUNDARIES() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__NO_MORE_MEMORY<l_imatrix>: virtual public ERROR_LIMATRIX, virtual public NO_MORE_MEMORY
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11002; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__NO_MORE_MEMORY"; }
		ERROR__NO_MORE_MEMORY() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__NO_MORE_MEMORY(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__NO_MORE_MEMORY() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__SUB_ARRAY_TOO_BIG<l_imatrix>: virtual public ERROR_LIMATRIX, virtual public SUB_ARRAY_TOO_BIG
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11201; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__SUB_ARRAY_TOO_BIG"; }
		ERROR__SUB_ARRAY_TOO_BIG() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__SUB_ARRAY_TOO_BIG(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__SUB_ARRAY_TOO_BIG() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__ROW_OR_COL_NOT_IN_MAT<l_imatrix>: virtual public ERROR_LIMATRIX, virtual public ROW_OR_COL_NOT_IN_MAT
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11204; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__ROW_OR_COL_NOT_IN_MAT"; }
		ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__ROW_OR_COL_NOT_IN_MAT(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_ROW_OR_COL<l_imatrix>: virtual public ERROR_LIMATRIX, virtual public WRONG_ROW_OR_COL
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11205; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__WRONG_ROW_OR_COL"; }
		ERROR__WRONG_ROW_OR_COL() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_ROW_OR_COL(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__WRONG_ROW_OR_COL() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_BOUNDARIES<l_ivector_slice>: virtual public ERROR_LIVECTOR, virtual public WRONG_BOUNDARIES
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7200; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR_LIVECTOR_WRONG_BOUNDARIES"; }
		ERROR__WRONG_BOUNDARIES() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_BOUNDARIES(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR_LIVECTOR_WRONG_BOUNDARIES() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__NO_MORE_MEMORY<l_ivector_slice>: virtual public ERROR_LIVECTOR, virtual public NO_MORE_MEMORY
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7002; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__NO_MORE_MEMORY"; }
		ERROR__NO_MORE_MEMORY() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__NO_MORE_MEMORY(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__NO_MORE_MEMORY() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__SUB_ARRAY_TOO_BIG<l_ivector_slice>: virtual public ERROR_LIVECTOR, virtual public SUB_ARRAY_TOO_BIG
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7201; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__SUB_ARRAY_TOO_BIG"; }
		ERROR__SUB_ARRAY_TOO_BIG() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__SUB_ARRAY_TOO_BIG(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__SUB_ARRAY_TOO_BIG() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__ELEMENT_NOT_IN_VEC<l_ivector_slice>: virtual public ERROR_LIVECTOR, virtual public ELEMENT_NOT_IN_VEC
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7203; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__ELEMENT_NOT_IN_VEC"; }
		ERROR__ELEMENT_NOT_IN_VEC() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__ELEMENT_NOT_IN_VEC(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__ELEMENT_NOT_IN_VEC() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__TYPE_CAST_OF_THICK_OBJ<l_ivector_slice>: virtual public ERROR_LIVECTOR, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7206; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__TYPE_CAST_OF_THICK_OBJ"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__TYPE_CAST_OF_THICK_OBJ<l_imatrix_slice>: virtual public ERROR_LIMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11206; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__TYPE_CAST_OF_THICK_OBJ"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__USE_OF_UNINITIALIZED_OBJ<l_ivector_slice>: virtual public ERROR_LIVECTOR, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7208; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__USE_OF_UNINITIALIZED_OBJ"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__USE_OF_UNINITIALIZED_OBJ<l_imatrix_slice>: virtual public ERROR_LIMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11208; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__USE_OF_UNINITIALIZED_OBJ"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__OP_WITH_WRONG_DIM<l_ivector_slice>: virtual public ERROR_LIVECTOR, virtual public OP_WITH_WRONG_DIM
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 7207; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__OP_WITH_WRONG_DIM"; }
		ERROR__OP_WITH_WRONG_DIM() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__OP_WITH_WRONG_DIM(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__OP_WITH_WRONG_DIM() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__OP_WITH_WRONG_DIM<l_imatrix_slice>: virtual public ERROR_LIMATRIX, virtual public OP_WITH_WRONG_DIM
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11207; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__OP_WITH_WRONG_DIM"; }
		ERROR__OP_WITH_WRONG_DIM() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__OP_WITH_WRONG_DIM(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__OP_WITH_WRONG_DIM() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_BOUNDARIES<l_imatrix_slice>: virtual public ERROR_LIMATRIX, virtual public WRONG_BOUNDARIES
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11200; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__WRONG_BOUNDARIES"; }
		ERROR__WRONG_BOUNDARIES() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_BOUNDARIES(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__WRONG_BOUNDARIES() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__NO_MORE_MEMORY<l_imatrix_slice>: virtual public ERROR_LIMATRIX, virtual public NO_MORE_MEMORY
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11002; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__NO_MORE_MEMORY"; }
		ERROR__NO_MORE_MEMORY() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__NO_MORE_MEMORY(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__NO_MORE_MEMORY() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__SUB_ARRAY_TOO_BIG<l_imatrix_slice>: virtual public ERROR_LIMATRIX, virtual public SUB_ARRAY_TOO_BIG
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11201; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__SUB_ARRAY_TOO_BIG"; }
		ERROR__SUB_ARRAY_TOO_BIG() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__SUB_ARRAY_TOO_BIG(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__SUB_ARRAY_TOO_BIG() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__ROW_OR_COL_NOT_IN_MAT<l_imatrix_slice>: virtual public ERROR_LIMATRIX, virtual public ROW_OR_COL_NOT_IN_MAT
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11204; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__ROW_OR_COL_NOT_IN_MAT"; }
		ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__ROW_OR_COL_NOT_IN_MAT(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_ROW_OR_COL<l_imatrix_slice>: virtual public ERROR_LIMATRIX, virtual public WRONG_ROW_OR_COL
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11205; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__WRONG_ROW_OR_COL"; }
		ERROR__WRONG_ROW_OR_COL() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_ROW_OR_COL(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__WRONG_ROW_OR_COL() noexcept(false) { }
//	private:
//		string fkt;
};


template <>
class ERROR__TYPE_CAST_OF_THICK_OBJ<l_imatrix_subv>: virtual public ERROR_LIMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11206; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__TYPE_CAST_OF_THICK_OBJ"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__TYPE_CAST_OF_THICK_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__TYPE_CAST_OF_THICK_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__USE_OF_UNINITIALIZED_OBJ<l_imatrix_subv>: virtual public ERROR_LIMATRIX, virtual public TYPE_CAST_OF_THICK_OBJ
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11208; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__USE_OF_UNINITIALIZED_OBJ"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__USE_OF_UNINITIALIZED_OBJ(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__USE_OF_UNINITIALIZED_OBJ() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__OP_WITH_WRONG_DIM<l_imatrix_subv>: virtual public ERROR_LIMATRIX, virtual public OP_WITH_WRONG_DIM
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11207; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__OP_WITH_WRONG_DIM"; }
		ERROR__OP_WITH_WRONG_DIM() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__OP_WITH_WRONG_DIM(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__OP_WITH_WRONG_DIM() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_BOUNDARIES<l_imatrix_subv>: virtual public ERROR_LIMATRIX, virtual public WRONG_BOUNDARIES
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11200; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__WRONG_BOUNDARIES"; }
		ERROR__WRONG_BOUNDARIES() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_BOUNDARIES(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__WRONG_BOUNDARIES() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__NO_MORE_MEMORY<l_imatrix_subv>: virtual public ERROR_LIMATRIX, virtual public NO_MORE_MEMORY
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11002; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__NO_MORE_MEMORY"; }
		ERROR__NO_MORE_MEMORY() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__NO_MORE_MEMORY(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__NO_MORE_MEMORY() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__SUB_ARRAY_TOO_BIG<l_imatrix_subv>: virtual public ERROR_LIMATRIX, virtual public SUB_ARRAY_TOO_BIG
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11201; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__SUB_ARRAY_TOO_BIG"; }
		ERROR__SUB_ARRAY_TOO_BIG() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__SUB_ARRAY_TOO_BIG(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__SUB_ARRAY_TOO_BIG() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__ROW_OR_COL_NOT_IN_MAT<l_imatrix_subv>: virtual public ERROR_LIMATRIX, virtual public ROW_OR_COL_NOT_IN_MAT
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11204; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__ROW_OR_COL_NOT_IN_MAT"; }
		ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__ROW_OR_COL_NOT_IN_MAT(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__ROW_OR_COL_NOT_IN_MAT() noexcept(false) { }
//	private:
//		string fkt;
};

template <>
class ERROR__WRONG_ROW_OR_COL<l_imatrix_subv>: virtual public ERROR_LIMATRIX, virtual public WRONG_ROW_OR_COL
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 11205; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR__WRONG_ROW_OR_COL"; }
		ERROR__WRONG_ROW_OR_COL() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR__WRONG_ROW_OR_COL(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR__WRONG_ROW_OR_COL() noexcept(false) { }
//	private:
//		string fkt;
};




#ifdef CXSC_INDEX_CHECK

typedef ERROR__WRONG_BOUNDARIES<rvector> ERROR_RVECTOR_WRONG_BOUNDARIES;
typedef ERROR__NO_MORE_MEMORY<rvector> ERROR_RVECTOR_NO_MORE_MEMORY;
typedef ERROR__SUB_ARRAY_TOO_BIG<rvector> ERROR_RVECTOR_SUB_ARRAY_TOO_BIG;
typedef ERROR__ELEMENT_NOT_IN_VEC<rvector> ERROR_RVECTOR_ELEMENT_NOT_IN_VEC;
typedef ERROR__TYPE_CAST_OF_THICK_OBJ<rvector> ERROR_RVECTOR_TYPE_CAST_OF_THICK_OBJ;
typedef ERROR__TYPE_CAST_OF_THICK_OBJ<rmatrix> ERROR_RMATRIX_TYPE_CAST_OF_THICK_OBJ;
typedef ERROR__USE_OF_UNINITIALIZED_OBJ<rvector> ERROR_RVECTOR_USE_OF_UNINITIALIZED_OBJ;
typedef ERROR__USE_OF_UNINITIALIZED_OBJ<rmatrix> ERROR_RMATRIX_USE_OF_UNINITIALIZED_OBJ;
typedef ERROR__OP_WITH_WRONG_DIM<rvector> ERROR_RVECTOR_OP_WITH_WRONG_DIM;
typedef ERROR__OP_WITH_WRONG_DIM<rmatrix> ERROR_RMATRIX_OP_WITH_WRONG_DIM;
typedef ERROR__WRONG_BOUNDARIES<rmatrix> ERROR_RMATRIX_WRONG_BOUNDARIES;
typedef ERROR__NO_MORE_MEMORY<rmatrix> ERROR_RMATRIX_NO_MORE_MEMORY;
typedef ERROR__SUB_ARRAY_TOO_BIG<rmatrix> ERROR_RMATRIX_SUB_ARRAY_TOO_BIG;
typedef ERROR__ROW_OR_COL_NOT_IN_MAT<rmatrix> ERROR_RMATRIX_ROW_OR_COL_NOT_IN_MAT;
typedef ERROR__WRONG_ROW_OR_COL<rmatrix> ERROR_RMATRIX_WRONG_ROW_OR_COL;
typedef ERROR__WRONG_BOUNDARIES<cvector> ERROR_CVECTOR_WRONG_BOUNDARIES;
typedef ERROR__NO_MORE_MEMORY<cvector> ERROR_CVECTOR_NO_MORE_MEMORY;
typedef ERROR__SUB_ARRAY_TOO_BIG<cvector> ERROR_CVECTOR_SUB_ARRAY_TOO_BIG;
typedef ERROR__ELEMENT_NOT_IN_VEC<cvector> ERROR_CVECTOR_ELEMENT_NOT_IN_VEC;
typedef ERROR__TYPE_CAST_OF_THICK_OBJ<cvector> ERROR_CVECTOR_TYPE_CAST_OF_THICK_OBJ;
typedef ERROR__TYPE_CAST_OF_THICK_OBJ<cmatrix> ERROR_CMATRIX_TYPE_CAST_OF_THICK_OBJ;
typedef ERROR__USE_OF_UNINITIALIZED_OBJ<cvector> ERROR_CVECTOR_USE_OF_UNINITIALIZED_OBJ;
typedef ERROR__USE_OF_UNINITIALIZED_OBJ<cmatrix> ERROR_CMATRIX_USE_OF_UNINITIALIZED_OBJ;
typedef ERROR__OP_WITH_WRONG_DIM<cvector> ERROR_CVECTOR_OP_WITH_WRONG_DIM;
typedef ERROR__OP_WITH_WRONG_DIM<cmatrix> ERROR_CMATRIX_OP_WITH_WRONG_DIM;
typedef ERROR__WRONG_BOUNDARIES<cmatrix> ERROR_CMATRIX_WRONG_BOUNDARIES;
typedef ERROR__NO_MORE_MEMORY<cmatrix> ERROR_CMATRIX_NO_MORE_MEMORY;
typedef ERROR__SUB_ARRAY_TOO_BIG<cmatrix> ERROR_CMATRIX_SUB_ARRAY_TOO_BIG;
typedef ERROR__ROW_OR_COL_NOT_IN_MAT<cmatrix> ERROR_CMATRIX_ROW_OR_COL_NOT_IN_MAT;
typedef ERROR__WRONG_ROW_OR_COL<cmatrix> ERROR_CMATRIX_WRONG_ROW_OR_COL;
typedef ERROR__WRONG_BOUNDARIES<ivector> ERROR_IVECTOR_WRONG_BOUNDARIES;
typedef ERROR__NO_MORE_MEMORY<ivector> ERROR_IVECTOR_NO_MORE_MEMORY;
typedef ERROR__SUB_ARRAY_TOO_BIG<ivector> ERROR_IVECTOR_SUB_ARRAY_TOO_BIG;
typedef ERROR__ELEMENT_NOT_IN_VEC<ivector> ERROR_IVECTOR_ELEMENT_NOT_IN_VEC;
typedef ERROR__TYPE_CAST_OF_THICK_OBJ<ivector> ERROR_IVECTOR_TYPE_CAST_OF_THICK_OBJ;
typedef ERROR__TYPE_CAST_OF_THICK_OBJ<imatrix> ERROR_IMATRIX_TYPE_CAST_OF_THICK_OBJ;
typedef ERROR__USE_OF_UNINITIALIZED_OBJ<ivector> ERROR_IVECTOR_USE_OF_UNINITIALIZED_OBJ;
typedef ERROR__USE_OF_UNINITIALIZED_OBJ<imatrix> ERROR_IMATRIX_USE_OF_UNINITIALIZED_OBJ;
typedef ERROR__OP_WITH_WRONG_DIM<ivector> ERROR_IVECTOR_OP_WITH_WRONG_DIM;
typedef ERROR__OP_WITH_WRONG_DIM<imatrix> ERROR_IMATRIX_OP_WITH_WRONG_DIM;
typedef ERROR__WRONG_BOUNDARIES<imatrix> ERROR_IMATRIX_WRONG_BOUNDARIES;
typedef ERROR__NO_MORE_MEMORY<imatrix> ERROR_IMATRIX_NO_MORE_MEMORY;
typedef ERROR__SUB_ARRAY_TOO_BIG<imatrix> ERROR_IMATRIX_SUB_ARRAY_TOO_BIG;
typedef ERROR__ROW_OR_COL_NOT_IN_MAT<imatrix> ERROR_IMATRIX_ROW_OR_COL_NOT_IN_MAT;
typedef ERROR__WRONG_ROW_OR_COL<imatrix> ERROR_IMATRIX_WRONG_ROW_OR_COL;
typedef ERROR__WRONG_BOUNDARIES<civector> ERROR_CIVECTOR_WRONG_BOUNDARIES;
typedef ERROR__NO_MORE_MEMORY<civector> ERROR_CIVECTOR_NO_MORE_MEMORY;
typedef ERROR__SUB_ARRAY_TOO_BIG<civector> ERROR_CIVECTOR_SUB_ARRAY_TOO_BIG;
typedef ERROR__ELEMENT_NOT_IN_VEC<civector> ERROR_CIVECTOR_ELEMENT_NOT_IN_VEC;
typedef ERROR__TYPE_CAST_OF_THICK_OBJ<civector> ERROR_CIVECTOR_TYPE_CAST_OF_THICK_OBJ;
typedef ERROR__TYPE_CAST_OF_THICK_OBJ<cimatrix> ERROR_CIMATRIX_TYPE_CAST_OF_THICK_OBJ;
typedef ERROR__USE_OF_UNINITIALIZED_OBJ<civector> ERROR_CIVECTOR_USE_OF_UNINITIALIZED_OBJ;
typedef ERROR__USE_OF_UNINITIALIZED_OBJ<cimatrix> ERROR_CIMATRIX_USE_OF_UNINITIALIZED_OBJ;
typedef ERROR__OP_WITH_WRONG_DIM<civector> ERROR_CIVECTOR_OP_WITH_WRONG_DIM;
typedef ERROR__OP_WITH_WRONG_DIM<cimatrix> ERROR_CIMATRIX_OP_WITH_WRONG_DIM;
typedef ERROR__WRONG_BOUNDARIES<cimatrix> ERROR_CIMATRIX_WRONG_BOUNDARIES;
typedef ERROR__NO_MORE_MEMORY<cimatrix> ERROR_CIMATRIX_NO_MORE_MEMORY;
typedef ERROR__SUB_ARRAY_TOO_BIG<cimatrix> ERROR_CIMATRIX_SUB_ARRAY_TOO_BIG;
typedef ERROR__ROW_OR_COL_NOT_IN_MAT<cimatrix> ERROR_CIMATRIX_ROW_OR_COL_NOT_IN_MAT;
typedef ERROR__WRONG_ROW_OR_COL<cimatrix> ERROR_CIMATRIX_WRONG_ROW_OR_COL;
typedef ERROR__WRONG_BOUNDARIES<intvector> ERROR_INTVECTOR_WRONG_BOUNDARIES;
typedef ERROR__NO_MORE_MEMORY<intvector> ERROR_INTVECTOR_NO_MORE_MEMORY;
typedef ERROR__SUB_ARRAY_TOO_BIG<intvector> ERROR_INTVECTOR_SUB_ARRAY_TOO_BIG;
typedef ERROR__ELEMENT_NOT_IN_VEC<intvector> ERROR_INTVECTOR_ELEMENT_NOT_IN_VEC;
typedef ERROR__TYPE_CAST_OF_THICK_OBJ<intvector> ERROR_INTVECTOR_TYPE_CAST_OF_THICK_OBJ;
typedef ERROR__TYPE_CAST_OF_THICK_OBJ<intmatrix> ERROR_INTMATRIX_TYPE_CAST_OF_THICK_OBJ;
typedef ERROR__USE_OF_UNINITIALIZED_OBJ<intvector> ERROR_INTVECTOR_USE_OF_UNINITIALIZED_OBJ;
typedef ERROR__USE_OF_UNINITIALIZED_OBJ<intmatrix> ERROR_INTMATRIX_USE_OF_UNINITIALIZED_OBJ;
typedef ERROR__OP_WITH_WRONG_DIM<intvector> ERROR_INTVECTOR_OP_WITH_WRONG_DIM;
typedef ERROR__OP_WITH_WRONG_DIM<intmatrix> ERROR_INTMATRIX_OP_WITH_WRONG_DIM;
typedef ERROR__WRONG_BOUNDARIES<intmatrix> ERROR_INTMATRIX_WRONG_BOUNDARIES;
typedef ERROR__NO_MORE_MEMORY<intmatrix> ERROR_INTMATRIX_NO_MORE_MEMORY;
typedef ERROR__SUB_ARRAY_TOO_BIG<intmatrix> ERROR_INTMATRIX_SUB_ARRAY_TOO_BIG;
typedef ERROR__ROW_OR_COL_NOT_IN_MAT<intmatrix> ERROR_INTMATRIX_ROW_OR_COL_NOT_IN_MAT;
typedef ERROR__WRONG_ROW_OR_COL<intmatrix> ERROR_INTMATRIX_WRONG_ROW_OR_COL;

typedef ERROR__WRONG_BOUNDARIES<l_rvector> ERROR_LRVECTOR_WRONG_BOUNDARIES;
typedef ERROR__NO_MORE_MEMORY<l_rvector> ERROR_LRVECTOR_NO_MORE_MEMORY;
typedef ERROR__SUB_ARRAY_TOO_BIG<l_rvector> ERROR_LRVECTOR_SUB_ARRAY_TOO_BIG;
typedef ERROR__ELEMENT_NOT_IN_VEC<l_rvector> ERROR_LRVECTOR_ELEMENT_NOT_IN_VEC;
typedef ERROR__TYPE_CAST_OF_THICK_OBJ<l_rvector> ERROR_LRVECTOR_TYPE_CAST_OF_THICK_OBJ;
typedef ERROR__TYPE_CAST_OF_THICK_OBJ<l_rmatrix> ERROR_LRMATRIX_TYPE_CAST_OF_THICK_OBJ;
typedef ERROR__USE_OF_UNINITIALIZED_OBJ<l_rvector> ERROR_LRVECTOR_USE_OF_UNINITIALIZED_OBJ;
typedef ERROR__USE_OF_UNINITIALIZED_OBJ<l_rmatrix> ERROR_LRMATRIX_USE_OF_UNINITIALIZED_OBJ;
typedef ERROR__OP_WITH_WRONG_DIM<l_rvector> ERROR_LRVECTOR_OP_WITH_WRONG_DIM;
typedef ERROR__OP_WITH_WRONG_DIM<l_rmatrix> ERROR_LRMATRIX_OP_WITH_WRONG_DIM;
typedef ERROR__WRONG_BOUNDARIES<l_rmatrix> ERROR_LRMATRIX_WRONG_BOUNDARIES;
typedef ERROR__NO_MORE_MEMORY<l_rmatrix> ERROR_LRMATRIX_NO_MORE_MEMORY;
typedef ERROR__SUB_ARRAY_TOO_BIG<l_rmatrix> ERROR_LRMATRIX_SUB_ARRAY_TOO_BIG;
typedef ERROR__ROW_OR_COL_NOT_IN_MAT<l_rmatrix> ERROR_LRMATRIX_ROW_OR_COL_NOT_IN_MAT;
typedef ERROR__WRONG_ROW_OR_COL<l_rmatrix> ERROR_LRMATRIX_WRONG_ROW_OR_COL;

typedef ERROR__WRONG_BOUNDARIES<l_ivector> ERROR_LIVECTOR_WRONG_BOUNDARIES;
typedef ERROR__NO_MORE_MEMORY<l_ivector> ERROR_LIVECTOR_NO_MORE_MEMORY;
typedef ERROR__SUB_ARRAY_TOO_BIG<l_ivector> ERROR_LIVECTOR_SUB_ARRAY_TOO_BIG;
typedef ERROR__ELEMENT_NOT_IN_VEC<l_ivector> ERROR_LIVECTOR_ELEMENT_NOT_IN_VEC;
typedef ERROR__TYPE_CAST_OF_THICK_OBJ<l_ivector> ERROR_LIVECTOR_TYPE_CAST_OF_THICK_OBJ;
typedef ERROR__TYPE_CAST_OF_THICK_OBJ<l_imatrix> ERROR_LIMATRIX_TYPE_CAST_OF_THICK_OBJ;
typedef ERROR__USE_OF_UNINITIALIZED_OBJ<l_ivector> ERROR_LIVECTOR_USE_OF_UNINITIALIZED_OBJ;
typedef ERROR__USE_OF_UNINITIALIZED_OBJ<l_imatrix> ERROR_LIMATRIX_USE_OF_UNINITIALIZED_OBJ;
typedef ERROR__OP_WITH_WRONG_DIM<l_ivector> ERROR_LIVECTOR_OP_WITH_WRONG_DIM;
typedef ERROR__OP_WITH_WRONG_DIM<l_imatrix> ERROR_LIMATRIX_OP_WITH_WRONG_DIM;
typedef ERROR__WRONG_BOUNDARIES<l_imatrix> ERROR_LIMATRIX_WRONG_BOUNDARIES;
typedef ERROR__NO_MORE_MEMORY<l_imatrix> ERROR_LIMATRIX_NO_MORE_MEMORY;
typedef ERROR__SUB_ARRAY_TOO_BIG<l_imatrix> ERROR_LIMATRIX_SUB_ARRAY_TOO_BIG;
typedef ERROR__ROW_OR_COL_NOT_IN_MAT<l_imatrix> ERROR_LIMATRIX_ROW_OR_COL_NOT_IN_MAT;
typedef ERROR__WRONG_ROW_OR_COL<l_imatrix> ERROR_LIMATRIX_WRONG_ROW_OR_COL;

#endif

// --------------------------------------------------------------------------

class ERROR_IDOTPRECISION_EMPTY_INTERVAL: virtual public ERROR_DOT, virtual public ERROR_INTERVAL, virtual public EMPTY_INTERVAL
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 2011; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR_IDOTPRECISION_EMPTY_INTERVAL"; }
		ERROR_IDOTPRECISION_EMPTY_INTERVAL() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR_IDOTPRECISION_EMPTY_INTERVAL(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR_IDOTPRECISION_EMPTY_INTERVAL() noexcept(false) { }
//	private:
//		string fkt;
};

class ERROR_CIDOTPRECISION_EMPTY_INTERVAL: virtual public ERROR_DOT, virtual public ERROR_CINTERVAL, virtual public EMPTY_INTERVAL
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 2011; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR_CIDOTPRECISION_EMPTY_INTERVAL"; }
		ERROR_CIDOTPRECISION_EMPTY_INTERVAL() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR_CIDOTPRECISION_EMPTY_INTERVAL(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR_CIDOTPRECISION_EMPTY_INTERVAL() noexcept(false) { }
//	private:
//		string fkt;
};

class ERROR_INTERVAL_EMPTY_INTERVAL: virtual public ERROR_INTERVAL, virtual public EMPTY_INTERVAL
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 4011; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR_INTERVAL_EMPTY_INTERVAL"; }
		ERROR_INTERVAL_EMPTY_INTERVAL() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR_INTERVAL_EMPTY_INTERVAL(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR_INTERVAL_EMPTY_INTERVAL() noexcept(false) { }
//	private:
//		string fkt;
};

class ERROR_CINTERVAL_EMPTY_INTERVAL: virtual public ERROR_CINTERVAL, virtual public EMPTY_INTERVAL
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 6011; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR_CINTERVAL_EMPTY_INTERVAL"; }
		ERROR_CINTERVAL_EMPTY_INTERVAL() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR_CINTERVAL_EMPTY_INTERVAL(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR_CINTERVAL_EMPTY_INTERVAL() noexcept(false) { }
//	private:
//		string fkt;
};


class ERROR_INTERVAL_STD_FKT_OUT_OF_DEF: virtual public ERROR_INTERVAL, virtual public STD_FKT_OUT_OF_DEF
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 4302; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR_INTERVAL_STD_FKT_OUT_OF_DEF"; }
		ERROR_INTERVAL_STD_FKT_OUT_OF_DEF() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR_INTERVAL_STD_FKT_OUT_OF_DEF(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR_INTERVAL_STD_FKT_OUT_OF_DEF() noexcept(false) { }
//	private:
//		string fkt;
};

class ERROR_LREAL_STD_FKT_OUT_OF_DEF: virtual public ERROR_LREAL, virtual public STD_FKT_OUT_OF_DEF
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 15302; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR_LREAL_STD_FKT_OUT_OF_DEF"; }
		ERROR_LREAL_STD_FKT_OUT_OF_DEF() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR_LREAL_STD_FKT_OUT_OF_DEF(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR_LREAL_STD_FKT_OUT_OF_DEF() noexcept(false) { }
//	private:
//		string fkt;
};

class ERROR_LINTERVAL_DIV_BY_ZERO: virtual public ERROR_LINTERVAL, virtual public DIV_BY_ZERO
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 16010; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR_LINTERVAL_DIV_BY_ZERO"; }
		ERROR_LINTERVAL_DIV_BY_ZERO() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR_LINTERVAL_DIV_BY_ZERO(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR_LINTERVAL_DIV_BY_ZERO() noexcept(false) { }
//	private:
//		string fkt;
};
 
class ERROR_LINTERVAL_EMPTY_INTERVAL: virtual public ERROR_LINTERVAL, virtual public EMPTY_INTERVAL
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 16011; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR_LINTERVAL_EMPTY_INTERVAL"; }
		ERROR_LINTERVAL_EMPTY_INTERVAL() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR_LINTERVAL_EMPTY_INTERVAL(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR_LINTERVAL_EMPTY_INTERVAL() noexcept(false) { }
//	private:
//		string fkt;
};

class ERROR_LINTERVAL_IN_EXACT_CH_OR_IS: virtual public ERROR_LINTERVAL, virtual public IN_EXACT_CH_OR_IS
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 16013; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR_LINTERVAL_IN_EXACT_CH_OR_IS"; }
		ERROR_LINTERVAL_IN_EXACT_CH_OR_IS() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR_LINTERVAL_IN_EXACT_CH_OR_IS(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR_LINTERVAL_IN_EXACT_CH_OR_IS() noexcept(false) { }
//	private:
//		string fkt;
};

class ERROR_LINTERVAL_WRONG_STAGPREC: virtual public ERROR_LINTERVAL, virtual public WRONG_STAGPREC
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 16300; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR_LINTERVAL_WRONG_STAGPREC"; }
		ERROR_LINTERVAL_WRONG_STAGPREC() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR_LINTERVAL_WRONG_STAGPREC(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR_LINTERVAL_WRONG_STAGPREC() noexcept(false) { }
//	private:
//		string fkt;
};

class ERROR_LINTERVAL_ELEMENT_NOT_IN_LONG: virtual public ERROR_LINTERVAL, virtual public ELEMENT_NOT_IN_LONG
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 16301; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR_LINTERVAL_ELEMENT_NOT_IN_LONG"; }
		ERROR_LINTERVAL_ELEMENT_NOT_IN_LONG() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR_LINTERVAL_ELEMENT_NOT_IN_LONG(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR_LINTERVAL_ELEMENT_NOT_IN_LONG() noexcept(false) { }
//	private:
//		string fkt;
};

class ERROR_LINTERVAL_STD_FKT_OUT_OF_DEF: virtual public ERROR_LINTERVAL, virtual public STD_FKT_OUT_OF_DEF
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 16302; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR_LINTERVAL_STD_FKT_OUT_OF_DEF"; }
		ERROR_LINTERVAL_STD_FKT_OUT_OF_DEF() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR_LINTERVAL_STD_FKT_OUT_OF_DEF(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR_LINTERVAL_STD_FKT_OUT_OF_DEF() noexcept(false) { }
//	private:
//		string fkt;
};

class ERROR_LINTERVAL_FAK_OVERFLOW: virtual public ERROR_LINTERVAL, virtual public FAK_OVERFLOW
{
	public:
		virtual int errnum() const noexcept(false)  ;// { return 16303; }
		virtual string errtext() const noexcept(false)  ;// { return fkt+": ERROR_LINTERVAL_FAK_OVERFLOW"; }
		ERROR_LINTERVAL_FAK_OVERFLOW() noexcept(false)  ;// { fkt="<unknown function>"; }
		ERROR_LINTERVAL_FAK_OVERFLOW(const string &f) noexcept(false)  ;// { fkt=f; }
//		virtual ~ERROR_LINTERVAL_FAK_OVERFLOW() noexcept(false) { }
//	private:
//		string fkt;
};

class REAL_NOT_ALLOWED: virtual public ERROR_ALL // Blomquist,26.01.08
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		REAL_NOT_ALLOWED() noexcept(false);
		REAL_NOT_ALLOWED(const string &f) noexcept(false);
};

class REAL_INT_OUT_OF_RANGE: virtual public ERROR_ALL // Blomquist,26.01.08
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		REAL_INT_OUT_OF_RANGE() noexcept(false);
		REAL_INT_OUT_OF_RANGE(const string &f) noexcept(false);
};

class NO_BRACKETS_IN_STRING: virtual public ERROR_ALL // Blomquist,26.01.08
{
	public:
		virtual int errnum() const noexcept(false);
		virtual string errtext() const noexcept(false);
		NO_BRACKETS_IN_STRING() noexcept(false);
		NO_BRACKETS_IN_STRING(const string &f) noexcept(false);
};


template <class T>
void cxscthrow(T e) noexcept(false)
{
   if(e.errnum()!=16013) // NERVIGE MELDUNG
      std::cerr << e.errtext() << std::endl;
   if(e.errnum()!=16013 // IN_EXACT_CH_OR_IS
   && e.errnum()!=16303 // FAK_OVERFLOW
   )
      throw e;
   // else continue
}

} // namespace cxsc 

#endif

