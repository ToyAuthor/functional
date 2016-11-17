/**
 * @file      bind.hpp
 * @brief     提供了 std::bind 這個實用的函式綁定工具
 * @author    ToyAuthor
 * @copyright Public Domain
 * <pre>
 * 通常能用 bind 取代繼承的場合就別用繼承了
 *
 * bind的實作方式用到許多樣版手法
 * 假如你平時很少使用樣版的話
 * 建議不要去研究 bind 了
 * 而且 std::bind 在 C++11 之後大概會漸漸被更強大的匿名函式取代
 *
 * 厲害的樣板技巧基本上都有人打造成方便的工具供你使用了
 * 會用就可以了
 * 不用搞懂內容
 *
 * http://github.com/ToyAuthor/functional
 * </pre>
 */


#ifndef _STD_BIND_HPP_
#define _STD_BIND_HPP_

// 判斷編譯器是否為C++11，是就改用標準庫吧
#if __cplusplus > 201100L

#include <functional>

#else

namespace std{


// 將一個type裝成變數來傳遞，用處是藉變數將type傳達進去
template<typename T> struct type{ typedef T value; };

// 這個結構的唯一用途就是可透過數字來辨識自己的type
template<int I> struct Argc{};

// 可將指標包裝的像個參考，用來應付一些你不能丟真正參考的情況
template<typename T>
struct reference_wrapper
{
	public:
		typedef T type;

		explicit reference_wrapper(T &t): t_(&t) {}

		operator T& ()   const { return *t_; }
		T& get()         const { return *t_; }
		T* get_pointer() const { return  t_; }

	private:
		T* t_;
};

//-------------不管輸入的是指標還是參考都會回傳成指標-------------
template<typename T> inline T* get_pointer(T *o){ return o; }
template<typename T> inline T* get_pointer(T &o){ return &o; }


//------------------實現param_traits------------------start

// 不管對象是不是參考都化為原本type
template<typename T> struct untie_ref{ typedef T type; };
template<typename T> struct untie_ref<T&> : untie_ref<T>{};

// 不管對象是不是參考都化為原本type的參考
template<typename T> struct param_traits
{
	typedef typename untie_ref<T>::type& type;
};

//------------------實現param_traits------------------end


//------------------實現result_traits------------------start

// 面對一般type直接取名為type
template<typename R> struct result_traits{ typedef R type; };

// 面對用type<>包裝過的type就取其中的result_type為type
template<typename F> struct result_traits<type<F> > : result_traits<typename F::result_type> {};

// 面對用type<>包裝過的type就取其中的result_type為type，為了支援reference_wrapper而寫的版本
template<typename F>
struct result_traits<type<reference_wrapper<F> > > : result_traits<typename F::result_type> {};

//------------------實現result_traits------------------end


// 替對象創造一個reference_wrapper，針對無法拷貝或者拷貝耗時的物件很有用，用法bind(&function,ref(obj))
template<typename T>
inline reference_wrapper<T> ref(T &t)
{
	return reference_wrapper<T>(t);
}

// const版的 ref()
template<typename T>
inline reference_wrapper<T const> cref(const T &t)
{
	return reference_wrapper<const T>(t);
}


/**
由於是inline函式
所以這裡的函式名稱可以視為附帶編號的macro
藉此實現bind()的placeholder
*/
namespace placeholders{

static inline Argc<1> _1() { return std::Argc<1>(); }
static inline Argc<2> _2() { return std::Argc<2>(); }
static inline Argc<3> _3() { return std::Argc<3>(); }
static inline Argc<4> _4() { return std::Argc<4>(); }

}

//-------------------------------------storage系列-------------------------------------start

// storage物件負責儲存呼叫function所需的參數們

// 所有storage系列的最底層基底類別
struct storage_base
{
	// T並非"Argc<1>(*)()"才會執行本method將這參數傳回
	// 另一個版本負責接收"Argc<1>(*)()"然後傳回自己攜帶的參數
	template<typename T>
	inline T& operator[](T &v) const {return v;}

	template<typename T>
	inline const T& operator[](const T &v) const{return v;}

	// 一樣將參考傳回去，只是接收的不是本尊而是用 reference_wrapper 做出來的假參考
	template<typename T>
	inline T& operator[](reference_wrapper<T> &v) const {return v.get();}

	template<typename T>
	inline T& operator[](const reference_wrapper<T> &v) const {return v.get();}
};

//-----------------------------零個參數的storage-----------------------------start
struct storage0 : storage_base
{
	using storage_base::operator[];

	// 沒有參數的情況下自然不需要那些花招，直接呼叫函式就好 < return type , function , storage >
	template<typename R, typename F, typename S>
	inline R operator()(type<R>, F &f, const S&) const
	{
		return f();
	}

	// 得到外部輸入的函式位址並呼叫函式(不知為何，這method不能用operator()多載，明明沒有跟誰衝突啊)
	template<typename R, typename F>
	inline R Do(type<R>, F &f) const
	{
		return f();
	}
};
//-----------------------------零個參數的storage-----------------------------end

//-----------------------------一個參數的storage-----------------------------start

// D是後面衍生的storage物件
template<typename D>
struct storage1_base : storage0
{
	typedef storage0 base;

	using base::operator[];

	// 接收外部的storage然後將裡面附帶的參數跟bind绑定的參數湊成完整的參數群
	template<typename R, typename F, typename S>
	inline R operator()(type<R>, F &f, const S &s) const
	{
		const D &d = static_cast<const D&>(*this);  // 因為樣板的關係，所以才有機會取得衍生類別的成員
		return d[f](s[d.a1_]);                      // 用多載[]運算子去決定要使用該參數還是用自己的參數(placeholder的情況)
	}
};

// 此物件可儲存一個參數
template<typename A1>
struct storage1 : storage1_base<storage1<A1> >
{
	typedef storage1_base<storage1<A1> > base;
	typedef typename param_traits<A1>::type P1;
	typedef typename result_traits<A1>::type result_type;

	storage1(P1 p1) : a1_(p1){}     // 將綁定的參數儲存起來

	// 丟進[]的外部參數若非"Argc<1>(*)()"類型則直接回傳外部參數，自己帶的成員參數"a1_"則沒有用到
	using base::operator[];

	// 丟進[]的外部參數若是Argc這型的函式指標時，會回傳本物件自己帶的成員參數"a1_"
	inline result_type operator[](Argc<1>(*)()) const { return a1_; }

	// 得到外部輸入的函式位址並呼叫函式
	template<typename R, typename F>
	inline R Do(type<R>, F &f) const
	{
		return f(a1_);
	}

	A1 a1_;     // 此物件儲存的參數
};

// 這個是上面storage1的空殼版本，當bind()的參數欄位被填了placeholder就會選擇此版本
template<int I>
struct storage1<Argc<I>(*)()> : storage1_base<storage1<Argc<I>(*)()> >
{
	typedef typename param_traits<Argc<I>(*)()>::type P1;

	storage1(P1){}          // 建構子允許丟參數進來，但是根本不會去用，因為那只是個placeholder

	Argc<I> (*a1_)(void);   // 這指標不需要指向何處，重點在於type，Argc<I>的"I"會讓編譯器去找到相呼應的operator[]
};
//-----------------------------一個參數的storage-----------------------------end

//-----------------------------兩個參數的storage-----------------------------start
template<typename D, typename A1>
struct storage2_base : storage1<A1>
{
	typedef storage1<A1> base;
	using base::operator[];

	storage2_base(typename base::P1 p1) : base(p1){}

	template<typename R, typename F, typename S>
	inline R operator()(type<R>, F &f, const S &s) const
	{
		const D &d = static_cast<const D&>(*this);
		return d[f](s[d.a1_], s[d.a2_]);
	}
};
template<typename A1, typename A2>
struct storage2 : storage2_base<storage2<A1, A2>, A1>
{
	typedef storage2_base<storage2<A1, A2>, A1> base;
	typedef typename param_traits<A2>::type P2;
	typedef typename result_traits<A2>::type result_type;

	storage2(typename base::P1 p1, P2 p2) : base(p1), a2_(p2) {}

	using base::operator[];
	inline result_type operator[](Argc<2> (*)()) const { return a2_; }

	template<typename R, typename F>
	inline R Do(type<R>, F &f) const {return f(this->a1_,a2_);}

	A2 a2_;
};
template<typename A1, int I>
struct storage2<A1, Argc<I>(*)()> : storage2_base<storage2<A1, Argc<I>(*)()>, A1>
{
	typedef storage2_base<storage2<A1, Argc<I>(*)()>, A1> base;
	typedef typename param_traits<Argc<I>(*)()>::type P2;

	storage2(typename base::P1 p1, P2) : base(p1){}

	Argc<I> (*a2_)(void);
};
//-----------------------------兩個參數的storage-----------------------------end

//-----------------------------三個參數的storage-----------------------------start
template<typename D, typename A1, typename A2>
struct storage3_base : storage2<A1, A2>
{
	typedef storage2<A1, A2> base;
	using base::operator[];

	storage3_base(typename base::P1 p1, typename base::P2 p2) : base(p1, p2) {}

	template<typename R, typename F, typename S>
	inline R operator()(type<R>, F &f, const S &s) const
	{
		const D &d = static_cast<const D&>(*this);
		return d[f](s[d.a1_], s[d.a2_], s[d.a3_]);
	}
};
template<typename A1, typename A2, typename A3>
struct storage3 : storage3_base<storage3<A1, A2, A3>, A1, A2>
{
	typedef storage3_base<storage3<A1, A2, A3>, A1, A2> base;
	typedef typename param_traits<A3>::type P3;
	typedef typename result_traits<A3>::type result_type;

	storage3(typename base::P1 p1, typename base::P2 p2, P3 p3) : base(p1, p2), a3_(p3) {}

	using base::operator[];
	inline result_type operator[](Argc<3> (*)()) const { return a3_; }

	template<typename R, typename F>
	inline R Do(type<R>, F &f) const {return f(this->a1_,this->a2_,a3_);}

	A3 a3_;
};
template<typename A1, typename A2, int I>
struct storage3<A1, A2, Argc<I>(*)()> : storage3_base<storage3<A1, A2, Argc<I>(*)()>, A1, A2>
{
	typedef storage3_base<storage3<A1, A2, Argc<I>(*)()>, A1, A2> base;
	typedef typename param_traits<Argc<I>(*)()>::type P3;

	storage3(typename base::P1 p1, typename base::P2 p2, P3) : base(p1, p2) {}
	Argc<I> (*a3_)(void);
};
//-----------------------------三個參數的storage-----------------------------end

//-----------------------------四個參數的storage-----------------------------start
template<typename D, typename A1, typename A2, typename A3>
struct storage4_base : storage3<A1, A2, A3>
{
	typedef storage3<A1, A2, A3> base;
	using base::operator[];

	storage4_base(typename base::P1 p1, typename base::P2 p2, typename base::P3 p3) : base(p1, p2, p3){}

	template<typename R, typename F, typename S>
	inline R operator()(type<R>, F &f, const S &s) const
	{
		const D &d = static_cast<const D&>(*this);
		return d[f](s[d.a1_], s[d.a2_], s[d.a3_], s[d.a4_]);
	}
};
template<typename A1, typename A2, typename A3, typename A4>
struct storage4 : storage4_base<storage4<A1, A2, A3, A4>, A1, A2, A3>
{
	typedef storage4_base<storage4<A1, A2, A3, A4>, A1, A2, A3> base;
	typedef typename param_traits<A4>::type P4;
	typedef typename result_traits<A4>::type result_type;

	using base::operator[];

	storage4(typename base::P1 p1, typename base::P2 p2, typename base::P3 p3, P4 p4) : base(p1, p2, p3), a4_(p4) {}

	inline result_type operator[](Argc<4>(*)()) const { return a4_; }

	template<typename R, typename F>
	inline R Do(type<R>, F &f) const {return f(this->a1_,this->a2_,this->a3_,a4_);}

	A4 a4_;
};
template<typename A1, typename A2, typename A3, int I>
struct storage4<A1, A2, A3, Argc<I>(*)()> : storage4_base<storage4<A1, A2, A3, Argc<I>(*)()>, A1, A2, A3>
{
	typedef storage4_base<storage4<A1, A2, A3, Argc<I>(*)()>, A1, A2, A3> base;
	typedef typename param_traits<Argc<I>(*)()>::type P4;

	storage4(typename base::P1 p1, typename base::P2 p2, typename base::P3 p3, P4) : base(p1, p2, p3) {}
	Argc<I> (*a4_)(void);
};
//-----------------------------四個參數的storage-----------------------------end

//-------------------------------------storage系列-------------------------------------end



namespace _bind{

//-------------------------------------_bind::f_*()系列-------------------------------------start
// "f_*()"是用來儲存成員函式的
// 當運行"()"運算子時會將填入的第一個參數視為物件指標
// 用"f_*()"來取代原本的一般函式指標就令Bind()變成可以支援成員函式了


// f_*()系列的共同基底
template<typename R, typename F>
struct f_b
{
	typedef typename result_traits<R>::type result_type;
	explicit f_b(const F &f) : f_(f) {}
	F f_;           //成員函式的位址
};

// 沒有參數的成員函式< 函式回傳值 , 類別type , 成員函式的原型 >
template<typename R, typename C, typename F>
struct f_0 : f_b<R, F>
{
	typedef f_b<R, F> base;

	explicit f_0(const F &f) : base(f) {}

	// 填入的物件指標type不符合時就用這個執行，之所以不符合也許是因為該物件指標是屬於基底類別的吧
	template<typename U>
	inline typename base::result_type operator()(U &u) const
	{
		return (get_pointer(u)->*base::f_)();
	}

	// 填入的物件指標type符合時就用這個執行
	inline typename base::result_type operator()(C &c) const
	{
		return (c.*base::f_)();
	}
};
// 一個參數的成員函式
template<typename R, typename C, typename F>
struct f_1 : f_b<R, F>
{
	typedef f_b<R, F> base;

	explicit f_1(const F &f) : base(f) {}

	template<typename U, typename A1>
	inline typename base::result_type operator()(U &u, A1 a1) const
	{
		return (get_pointer(u)->*base::f_)(a1);
	}

	template<typename A1>
	inline typename base::result_type operator()(C &c, A1 a1) const
	{
		return (c.*base::f_)(a1);
	}
};
// 兩個參數的成員函式
template<typename R, typename C, typename F>
struct f_2 : f_b<R, F>
{
	typedef f_b<R, F> base;

	explicit f_2(const F &f) : base(f) {}

	template<typename U, typename A1, typename A2>
	inline typename base::result_type operator()(U &u, A1 a1, A2 a2) const
	{
		return (get_pointer(u)->*base::f_)(a1, a2);
	}

	template<typename A1, typename A2>
	inline typename base::result_type operator()(C &c, A1 a1, A2 a2) const
	{
		return (c.*base::f_)(a1, a2);
	}
};
// 三個參數的成員函式
template<typename R, typename C, typename F>
struct f_3 : f_b<R, F>
{
	typedef f_b<R, F> base;

	explicit f_3(const F &f) : base(f) {}

	template<typename U, typename A1, typename A2, typename A3>
	inline typename base::result_type operator()(U &u, A1 a1, A2 a2, A3 a3) const
	{
		return (get_pointer(u)->*base::f_)(a1, a2, a3);
	}

	template<typename A1, typename A2, typename A3>
	inline typename base::result_type operator()(C &c, A1 a1, A2 a2, A3 a3) const
	{
		return (c.*base::f_)(a1, a2, a3);
	}
};

//-------------------------------------_bind::f_*()系列-------------------------------------end

}//namespace _bind

/// 這個是bind()的回傳值
template<typename R, typename F, typename S> struct bind_t
{
	public:

		// result_traits對R做了解析並確保能得到真正的回傳值型態
		typedef typename result_traits<R>::type result_type;

		bind_t(const F &f, const S &s) : f_(f), s_(s) {}


		//----------------------eval----------------------start

		// eval使得外部可以只輸入storage物件來呼叫執行function
		// 方便外部設計出簡潔的介面

		template<typename A>
		inline result_type eval(A &a)
		{
			return s_(type<result_type>(), f_, a);
		}
		template<typename A>
		inline result_type eval(A &a) const
		{
			return s_(type<result_type>(), f_, a);
		}

		//----------------------eval----------------------end

		//-------------------輸入零個參數時的情況-------------------start

		inline result_type operator()()
		{
			typedef storage0 ll;
			return s_(type<result_type>(), f_, ll());
		}
		inline result_type operator()() const
		{
			typedef storage0 ll;
			return s_(type<result_type>(), f_, ll());
		}
		//-------------------輸入零個參數時的情況-------------------end

		//-------------------輸入一個參數時的情況-------------------start
		// 為了支持const而多寫了很多版本(2^2個)

		template<typename P1>
		inline result_type operator()(P1 &p1)
		{
			typedef storage1<P1&> ll;
			return l_(type<result_type>(), f_, ll(p1));
		}
		template<typename P1>
		inline result_type operator()(P1 &p1) const
		{
			typedef storage1<P1&> ll;
			return l_(type<result_type>(), f_, ll(p1));
		}
		template<typename P1>
		inline result_type operator()(const P1 &p1)
		{
			typedef storage1<const P1&> ll;
			return l_(type<result_type>(), f_, ll(p1));
		}
		template<typename P1>
		inline result_type operator()(const P1 &p1) const//--------這個足以應付大多數情況了
		{
			typedef storage1<const P1&> ll;
			return l_(type<result_type>(), f_, ll(p1));
		}
		//-------------------輸入一個參數時的情況-------------------end

		//--------以下省略只寫最通用的版本

		// 輸入兩個參數時
		template<typename P1, typename P2>
		inline result_type operator()(const P1 &p1, const P2 &p2) const
		{
			typedef storage2<const P1&, const P2&> ll;
			return s_(type<result_type>(), f_, ll(p1, p2));
		}
		// 輸入三個參數時
		template<typename P1, typename P2, typename P3>
		inline result_type operator()(const P1 &p1, const P2 &p2, const P3 &p3) const
		{
			typedef storage3<const P1&, const P2&, const P3&> ll;
			return s_(type<result_type>(), f_, ll(p1, p2, p3));
		}
		// 輸入四個參數時
		template<typename P1, typename P2, typename P3, typename P4>
		inline result_type operator()(const P1 &p1, const P2 &p2, const P3 &p3, const P4 &p4) const
		{
			typedef storage4<const P1&, const P2&, const P3&, const P4&> ll;
			return s_(type<result_type>(), f_, ll(p1, p2, p3, p4));
		}

	private:

		F f_;
		S s_;
};

//----------------------------處理一般函式----------------------------start

template<typename R>
bind_t<R, R (*)(), storage0> bind(R (*f)())
{
	typedef R (*F)();
	typedef storage0 S;
	return bind_t<R, F, S>(f, S());
};
template<typename R, typename P1, typename A1>
bind_t<R, R (*)(P1), storage1<A1> > bind(R (*f)(P1), A1 a1)
{
	typedef R (*F)(P1);
	typedef storage1<A1> S;
	return bind_t<R, F, S>(f, S(a1));
}
template<typename R, typename P1, typename P2, typename A1, typename A2>
bind_t<R, R (*)(P1, P2), storage2<A1, A2> > bind(R (*f)(P1, P2), A1 a1, A2 a2)
{
	typedef R (*F)(P1, P2);
	typedef storage2<A1, A2> S;
	return bind_t<R, F, S>(f, S(a1, a2));
}
template<typename R, typename P1, typename P2, typename P3, typename A1, typename A2, typename A3>
bind_t<R, R (*)(P1, P2, P3), storage3<A1, A2, A3> > bind(R (*f)(P1, P2, P3), A1 a1, A2 a2, A3 a3)
{
	typedef R (*F)(P1, P2, P3);
	typedef storage3<A1, A2, A3> S;
	return bind_t<R, F, S>(f, S(a1, a2, a3));
}
template<typename R, typename P1, typename P2, typename P3, typename P4, typename A1, typename A2, typename A3, typename A4>
bind_t<R, R (*)(P1, P2, P3, P4), storage4<A1, A2, A3, A4> > bind(R (*f)(P1, P2, P3, P4), A1 a1, A2 a2, A3 a3, A4 a4)
{
	typedef R (*F)(P1, P2, P3, P4);
	typedef storage4<A1, A2, A3, A4> S;
	return bind_t<R, F, S>(f, S(a1, a2, a3, a4));
}

//----------------------------處理一般函式----------------------------end

//----------------------------針對成員函式----------------------------start

template<typename R, typename C, typename C1>
bind_t<R, _bind::f_0<R, C, R (C::*)()>, storage1<C1> > bind(R (C::*f)(), C1 c1)
{
	typedef _bind::f_0<R, C, R (C::*)()> F;
	typedef storage1<C1> S;
	return bind_t<R, F, S>(F(f), S(c1));
}
template<typename R, typename C, typename C1>
bind_t<R, _bind::f_0<R, C, R (C::*)() const>, storage1<C1> > bind(R (C::*f)() const, C1 c1)
{
	typedef _bind::f_0<R, C, R (C::*)() const> F;
	typedef storage1<C1> S;
	return bind_t<R, F, S>(F(f), S(c1));
}
template<typename R, typename C, typename A1, typename C1, typename P1>
bind_t<R, _bind::f_1<R, C, R (C::*)(A1)>, storage2<C1, P1> > bind(R (C::*f)(A1), C1 c1, P1 p2)
{
	typedef _bind::f_1<R, C, R (C::*)(A1)> F;
	typedef storage2<C1, P1> S;
	return bind_t<R, F, S>(F(f), S(c1, p2));
}
template<typename R, typename C, typename A1, typename C1, typename P1>
bind_t<R, _bind::f_1<R, C, R (C::*)(A1)>, storage2<C1, P1> > bind(R (C::*f)(A1) const, C1 c1, P1 p2)
{
	typedef _bind::f_1<R, C, R (C::*)(A1) const> F;
	typedef storage2<C1, P1> S;
	return bind_t<R, F, S>(F(f), S(c1, p2));
}
template<typename R, typename C, typename A1, typename A2, typename C1, typename P1, typename P2>
bind_t<R, _bind::f_2<R, C, R (C::*)(A1,A2)>, storage3<C1, P1, P2> > bind(R (C::*f)(A1,A2), C1 c1, P1 p1, P2 p2)
{
	typedef _bind::f_2<R, C, R (C::*)(A1,A2)> F;
	typedef storage3<C1, P1, P2> S;
	return bind_t<R, F, S>(F(f), S(c1, p1, p2));
}
template<typename R, typename C, typename A1, typename A2, typename C1, typename P1, typename P2>
bind_t<R, _bind::f_2<R, C, R (C::*)(A1,A2) const>, storage3<C1, P1, P2> > bind(R (C::*f)(A1,A2) const, C1 c1, P1 p1, P2 p2)
{
	typedef _bind::f_2<R, C, R (C::*)(A1,A2) const> F;
	typedef storage3<C1, P1, P2> S;
	return bind_t<R, F, S>(F(f), S(c1, p1, p2));
}
template<typename R, typename C, typename A1, typename A2, typename A3, typename C1, typename P1, typename P2, typename P3>
bind_t<R, _bind::f_3<R, C, R (C::*)(A1,A2,A3)>, storage4<C1, P1, P2, P3> > bind(R (C::*f)(A1,A2,A3), C1 c1, P1 p1, P2 p2, P3 p3)
{
	typedef _bind::f_3<R, C, R (C::*)(A1,A2,A3)> F;
	typedef storage4<C1, P1, P2, P3> S;
	return bind_t<R, F, S>(F(f), S(c1, p1, p2, p3));
}
template<typename R, typename C, typename A1, typename A2, typename A3, typename C1, typename P1, typename P2, typename P3>
bind_t<R, _bind::f_3<R, C, R (C::*)(A1,A2,A3) const>, storage4<C1, P1, P2, P3> > bind(R (C::*f)(A1,A2,A3) const, C1 c1, P1 p1, P2 p2, P3 p3)
{
	typedef _bind::f_3<R, C, R (C::*)(A1,A2,A3) const> F;
	typedef storage4<C1, P1, P2, P3> S;
	return bind_t<R, F, S>(F(f), S(c1, p1, p2, p3));
}

//----------------------------針對成員函式----------------------------end

}//namespace std



#endif//__cplusplus > 201100L

#endif//_STD_BIND_HPP_
