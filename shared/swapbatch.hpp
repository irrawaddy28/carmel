#ifndef SWAPBATCH_HPP
#define SWAPBATCH_HPP

#include "checkpoint_istream.hpp"
#include "memmap.hpp"
#include <string>
#include "dynarray.h"
#include <boost/lexical_cast.hpp>
#include "backtrace.hpp"
#include "stackalloc.hpp"

#ifdef HINT_SWAPBATCH_BASE
# ifdef BOOST_IO_WINDOWS
#  ifdef CYGWIN
#   define VIRTUAL_MEMORY_USER_HEAP_TOP ((void *)0x0A000000U)
#  else
#   define VIRTUAL_MEMORY_USER_HEAP_TOP ((void *)0x78000000U)
#  endif
# else
#  ifdef SOLARIS
#   define VIRTUAL_MEMORY_USER_HEAP_TOP ((void *)0xE0000000U)
#  else
#   define VIRTUAL_MEMORY_USER_HEAP_TOP ((void *)0xA0000000U)
#  endif
# endif
# define DEFAULT_SWAPBATCH_BASE_ADDRESS VIRTUAL_MEMORY_USER_HEAP_TOP
// should be NULL but getting odd bad_alloc failure
#else
# define DEFAULT_SWAPBATCH_BASE_ADDRESS ((void *)NULL)
#endif

/*
 * given an input file, a class B with a read method, a batch size (in bytes), and a pathname prefix:
 *
 *  memory maps a region of batch size bytes to files created as prefix+N,
 *  from N=[0,n_batches)

 * On destruction, the files are deleted; without additional specification they
 *wouldn't be portable to future runs (relocatable memory addresses,
 *endian-ness)

 * each batch entry is prefixed by a (aligned - so not indicating the actual
 * semantic content length) size_type d_next; incrementing a size_type pointer by that amount
 * gets you to the next batchentry's header (essentially, a linked list with
 * variable sized data). when the delta is 0, that means the current batch is
 * done, and the next, if any, may be mapped in.  following the d_next header is
 * a region of sizeof(B) bytes, typed as B, of course.  (in this way, forward
 * iteration is supported). no indexing is done to permit random access (except
 * that you may seek to a particular batch), although that could be supported
 * (placing the index at the end or kept in the normal heap).  note that the
 * d_next isn't known until the B is read; the length field is allocated but not
 * initialized until the read is complete.

 * B's read method is free to use any amount of unused batch space (filled from
 * lower addresses->top like a stack).  The read method must acquire it using an
 * (untyped) StackAlloc (which supports, essentially, a bound-checked
 * push_back(N) operation).  Perhaps later I can provide a heap (allowing
 * dealloc/reuse) allocator wrapper that takes a StackAlloc (like sbrk() used in
 * malloc()).

 * When the StackAlloc can't fulfill a request, it throws an exception which is
 * to be caught by SwapBatch; if the batch wasn't empty, a new one can be
 * allocated, the input file read pointer seeked back, and the read retried.  A
 * second failure is permanent (the size of the batch can't be increased, unless
 * we are willing to require a B::relocate() which allows for loading a batch at
 * a different base address; you can't assure a fixed mmap unless you're
 * replacing an old one.
 *
 * StackAlloc requires you to explicitly align<T>() before alloc<T>(). (or use
 * aligned_alloc<T>())
 *
 * void read(ifstream &in,B &b,StackAlloc &alloc) ... which sets !in (or throws)
 * if input fails, and uses alloc.alloc<T>(n) as space to store an input, returning the
 * new beg - [ret,end) is the new unused range
 */


template <class B>
struct SwapBatch {
    typedef B BatchMember;
    typedef std::size_t size_type; // boost::intmax_t

    struct iterator
    {
        typedef SwapBatch<B> Cont;
        typedef B value_type;
        typedef B &reference;
        typedef B *pointer;
        typedef void difference_type; // not implemented but could be unsigned
        typedef forward_iterator_tag iterator_category;

        Cont *cthis;
        unsigned batch;
        size_type *header;
        reference operator *()
        {
            Assert(batch < cthis->n_batch);
            load_batch(batch);
            return Cont::data_for_header(&header);
        }
        pointer operator ->()
        {
            return &operator *();
        }
        void set_end()
        {
            cthis=NULL;
        }
        void is_end() const
        {
            return cthis==NULL;
        }
        void operator ++()
        {
            if (*header) {
                header += *header;
                if (!*header && batch == cthis->nbatch-1)
                    set_end();
            } else {
                ++batch;
                Assert (batch < cthis->nbatch)
                cthis->load_batch(batch);
                header=cthis->memmap.begin();
            }
        }
        bool operator ==(const iterator &o) const
        {
            Assert (!cthis || !o.cthis || cthis==o.cthis);
            if (o.is_end()  && is_end())
                return true;
            if (!o.is_end() && !is_end())
                return  o.batch==batch && o.header == o.header; // cthis=o.cthis &&
            return false;
//                return cthis==NULL; //!*header && batch = cthis->nbatch-1; // now checked in operator ++
        }
        bool operator !=(const iterator &o) const
        {
            return ! operator==(o);
        }
    };

    iterator begin()
    {
        iterator ret;
        if (total_items) {
            ret.cthis=this;
            ret.batch=0;
            ret.header=memmap.begin();
        } else
            return end();
        return ret;
    }

    iterator end()
    {
        iterator ret;
        ret.set_end();
        return ret;
    }


    size_type n_batch;
    std::string basename;
    mapped_file memmap;
    size_type batchsize;
    unsigned loaded_batch; // may have to go if we allow memmap sharing between differently-typed batches.
    StackAlloc space;
    bool autodelete;
    void preserve_swap() {
        autodelete=false;
    }
    void autodelete_swap() {
        autodelete=true;
    }
    std::string batch_name(unsigned n) const {
        return basename+boost::lexical_cast<std::string>(n);
    }
/*    void *end() {
        return memmap.end();
    }
    void *begin() {
        return memmap.data();
    }
    const void *end() const {
        return memmap.end();
    }
    const void *begin() const {
        return memmap.data();
        }*/
    size_type capacity() const {
        return memmap.size();
    }
    void create_next_batch() {
        BACKTRACE;
//        DBP_VERBOSE(0);
        DBPC2("creating batch",n_batch);
        if (n_batch==0) {
            char *base=(char *)DEFAULT_SWAPBATCH_BASE_ADDRESS;
            if (base) {
                const unsigned mask=0x0FFFFFU;
//                DBP4((void *)base,(void*)~mask,(void*)(mask+1),batchsize&(~mask));
                unsigned batchsize_rounded_up=(batchsize&(~mask));
//                DBP2((void*)batchsize_rounded_up,(void*)(mask+1));
                batchsize_rounded_up+=(mask+1);
//                DBP((void*)batchsize_rounded_up);
//                DBP3((void *)base,(void *)batchsize_rounded_up,(void *)(base-batchsize_rounded_up));
                base -= batchsize_rounded_up;
            }
            memmap.open(batch_name(n_batch),std::ios::out,batchsize,0,true,base,true); // creates new file and memmaps
        } else {
            memmap.reopen(batch_name(n_batch),std::ios::out,batchsize,0,true); // creates new file and memmaps
        }
        loaded_batch=n_batch++;
        space.init(memmap.begin(),memmap.end());
        d_tail=space.alloc<size_type>(); // guarantee already aligned
        *d_tail=0; // class invariant: each batch has a chain of d_tail diffs ending in 0.
    }
    void load_batch(unsigned i) {
        BACKTRACE;
        if (loaded_batch == i)
            return;
        if (i >= n_batch)
            throw std::range_error("batch swapfile index too large");
        loaded_batch=(unsigned)-1;
        memmap.reopen(batch_name(i),std::ios::in,batchsize,0,false);
        loaded_batch=i;
    }
    void remove_batches() {
        BACKTRACE;
        for (unsigned i=0;i<=n_batch;++i) { // delete next file too in case exception came during create_next_batch
            remove_file(batch_name(i));
        }
        n_batch=0; // could loop from n_batch ... 0 but it might confuse :)
    }
    size_t total_items;
    size_t size() const {
        BACKTRACE;
        return total_items;
    }
    size_t n_batches() const {
        return n_batch;
    }
    void print_stats(std::ostream &out) const {
        out << size() << " items in " << n_batches() << " batches of " << batchsize << " bytes, stored in " << basename << "N";
    }
    size_type *d_tail;
    BatchMember *read_one(std::ifstream &is)
    {
        BACKTRACE;
        DBP_ADD_VERBOSE(3);
        std::streampos pos=is.tellg();
        bool first=true;
        void *save;
    again:
        save=space.save_end();
        BatchMember *newguy;
        try {
            space.alloc_end<size_type>(); // precautionary: ensure we can alloc new d_tail.
            DBP(space.remain());
            newguy=space.aligned_alloc<BatchMember>();
            DBP2((void*)newguy,space.remain());
            read((std::istream&)is,*newguy,space);
            DBP2(newguy,space.remain());
        }
        catch (StackAlloc::Overflow &o) {
            if (!first)
                throw std::ios::failure("an entire swap space segment couldn't hold the object being read.");
            //ELSE:
            is.seekg(pos);
            create_next_batch();
            first=false;
            goto again;
        }
        if (!is) {
            if (is.eof()) {
                return NULL;
            }
            throw std::ios::failure("error reading item into swap batch.");
        }
        // ELSE: read was success!
        ++total_items;
        DBP2(save,space.end);
        space.restore_end(save);
        DBP2(space.top,space.end);
        size_type *d_last_tail=d_tail;
        Assert(space.capacity<size_type>());
        d_tail=space.aligned_alloc<size_type>(); // can't fail, because of safety
        *d_tail=0; // indicates end of batch; will reset if read is succesful
        *d_last_tail=d_tail-d_last_tail;
        DBP((void*)space.top);
        return newguy;
    }

    void read_all(std::ifstream &is) {
        BACKTRACE;
        while(is)
            read_one(is);
    }

    template <class F>
    void read_all_enumerate(std::ifstream &is,F f) {
        BACKTRACE;
        while(is) {
            BatchMember *newguy=read_one(is);
            if (newguy)
                deref(f)(*newguy);
        }

    }

    static BatchMember *data_for_header(size_type *header)
    {
        BatchMember *b=(BatchMember*)(d_next+1);
        b=::align(b);
        return b;
    }

    template <class F>
    void enumerate(F f)  {
        BACKTRACE;
        //      DBP_VERBOSE(0);

        for (unsigned i=0;i<n_batch;++i) {
            DBPC2("enumerate batch ",i);
            load_batch(i);
            //=align((size_type *)begin()); // can guarantee first alignment implicitly
            for(const size_type *d_next=(size_type *)memmap.begin();*d_next;d_next+=*d_next) {
                deref(f)(*data_for_header(d_next));
            }
        }
    }

    SwapBatch(const std::string &basename_,size_type batch_bytesize) : basename(basename_),batchsize(batch_bytesize), autodelete(true) {
        BACKTRACE;
        total_items=0;
        n_batch=0;
        create_next_batch();
    }


    ~SwapBatch() {
        BACKTRACE;
        if (autodelete)
            remove_batches();
    }
};

/*
template <class B>
void read(std::istream &in,B &b,StackAlloc &a) {
    in >> b;
}
*/

void read(std::istream &in,const char * &b,StackAlloc &a) {
    char c;
    std::istream::sentry s(in,true); //noskipws!
    char *p=a.alloc<char>(); // space for the final 0
    b=p;
    if (s) {
        while (in.get(c) && c!='\n'){
            a.alloc<char>(); // space for this char (actually, the next)
            *p++=c;
        }

    }
    *p=0;
    // # alloc<char> = # p increments, + 1.  good.
}

# ifdef TEST
#  include "stdio.h"
#  include "string.h"

const char *swapbatch_test_expect[] = {
    "string one",//10+8
    "2",//2+8
    "3 .",//4+8
    " abcdefghijklmopqrstuvwxyz",
    "4",
    "end",
    ""
};


void swapbatch_test_do(const char *c) {
    static unsigned swapbatch_test_i=0;
    const char *o=swapbatch_test_expect[swapbatch_test_i++];
    DBP2(o,c);
    BOOST_CHECK(!strcmp(c,o));
}

#  include <sstream>
#  include "os.hpp"

BOOST_AUTO_UNIT_TEST( TEST_SWAPBATCH )
{
    using namespace std;
    const char *s1="string one\n2\n3 .\n abcdefghijklmopqrstuvwxyz\n4\nend\n\n";
    string t1=tmpnam(0);
    DBP(t1);
    SwapBatch<const char *> b(t1,28+2*sizeof(size_t)+sizeof(char*)); // string is exactly 28 bytes counting \n
    tmp_fstream i1(s1);
    BOOST_CHECK_EQUAL(b.n_batches(),1);
    BOOST_CHECK_EQUAL(b.size(),0);
    b.read_all((ifstream&)i1.file);
    BOOST_CHECK_EQUAL(b.n_batches(),5);
    BOOST_CHECK_EQUAL(b.size(),sizeof(swapbatch_test_expect)/sizeof(const char *));
    b.enumerate(swapbatch_test_do);
}

# endif

#endif
