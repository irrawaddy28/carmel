#ifndef _FUNCS_HPP
#define _FUNCS_HPP

#include <cmath>
#include <string>
#include <boost/scoped_array.hpp>
#include <boost/lexical_cast.hpp>

#include <boost/iterator/iterator_adaptor.hpp>
#include <stdexcept>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/utility/enable_if.hpp>

template <class T>
struct bounded_iterator : public boost::iterator_adaptor<bounded_iterator<T>,T> {
private:
    struct enabler {};
    typedef boost::iterator_adaptor<bounded_iterator<T>,T> Super;
    friend class boost::iterator_core_access;
    void increment() { if (this->base() == end) throw std::out_of_range("bounded iterator incremented past end"); ++this->base_reference(); }
    T end;
public:
    bounded_iterator(const T& begin,const T &end_) : Super(begin), end(end_) {}
/*    template <class O>
    bounded_iterator(bounded_iter<O> const& other, typename boost::enable_if<boost::is_convertible<O*,T*>, enabler>::type = enabler())
    ) : Super(other.base()) {}*/
    bounded_iterator(bounded_iterator<T> const &other) : Super(other),end(other.end) {}
};

template <class T>
inline std::string concat(const std::string &s,const T& suffix) {
    return s+boost::lexical_cast<std::string>(suffix);
}

template <class S,class T>
inline std::string concat(const S &s,const T& suffix) {
    return boost::lexical_cast<std::string>(s)+boost::lexical_cast<std::string>(suffix);
}


inline unsigned random_less_than(unsigned limit) {
    Assert(limit!=0);
    if (limit <= 1)
        return 0;
// correct against bias (which is worse when limit is almost RAND_MAX)
    const unsigned randlimit=(RAND_MAX / limit)*limit;
    unsigned r;
    while ((r=std::rand()) >= randlimit) ;
    return r % limit;
}

#define NLETTERS 26
// works for only if a-z A-Z and 0-9 are contiguous

inline char random_alpha() {
    unsigned r=random_less_than(NLETTERS*2);
    return (r < NLETTERS) ? 'a'+r : ('A'-NLETTERS)+r;
}

inline char random_alphanum() {
    unsigned r=random_less_than(NLETTERS*2+10);
    return r < NLETTERS*2 ? ((r < NLETTERS) ? 'a'+r : ('A'-NLETTERS)+r) : ('0'-NLETTERS*2)+r;
}
#undef NLETTERS

inline std::string random_alpha_string(unsigned len) {
    boost::scoped_array<char> s(new char[len+1]);
    char *e=s.get()+len;
    *e='\0';
    while(s.get() < e--)
        *e=random_alpha();
    return s.get();
}


inline double random_pos_fraction() // returns uniform random number on (0..1]
{
    return ((double)std::rand()+1.) /
        ((double)RAND_MAX+1.);
}

struct set_one {
    template <class C>
    void operator()(C &c) {
        c=1;
    }
};

struct set_zero {
    template <class C>
    void operator()(C &c) {
        c=0;
    }
};

template <class V>
struct set_value {
    V v;
    set_value(const V& init_value) : v(init_value) {}
    template <class C>
    void operator()(C &c) {
        c=v;
    }
};

template <class V>
set_value<V> value_setter(V &init_value) {
    return set_value<V>(init_value);
}

struct set_random_pos_fraction {
    template <class C>
    void operator()(C &c) {
        c=random_pos_fraction();
    }
};


// useful for sorting; could parameterize on predicate instead of just <=lt, >=gt
template <class I, class B>
struct indirect_lt {
    typedef I Index;
    typedef B Base;
    B base;
    indirect_lt(const B &b) : base(b) {}
    indirect_lt(const indirect_lt<I,B> &o): base(o.base) {}

    bool operator()(const I &a, const I &b) const {
        return base[a] < base[b];
    }
};


template <class I, class B>
struct indirect_gt {
    typedef I Index;
    typedef B Base;
    B base;
    indirect_gt(const B &b) : base(b) {}
    indirect_gt(const indirect_gt<I,B> &o): base(o.base) {}
    bool operator()(const I &a, const I &b) const {
        return base[a] > base[b];
    }
};

  template <class ForwardIterator>
  bool is_sorted(ForwardIterator begin, ForwardIterator end)
  {
    if (begin == end) return true;

    ForwardIterator next = begin ;
    ++next ;
    for ( ; next != end; ++begin , ++next) {
      if (*next < *begin) return false;
    }

    return true;
  }

  template <class ForwardIterator, class StrictWeakOrdering>
  bool is_sorted(ForwardIterator begin, ForwardIterator end,
                 StrictWeakOrdering comp)
  {
    if (begin == end) return true;

    ForwardIterator next = begin ;
    ++next ;
    for ( ; next != end ; ++begin, ++next) {
      if ( comp(*next, *begin) ) return false;
    }

    return true;
  }

  template <class ForwardIterator, class ValueType >
  void iota(ForwardIterator begin, ForwardIterator end, ValueType value)
  {
    while ( begin != end ) {
      *begin = value ;
      ++begin ;
      ++value ;
    }
  }

#ifdef TEST
#include "test.hpp"
#include <cctype>
#include "debugprint.hpp"
#endif

#ifdef TEST
BOOST_AUTO_UNIT_TEST( TEST_FUNCS )
{
    using namespace std;
    const int NREP=10000;
    for (int i=1;i<NREP;++i) {
        unsigned ran_lt_i=random_less_than(i);
        BOOST_CHECK(0 <= ran_lt_i && ran_lt_i < i);
        BOOST_CHECK(isalpha(random_alpha()));
        char r_alphanum=random_alphanum();
        BOOST_CHECK(isalpha(r_alphanum) || isdigit(r_alphanum));
    }
}
#endif

#endif
