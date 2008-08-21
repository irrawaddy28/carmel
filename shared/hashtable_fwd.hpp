// for sources defining hash functions but not using hash tables themselves
#ifndef GRAEHL_SHARED_HASHTABLE_FWD_HPP
#define GRAEHL_SHARED_HASHTABLE_FWD_HPP

#define GOLDEN_MEAN_FRACTION 2654435769U

#ifdef USE_GRAEHL_HASH_MAP
#  ifdef USE_GNU_HASH_MAP
#   undef USE_GRAEHL_HASH_MAP
#  endif 
#else 
# define USE_GNU_HASH_MAP
#endif

#include <cstddef>
#include <boost/functional/hash/hash.hpp>
#include <graehl/shared/hash_functions.hpp>

#ifdef USE_GNU_HASH_MAP
# define USE_STD_HASH_MAP
# ifndef __NO_GNU_NAMESPACE__
//using namespace __gnu_cxx;
# endif
# include <ext/hash_map>
# define HashTable __gnu_cxx::hash_map
# define HASHNS  __gnu_cxx::          
# define HASHNS_B namespace __gnu_cxx {
# define HASHNS_E }
# include <string>
HASHNS_B
template <class Char,class T,class A>
struct hash<std::basic_string<Char,T,A> > 
{
    typedef std::basic_string<Char,T,A> arg_type;
    const std::size_t operator()( const  arg_type &s ) const 
    {
                std::size_t h=0;
//        return cstr_hash(s.c_str());
                for (typename arg_type::const_iterator i=s.begin(),e=s.end();i!=e;++i)
                h = 31 * h + *i; // should optimize to ( h << 5 ) - h if faster
        return h;
    }
};
HASHNS_E
#endif

#ifdef UNORDERED_MAP
# define USE_STD_HASH_MAP
//#error
#include <boost/unordered_map.hpp>
// no template typedef so ...
#define HashTable boost::unordered_map
#define HASHNS boost::          
#define HASHNS_B namespace boost {
#define HASHNS_E }

/*
template <class K,class V,class H,class P,class A>
inline typename HashTable<K,V,H,P,A>::iterator find_value(const HashTable<K,V,H,P,A>& ht,const K& first) {
  return ht.find(first);
}
*/


#endif

namespace graehl {

    template <class H>
    struct hash_traits
    {
        typedef typename H::iterator find_return_type;
        typedef std::pair<find_return_type,bool> insert_return_type;
    };
        
template <class Map,class Key> inline
bool has_key(const Map &map,const Key &key)
{
    return map.find(key)!=map.end();
}


#ifndef USE_GRAEHL_HASH_MAP
template <class K,class V,class H,class P,class A>
inline V *add(HashTable<K,V,H,P,A>& ht,const K&k,const V& v=V())
{
  return &(ht[k]=v);
}

template <class Map,class Key> inline
const typename Map::data_type *find_second(const Map &map,const Key &key)
{
    typename Map::const_iterator find_it=map.find(key);
    if (find_it == map.end())
        return NULL;
    return &find_it->second;
}

template <class Map,class Key> inline
typename Map::data_type *find_second(Map &map,const Key &key)
{
    typename Map::iterator find_it=map.find(key);
    if (find_it == map.end())
        return NULL;
    return &find_it->second;
}

#else
// graehl hash map

template <class K,class V,class H,class P,class A> class HashTable ;

template <class K,class V,class H,class P,class A>
inline V *find_second(const HashTable<K,V,H,P,A>& ht,const K& first)
{
  return ht.find_second(first);
}

#define HASHNS graehl::
#define HASHNS_B namespace graehl {
#define HASHNS_E }

template <class T> struct hash;

  template <class C>
  struct hash { std::size_t operator()(const C &c) const { return boost::hash_value(c); } };
#endif

template<class T1,class T2,class T3,class T4,class T5,class A,class B>
inline
std::basic_ostream<A,B>&
         operator<< (std::basic_ostream<A,B> &out, const HashTable<T1,T2,T3,T4,T5>& t) {
  typename HashTable<T1,T2,T3,T4,T5>::const_iterator i=t.begin();
  out << "begin" << std::endl;
  for (;i!=t.end();++i) {
        out << *i << std::endl;
  }
  out << "end" << std::endl;
  return out;
}

template<class C1,class C2,class A,class B>
inline
std::basic_ostream<A,B>&
         operator<< (std::basic_ostream<A,B> &out, const std::pair< C1, C2 > &p)
{
  return out << '(' << p.first << ',' << p.second << ')';
}

template <class C,class H=
          HASHNS hash<typename C::value_type>
>
struct hash_container
{
    const std::size_t operator()(const C& c) const 
    {
        return hash_range(c.begin(),c.end(),H());
    }
};

template <class K,class V,class H,class P,class A>
inline
  typename HashTable<K,V,H,P,A>::insert_return_type insert(HashTable<K,V,H,P,A>& ht,const K& first,const V &v=V())
{
  return ht.insert(first,v);
}

template <class InsertRet>
inline bool was_inserted(const InsertRet &insert_ret) 
{
    return insert_ret.second;
}

}//graehl

#define BEGIN_HASH_VAL(C) \
HASHNS_B \
template<> struct hash<C> \
{ \
  std::size_t operator()(const C x) const



#define BEGIN_HASH(C) \
HASHNS_B \
template<> struct hash<C> \
{ \
  std::size_t operator()(const C& x) const

#define END_HASH        \
};\
HASHNS_E

HASHNS_B
  template<class P> struct hash<P *>
  {
    const std::size_t operator()( const P *x ) const
    {
//      const unsigned GOLDEN_MEAN_FRACTION=2654435769U;
      return ((std::size_t)x / sizeof(P))*GOLDEN_MEAN_FRACTION;
    }
  };
HASHNS_E


#endif
