/**
@file   functional.hpp
@brief  提供了"function"這種可以儲存函式的物件
@author ToyAuthor
<pre>
function可以儲存一般函式也可以儲存成員函式
儲存成員函式時要搭配bind()使用

由於這東西的實現完全是我自己想的
效率提升空間應該還很大
http://github.com/ToyAuthor/functional
</pre>
*/


#ifndef _STD_FUNCTIONAL_HPP_
#define _STD_FUNCTIONAL_HPP_


// 判斷編譯器是否為C++11，是就改用標準庫的內容吧
#if __cplusplus > 201100L

// C++11標準庫的std::function
#include <functional>

#else


#include <bind.hpp>

namespace std{


namespace _functional{

/// function物件的核心所在< 函式回傳值的型態 , storage的種類 >
template<typename R, typename S>
struct core_base
{
	virtual ~core_base(){}
	virtual core_base* clone() const=0;             // 幫助function物件copy自己
	virtual R CallFunction(const S &s) const=0;     // 執行function內容
	virtual void SetObject(void *p){}               // 為了從外部輸入物件指標而設計的
};

/// 支援一般函式< return type , storage type , _functional pointer type >
template<typename R, typename S, typename F>
struct core_function : core_base< R, S >
{
	const F     _f;     // 一般函式的指標

	explicit core_function(const F &f):_f(f){}

	virtual core_base<R,S>* clone() const
	{
		return new core_function(_f);
	}

	R CallFunction(const S &s) const
	{
		return s.Do(type<R>(),_f);
	}
};

/// 偽裝成一般函式指標，但是內含物件指標，真正執行的是成員函式< return type , member function pointer , claas type >
template<typename R, typename F, typename C>
struct member_function
{
	explicit member_function(const member_function &other):pFunction(other.pFunction),pObject(other.pObject){}
	explicit member_function(const F &f):pFunction(f),pObject(0){}
	explicit member_function():pFunction(0),pObject(0){}

	const F     pFunction;      // 成員函式的指標
	C*          pObject;        // 物件指標

	inline void set_this(void *app)
	{
		pObject=static_cast<C*>(app);
	}

	inline R operator()() const
	{
		// 日後有必要在這裡檢查指標
		return (pObject->*pFunction)();
	}

	template<typename A1>
	inline R operator()(A1 a1) const
	{
		return (pObject->*pFunction)(a1);
	}

	template<typename A1, typename A2>
	inline R operator()(A1 a1,A2 a2) const
	{
		return (pObject->*pFunction)(a1,a2);
	}

	template<typename A1, typename A2, typename A3>
	inline R operator()(A1 a1,A2 a2,A3 a3) const
	{
		return (pObject->*pFunction)(a1,a2,a3);
	}

	template<typename A1, typename A2, typename A3, typename A4>
	inline R operator()(A1 a1,A2 a2,A3 a3,A4 a4) const
	{
		return (pObject->*pFunction)(a1,a2,a3,a4);
	}
};

/// 支援成員函式< 回傳值type, storage type , 真正有用的storage , member_function , 類別type >
template<typename R, typename S, typename F, typename C>
struct core_member_function : core_base< R, S >
{
	F       _f;     // 只能夠是member_function<>了，不能放其他的

	explicit core_member_function(const F &f):_f(f){}

	virtual core_base<R,S>* clone() const
	{
		return new core_member_function(_f);
	}
	R CallFunction(const S &s) const
	{
		return s.Do(type<R>(),_f);
	}
	void SetObject(void *app)
	{
		_f.set_this(app);
	}
};

/// 支援std::bind()< bind_t的種類 , storage的種類 >
template<typename T, typename S>
struct core_bind : core_base< typename T::result_type, S >
{
	typedef typename T::result_type result_type;

	const T     obj;        // 用來儲存bind_t物件

	explicit core_bind(const T &b):obj(b){}

	virtual core_base<result_type,S>* clone() const
	{
		return new core_bind(obj);
	}

	virtual result_type CallFunction(const S &s) const
	{
		return obj.eval(s);
	}
};

}//namespace _functional

/// function_base是下面各種function類別的共同基底，負責實現所有function都會需要的共同特徵< 函式回傳值的型態 , storage的種類 >
template<typename R, typename S> struct function_base
{
	function_base():pCore(0){}
	~function_base(){delete pCore;}

	_functional::core_base<R,S>     *pCore;     // 外部使用者不該修改此指標，該老慮要不要"protected"起來了

	// 用來輸入物件指標
	template<typename C>
	inline void set(C* c)
	{
		pCore->SetObject((void*)(c));
	}

	//----讓function物件可以像普通結構一樣的複製、傳遞----start

	function_base(const function_base &other)
	{
		if (other.pCore)
		{
			pCore = other.pCore->clone();
		}
		// 這邊沒有else的處理感覺比較危險
	}

	function_base& operator=(const function_base &other)
	{
		delete pCore;
		pCore = other.pCore ? other.pCore->clone() : other.pCore;
		return *this;
	}

	//----讓function物件可以像普通結構一樣的複製、傳遞----end
};


/// function的樣板原型，沒有用處，真正有用的是它的偏特化版本
template<typename S> struct function{};

//---------------------------function類別們---------------------------start

/// function的沒有參數版本
template<typename R>
struct function<R()> : function_base<R, storage0>
{
	typedef R (*Fn)();
	typedef storage0 St;
	using function_base<R,St>::pCore;       // 想令"pCore"屬性非public就不能用這招了

	function(){}        // function在宣告時可以不用賦予內容沒關係

	//---------------------支援一般函式---------------------start
	function(Fn f)
	{
		pCore = new _functional::core_function<R, St, Fn>(f);
	}
	function operator=(Fn f)
	{
		delete pCore;
		pCore = new _functional::core_function<R, St, Fn>(f);
		return *this;
	}
	//---------------------支援一般函式---------------------end

	//---------------------支援成員函式---------------------start
	template<typename C>
	function(R(C::*f)())
	{
		typedef _functional::member_function<R,R (C::*)(),C> F;
		pCore = new _functional::core_member_function<R, St, F, C>(F(f));
	}
	template<typename C>
	function(R(C::*f)(),C* c)
	{
		typedef _functional::member_function<R,R (C::*)(),C> F;
		pCore = new _functional::core_member_function<R, St, F, C>(F(f));
		pCore->SetObject((void*)(c));
	}
	template<typename C>
	function operator=(R(C::*f)())
	{
		typedef _functional::member_function<R,R (C::*)(),C> F;
		delete pCore;
		pCore = new _functional::core_member_function<R, St, F, C>(F(f));
		return *this;
	}
	//---------------------支援成員函式---------------------end

	//---------------------支援bind()---------------------start
	template<typename A,typename B,typename C>
	function(const bind_t<A,B,C> &b)
	{
		pCore = new _functional::core_bind<bind_t<A,B,C>, St >(b);
	}
	template<typename A,typename B,typename C>
	function operator=(const bind_t<A,B,C> &b)
	{
		delete pCore;
		pCore = new _functional::core_bind<bind_t<A,B,C>, St >(b);
		return *this;
	}
	//---------------------支援bind()---------------------end

	// 執行core_base裡暗藏的function，
	R operator()() const
	{
		return pCore->CallFunction(St());
	}
};

/// function的一個參數版本
template<typename R, typename P1>
struct function<R(P1)> : function_base<R, storage1<P1> >
{
	typedef R (*Fn)(P1);
	typedef storage1<P1> St;
	using function_base<R,St>::pCore;

	function(){}

	//---------------------支援一般函式---------------------start
	function(Fn f)
	{
		pCore = new _functional::core_function<R, St, Fn>(f);
	}
	function operator=(Fn f)
	{
		delete pCore;
		pCore = new _functional::core_function<R, St, Fn>(f);
		return *this;
	}
	//---------------------支援一般函式---------------------end

	//---------------------支援成員函式---------------------start
	template<typename C>
	function(R(C::*f)(P1))
	{
		typedef _functional::member_function<R,R (C::*)(P1),C> F;
		pCore = new _functional::core_member_function<R, St, F, C>(F(f));
	}
	template<typename C>
	function(R(C::*f)(P1),C* c)
	{
		typedef _functional::member_function<R,R (C::*)(P1),C> F;
		pCore = new _functional::core_member_function<R, St, F, C>(F(f));
		pCore->SetObject((void*)(c));
	}
	template<typename C>
	function operator=(R(C::*f)(P1))
	{
		typedef _functional::member_function<R,R (C::*)(P1),C> F;
		delete pCore;
		pCore = new _functional::core_member_function<R, St, F, C>(F(f));
		return *this;
	}
	//---------------------支援成員函式---------------------end

	//---------------------支援bind()---------------------start
	template<typename A,typename B,typename C>
	function(const bind_t<A,B,C> &b)
	{
		pCore = new _functional::core_bind<bind_t<A,B,C>, St >(b);
	}
	template<typename A,typename B,typename C>
	function operator=(const bind_t<A,B,C> &b)
	{
		delete pCore;
		pCore = new _functional::core_bind<bind_t<A,B,C>, St >(b);
		return *this;
	}
	//---------------------支援bind()---------------------end

	R operator()(P1 p1) const
	{
		return pCore->CallFunction(St(p1));
	}
};

/// function的兩個參數版本
template<typename R, typename P1, typename P2>
struct function<R(P1, P2)> : function_base<R, storage2<P1, P2> >
{
	typedef R (*Fn)(P1,P2);
	typedef storage2<P1,P2> St;
	using function_base<R,St>::pCore;

	function(){}
	function(Fn f)
	{
		pCore = new _functional::core_function<R, St, Fn>(f);
	}
	function operator=(Fn f)
	{
		delete pCore;
		pCore = new _functional::core_function<R, St, Fn>(f);
		return *this;
	}

	template<typename C>
	function(R(C::*f)(P1,P2))
	{
		typedef _functional::member_function<R,R (C::*)(P1,P2),C> F;
		pCore = new _functional::core_member_function<R, St, F, C>(F(f));
	}
	template<typename C>
	function(R(C::*f)(P1,P2),C* c)
	{
		typedef _functional::member_function<R,R (C::*)(P1,P2),C> F;
		pCore = new _functional::core_member_function<R, St, F, C>(F(f));
		pCore->SetObject((void*)(c));
	}
	template<typename C>
	function operator=(R(C::*f)(P1,P2))
	{
		typedef _functional::member_function<R,R (C::*)(P1,P2),C> F;
		delete pCore;
		pCore = new _functional::core_member_function<R, St, F, C>(F(f));
		return *this;
	}

	template<typename A,typename B,typename C>
	function(const bind_t<A,B,C> &b)
	{
		pCore = new _functional::core_bind<bind_t<A,B,C>, St >(b);
	}
	template<typename A,typename B,typename C>
	function operator=(const bind_t<A,B,C> &b)
	{
		delete pCore;
		pCore = new _functional::core_bind<bind_t<A,B,C>, St >(b);
		return *this;
	}

	R operator()(P1 p1, P2 p2) const
	{
		return pCore->CallFunction(St(p1, p2));
	}
};

/// function的三個參數版本
template<typename R, typename P1, typename P2, typename P3>
struct function<R(P1, P2, P3)> : function_base<R, storage3<P1, P2, P3> >
{
	typedef R (*Fn)(P1,P2,P3);
	typedef storage3<P1,P2,P3> St;
	using function_base<R,St>::pCore;

	function(){}
	function(Fn f)
	{
		pCore = new _functional::core_function<R, St, Fn>(f);
	}
	function operator=(Fn f)
	{
		delete pCore;
		pCore = new _functional::core_function<R, St, Fn>(f);
		return *this;
	}

	template<typename C>
	function(R(C::*f)(P1,P2,P3))
	{
		typedef _functional::member_function<R,R (C::*)(P1,P2,P3),C> F;
		pCore = new _functional::core_member_function<R, St, F, C>(F(f));
	}
	template<typename C>
	function(R(C::*f)(P1,P2,P3),C* c)
	{
		typedef _functional::member_function<R,R (C::*)(P1,P2,P3),C> F;
		pCore = new _functional::core_member_function<R, St, F, C>(F(f));
		pCore->SetObject((void*)(c));
	}
	template<typename C>
	function operator=(R(C::*f)(P1,P2,P3))
	{
		typedef _functional::member_function<R,R (C::*)(P1,P2,P3),C> F;
		delete pCore;
		pCore = new _functional::core_member_function<R, St, F, C>(F(f));
		return *this;
	}

	template<typename A,typename B,typename C>
	function(const bind_t<A,B,C> &b)
	{
		pCore = new _functional::core_bind<bind_t<A,B,C>, St >(b);
	}
	template<typename A,typename B,typename C>
	function operator=(const bind_t<A,B,C> &b)
	{
		delete pCore;
		pCore = new _functional::core_bind<bind_t<A,B,C>, St >(b);
		return *this;
	}

	R operator()(P1 p1, P2 p2, P3 p3) const
	{
		return pCore->CallFunction(St(p1, p2, p3));
	}
};

/// function的四個參數版本
template<typename R, typename P1, typename P2, typename P3, typename P4>
struct function<R(P1, P2, P3, P4)> : function_base<R, storage4<P1, P2, P3, P4> >
{
	typedef R (*Fn)(P1,P2,P3,P4);
	typedef storage4<P1,P2,P3,P4> St;
	using function_base<R,St>::pCore;

	function(){}
	function(Fn f)
	{
		pCore = new _functional::core_function<R, St, Fn>(f);
	}
	function operator=(Fn f)
	{
		delete pCore;
		pCore = new _functional::core_function<R, St, Fn>(f);
		return *this;
	}

	template<typename C>
	function(R(C::*f)(P1,P2,P3,P4))
	{
		typedef _functional::member_function<R,R (C::*)(P1,P2,P3,P4),C> F;
		pCore = new _functional::core_member_function<R, St, F, C>(F(f));
	}
	template<typename C>
	function(R(C::*f)(P1,P2,P3,P4),C* c)
	{
		typedef _functional::member_function<R,R (C::*)(P1,P2,P3,P4),C> F;
		pCore = new _functional::core_member_function<R, St, F, C>(F(f));
		pCore->SetObject((void*)(c));
	}
	template<typename C>
	function operator=(R(C::*f)(P1,P2,P3,P4))
	{
		typedef _functional::member_function<R,R (C::*)(P1,P2,P3,P4),C> F;
		delete pCore;
		pCore = new _functional::core_member_function<R, St, F, C>(F(f));
		return *this;
	}

	template<typename A,typename B,typename C>
	function(const bind_t<A,B,C> &b)
	{
		pCore = new _functional::core_bind<bind_t<A,B,C>, St >(b);
	}
	template<typename A,typename B,typename C>
	function operator=(const bind_t<A,B,C> &b)
	{
		delete pCore;
		pCore = new _functional::core_bind<bind_t<A,B,C>, St >(b);
		return *this;
	}

	R operator()(P1 p1, P2 p2, P3 p3, P4 p4) const
	{
		return pCore->CallFunction(St(p1, p2, p3, p4));
	}
};

//---------------------------function類別們---------------------------end

}//namespace std


#endif//__cplusplus > 201100L

#endif//_STD_FUNCTIONAL_HPP_
