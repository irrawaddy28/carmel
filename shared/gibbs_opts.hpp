#ifndef GRAEHL_SHARED__GIBBS_OPTS_HPP
#define GRAEHL_SHARED__GIBBS_OPTS_HPP

//#include <boost/program_options.hpp>
#include <graehl/shared/time_series.hpp>
#include <graehl/shared/weight.h>
#include <graehl/shared/stream_util.hpp>

namespace graehl {

struct gibbs_opts
{
    static char const* desc() { return "Gibbs (chinese restaurant process) options"; }
    template <class OD>
    void add_options(OD &opt, bool carmel_opts=false, bool forest_opts=false)
    {
        opt.add_options()
            ("crp", defaulted_value(&iter),
             "# of iterations of Bayesian 'chinese restaurant process' parameter estimation (Gibbs sampling) instead of EM")
            ("crp-exclude-prior", defaulted_value(&exclude_prior)->zero_tokens(),
             "When writing .trained weights, use only the expected counts from samples, excluding the prior (p0) counts")
            ("cache-prob", defaulted_value(&cache_prob)->zero_tokens(),
             "Show the true probability according to cache model for each sample")
            ("sample-prob", defaulted_value(&ppx)->zero_tokens(),
             "Show the sample prob given model, previous sample")
            ("high-temp", defaulted_value(&high_temp),
             "Raise probs to 1/temp power before making each choice - deterministic annealing for --unsupervised")
            ("low-temp", defaulted_value(&low_temp),
             "See high-temp. temperature is high-temp @i=0, low-temp @i=finaltemperature at final iteration (linear interpolation from high->low)")
            ("burnin", defaulted_value(&burnin),
             "When summing gibbs counts, skip <burnin> iterations first (iteration 0 is a random derivation from initial weights)")
            ("final-counts", defaulted_value(&final_counts)->zero_tokens(),
             "Normally, counts are averaged over all the iterations after --burnin.  this option says to use only final iteration's (--burnin is ignored; effectively set burnin=# crp iter)")
            ("crp-restarts", defaulted_value(&restarts),
             "Number of additional runs (0 means just 1 run), using cache-prob at the final iteration select the best for .trained and --print-to output.  --init-em affects each start.  TESTME: print-every with path weights may screw up start weights")
            ("crp-argmax-final",defaulted_value(&argmax_final)->zero_tokens(),
             "For --crp-restarts, choose the sample/.trained weights with best final sample cache-prob.  otherwise, use best entropy over all post --burnin samples")
            ("crp-argmax-sum",defaulted_value(&argmax_sum)->zero_tokens(),
             "Instead of multiplying the sample probs together and choosing the best, sum (average) them")
            ("print-counts-from",defaulted_value(&print_counts_from),
             "Every --print-every, print the instantaneous and cumulative counts for parameters from...(to-1) (for debugging)")
            ("print-counts-to",defaulted_value(&print_counts_to),
             "See print-counts-from.  -1 means until end")
            ("print-counts-sparse",defaulted_value(&print_counts_sparse),
             "(if nonzero) skip printing counts with avg(count)<prior+sparse, and show parameter index")
            ("print-counts-rich",defaulted_value(&rich_counts)->zero_tokens(),
             "print a rich identifier for each parameter's counts")
            ("width",defaulted_value(&width),
             "limit counts/probs to this many printed characters")
            ("print-norms-from",defaulted_value(&print_norms_from),
             "Every --print-every, print the normalization groups' instantaneous (proposal HMM) sum-counts, for from...(to-1)")
            ("print-norms-to",defaulted_value(&print_norms_to),
             "See print-norms-to.  -1 means until end")
            ("print-every",defaulted_value(&print_every),
             "print the 0th,nth,2nth,,... (every n) iterations as well as the final one.  these are prefaced and suffixed with comment lines starting with #")
            ("progress-every",defaulted_value(&tick_every),
             "show a progress tick (.) every N blocks")
            ;
        if (forest_opts)
            opt.add_options()
                ("alpha",defaulted_value(&alpha),
                 "prior applied to initial param values: alpha*p0*N (where N is # of items in normgroup, so uniform has p0*N=1)")
                ;

        if (carmel_opts)
            opt.add_options()
                ("print-from",defaulted_value(&print_from),
                 "For [print-from]..([print-to]-1)th input transducer, print the final iteration's path on its own line.  a blank line follows each training example")
                ("print-to",defaulted_value(&print_to),
                 "See print-from.  -1 means until end")
                ("init-em",defaulted_value(&init_em),
                 "Perform n iterations of EM to get weights for randomly choosing initial sample, but use initial weights (pre-em) for p0 base model; note that EM respects tied/locked arcs but --crp removes them")
                ("em-p0",defaulted_value(&em_p0)->zero_tokens(),
                 "With init-em=n, use the trained weights as the base distribution as well (note: you could have done this in a previous carmel invocation, unlike --init-em alone)")
                ("uniform-p0",defaulted_value(&uniformp0)->zero_tokens(),
                 "Use a uniform base probability model for --crp, even when the input WFST have weights.  --em-p0 overrides this.")
                ;
    }
    //forest-em and carmel supported:
    unsigned iter;
    bool exclude_prior;
    bool cache_prob;
    bool ppx;
    double high_temp,low_temp;
    unsigned burnin;
    bool final_counts;
    unsigned print_every; // print the current sample every N iterations
    unsigned print_counts_from,print_counts_to; // which param ids' counts to print
    bool rich_counts; // show extra identification for each parameter, not just its id
    unsigned width; // # chars for counts/probs etc.
    double print_counts_sparse;
    unsigned print_norms_from,print_norms_to; // which normgroup ids' sums to print

    unsigned restarts; // 0 = 1 run (no restarts)
    unsigned tick_every;

     // criteria to max over restarts:
    bool argmax_final;
    bool argmax_sum;

    //carmel only:
    unsigned init_em;
    bool em_p0;
    bool uniformp0;
    unsigned print_from,print_to; // which blocks to print

    //forest-em only:
    double alpha;

    bool printing_sample() const
    {
        return print_to>print_from;
    }

    bool printing_counts() const
    {
        return print_counts_to>print_counts_from;
    }

    bool printing_norms() const
    {
        return print_norms_to>print_norms_from;
    }


    // random choices have probs raised to 1/temperature(iteration) before coin flip
    typedef clamped_time_series<double> temps;
    temps temp;
    temps temperature() const {
        return temps(high_temp,low_temp,iter,temps::linear);
    }
    gibbs_opts() { set_defaults(); }
    void set_defaults()
    {
        tick_every=0;
        width=7;
        iter=0;
        burnin=0;
        restarts=0;
        init_em=0;
        em_p0=false;
        cache_prob=false;
        print_counts_sparse=0;
        print_counts_from=print_counts_to=0;
        print_norms_from=print_norms_to=0;
        uniformp0=false;
        ppx=true;
        print_every=0;
        print_from=print_to=0;
        high_temp=low_temp=1;
        final_counts=false;
        argmax_final=false;
        argmax_sum=false;
        exclude_prior=false;
    }
    void validate()
    {
        if (tick_every<1) tick_every=1;
        if (final_counts) burnin=iter;
        if (burnin>iter)
            burnin=iter;
        if (restarts>0)
            cache_prob=true;
//            if (!cumulative_counts) argmax_final=true;
        temp=temperature();
    }
};



struct gibbs_stats
{
    double N; // from burnin...last iter, or just last if --final-counts
    Weight sumprob; //FIXME: more precision, or (scale,sum) pair
    Weight allprob,finalprob; // (prod over all N, and final) sample cache probs
    typedef gibbs_stats self_type;
    gibbs_stats() {clear();}
    void clear()
    {
        allprob.setOne();
        finalprob.setOne();
        sumprob=0;
        N=0;
    }
    void record(double t,Weight prob)
    {
        if (t>=0) {
            N+=1;
            sumprob+=prob;
            allprob*=prob;
            finalprob=prob;
        }
    }
    TO_OSTREAM_PRINT
    template <class O>
    void print(O&o) const
    {
        o << "#samples="<<N<<" final sample ppx="<<finalprob.ppxper().as_base(2)<<" burned-in avg="<<allprob.ppxper(N).as_base(2);
    }
    bool better(gibbs_stats const& o,gibbs_opts const& gopt) const
    {
        return gopt.argmax_final ? finalprob>o.finalprob : (gopt.argmax_sum ? sumprob>o.sumprob : allprob>o.allprob);
    }
};

}

#endif