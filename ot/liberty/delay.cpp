#include <ot/liberty/delay.hpp>
#include <ot/static/logger.hpp>
#include <ot/headerdef.hpp>
#include <cmath>
#include <vector>
#include <algorithm>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_cdf.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_histogram.h>
#include <gsl/gsl_multiroots.h>
#include <boost/math/distributions/skew_normal.hpp>

namespace ot {

// ------------------------------------------------------------------------------------------------

/* 
* Moment match of two LSN distribution, used in operator +- 
* Additional function required:
* 	 1. The cdf of the normal distribution: 
* 	   float phi(float x);
* 	 2. non-linear equation to get optimal lambda
* 	   int lambda_func(const gsl_vector *x, void *params, gsl_vector *f); 
*/

// the cdf of the normal distribution
float phi(float x)
{
    return gsl_cdf_gaussian_P(x, 1.0);
}

// non-linear equation to get optimal lambda
int lambda_func(const gsl_vector *x, void *params, gsl_vector *f)
{
    // first distribution 
    float mu1    = ((struct Dist_param *)params)->mu1;
    float sigma1 = ((struct Dist_param *)params)->sigma1;
    // second distribution
    float mu2    = ((struct Dist_param *)params)->mu2;
    float sigma2 = ((struct Dist_param *)params)->sigma2;

    // the root of the non-linear equation
    float lambda = gsl_vector_get(x, 0);

    //  non-linear equation
    float term1 = exp(2.0 * mu1) * exp(sigma1 * sigma1) * (exp(sigma1 * sigma1) - 1.0);
    float term2 = exp(2.0 * mu2) * exp(sigma2 * sigma2) * (exp(sigma2 * sigma2) - 1.0);
    float term3 = pow(exp(mu1) * exp(0.5 * sigma1 * sigma1) + exp(mu2) * exp(0.5 * sigma2 * sigma2), 2);
    float term4 = exp((1.0 + lambda * lambda) / (1.0 / (sigma1 * sigma1) + 1.0 / (sigma2 * sigma2)));
    float term5 = phi(2.0 * lambda / sqrt(1.0 / (sigma1 * sigma1) + 1.0 / (sigma2 * sigma2)));
    float term6 = 2.0 * pow(phi(lambda / sqrt(1.0 / (sigma1 * sigma1) + 1.0 / (sigma2 * sigma2))), 2);
    float fval  = (term1 + term2) / term3 - term4 * (term5 / term6) + 1;

    gsl_vector_set(f, 0, fval);

    return GSL_SUCCESS;
} 

float get_lambda( float mu1, float std1, float mu2, float std2 )
{
    const gsl_multiroot_fsolver_type *T;
    gsl_multiroot_fsolver *s;

    int status;
    size_t iter = 0;

    const size_t n = 1;

    // first distribution 
    float mu_1    = mu1;
    float sigma_1 = std1;
    // second distribution
    float mu_2    = mu2;
    float sigma_2 = std2;

    struct Dist_param d = {mu_1, mu_2, sigma_1, sigma_2};

    // the initial solution of root
    float max_sigma = std::max(sigma_1, sigma_2);
    float x0 = sqrt(max_sigma * max_sigma * (1 / (sigma_1 * sigma_1) + 1 / (sigma_2 * sigma_2)) - 1);

    gsl_vector *x = gsl_vector_alloc(n);
    gsl_vector_set(x, 0, x0);

    gsl_multiroot_function f = {&lambda_func, n, &d};

    T = gsl_multiroot_fsolver_hybrids;
    s = gsl_multiroot_fsolver_alloc(T, 1);
    gsl_multiroot_fsolver_set(s, &f, x);

    do {
        iter++;
        status = gsl_multiroot_fsolver_iterate(s);

        if (status)
            break;

        status = gsl_multiroot_test_residual(s->f, 1e-7);
    } while (status == GSL_CONTINUE && iter < 10000);

    if (status != GSL_SUCCESS)
        OT_LOGD("Can't find solutionQQ\n");

    gsl_multiroot_fsolver_free(s);
    gsl_vector_free(x);

    float lambda = gsl_vector_get(s->x, 0);

    return lambda;
}

// ------------------------------------------------------------------------------------------------

// Constructor
Dist::Dist(float nom, float ms, float stdev, float s) 
{
    float _k_star = 0.f, _beta = 0.f;

    _nominal    = nom;
    _mean_shift = ms;
    _mean       = nom + ms;
    _std        = stdev;
    _skew       = s;

    _k_star  = M_PI / 2.0 * pow(2.0 * s / (4.0 - M_PI), 2.0 / 3.0);
    _lambda  = sqrt(_k_star / (1.0 + (2.0 / M_PI - 1.0) * _k_star));
    _beta    = _lambda / sqrt(1.0 + _lambda * _lambda);
    _omega   = sqrt(_std * _std / (1.0 - 2.0 * _beta * _beta / M_PI));
    _epsilon = _mean - sqrt(2.0 / M_PI) * _omega * _beta;
}

// Function: get_sample
// get the sampled value of the distribution
std::vector<float> Dist::get_sample( int sample_num )
{
    std::vector<float> samples(sample_num);

    // Setup generators
    std::random_device rd;
    std::default_random_engine noise_generator;

    // Sample from a uniform distribution i.e. [0,1)
    std::uniform_real_distribution<float> uniform_dist(0, 1.0);

    auto skew_norm_dist = boost::math::skew_normal_distribution<float>(_epsilon, _lambda, _omega);

    for(int i = 0; i < sample_num; i++)
    {
        // Use the probability from the uniform distribution with the percent point
        // function of the skew_normal
        float value = 0.f;

        do {
            float x = 0.f;
            // Take a different value every time to generate probabilities from 0 to 1
            noise_generator.seed(rd());
            x = uniform_dist(noise_generator);
            value = boost::math::quantile(skew_norm_dist, x);

        } while (value == -INFINITY || value == INFINITY);

        samples[i] = value;
    }

    return samples;
}

// Function: get_dist
// get the probability density function of delay distribution
std::vector<float> Dist::get_dist( float time_step )
{
    int num_bins = 0;
    int num_samples = 100000;
    float bin_width = time_step;
    float max_val = 0.f, min_val = 0.f;

    std::vector<float> values = get_sample(num_samples);
    max_val = *max_element(values.begin(), values.end());
    min_val = *min_element(values.begin(), values.end());
    num_bins = int((max_val - min_val) / bin_width) + 1;

    std::vector<float> prob_dist(num_bins);

    gsl_histogram* hist = gsl_histogram_alloc(num_bins);
    gsl_histogram_set_ranges_uniform(hist, min_val, max_val);

    for (auto val : values) {
        gsl_histogram_increment(hist, val);
    }

    for (int i = 0; i < num_bins; i++) {
        double bin_low, bin_high;
        gsl_histogram_get_range(hist, i, &bin_low, &bin_high);
        prob_dist[i] = gsl_histogram_get(hist, i) / (bin_width * values.size());
    }

    gsl_histogram_free(hist);

    return prob_dist;
}

// ------------------------------------------------------------------------------------------------

// Constructor
Statisical_delay::Statisical_delay(float e, float l, float o, float b = 0.0f)
{
    float beta = l / sqrt(1.0 + l * l);
    _epsilon = e;
    _lambda  = l;
    _omega   = o;
    _bias    = b;

    // calculate the statisical moment
    _mean = 2.0 * exp(e) * exp(o * o / 2) * phi(beta * o);
    _std  = sqrt(2.0 * exp(2.0 * e) * exp(o * o) * (exp(o * o) * 
                 phi(2.0 * beta * o) - 2 * pow(phi(beta * o), 2.0)));
    _skew = (2.0 * exp(3.0 * e) * exp(4.5 * o * o) * phi(3.0 * beta * o) - 
             3.0 * _mean * _std * _std - pow(_mean, 3.0)) / pow(_std, 3.0);
}

// Operator
Statisical_delay Statisical_delay::operator+ (const std::variant<float, Dist> dist) const
{
    std::visit(Functors{
        // Case 1: Net arc
        [this] (float f) 
        {
            return Statisical_delay(_epsilon, _lambda, _omega, _bias + f);
        },
        // Case 2: Cell arc
        [this] (Dist d) 
        {
            float epsilon = 0.f, lambda = 0.f, omega = 0.f, bias = 0.f;
            float mu1  = get_mean();
            float std1 = get_std();
            float mu2  = d.get_mean();
            float std2 = d.get_std();

            lambda  = get_lambda(mu1, std1, mu2, std2);
            omega   = sqrt((1.f + lambda * lambda) / (1.f / (std1 * std1) + 1.f / (std2 * std2)));
            epsilon = log(exp(mu1) * exp(std1 * std1 / 2.f) + exp(mu2) * exp(std2 * std2 / 2.f)) -
                    omega * omega / 2.f - log(phi(lambda / sqrt(1.f / (std1 * std1) + 1.f / (std2 * std2))));
            bias    = get_bias();

            return Statisical_delay(epsilon, lambda, omega, bias);
        }
    }, dist);
}

// Operator
Statisical_delay Statisical_delay::operator- (const std::variant<float, Dist> dist) const
{
    std::visit(Functors{
        // Case 1: Net arc
        [this] (float f) 
        {
            return Statisical_delay(_epsilon, _lambda, _omega, _bias + f);
        },
        // Case 2: Cell arc
        [this] (Dist d) 
        {
            float epsilon = 0.f, lambda = 0.f, omega = 0.f, bias = 0.f;
            float mu1  = get_mean();
            float std1 = get_std();
            float mu2  = -1.f * d.get_mean();
            float std2 = d.get_std();

            lambda  = get_lambda(mu1, std1, mu2, std2);
            omega   = sqrt((1.f + lambda * lambda) / (1.f / (std1 * std1) + 1.f / (std2 * std2)));
            epsilon = log(exp(mu1) * exp(std1 * std1 / 2.f) + exp(mu2) * exp(std2 * std2 / 2.f)) -
                    omega * omega / 2.f - log(phi(lambda / sqrt(1.f / (std1 * std1) + 1.f / (std2 * std2))));
            bias    = get_bias();

            return Statisical_delay(epsilon, lambda, omega, bias);
        }
    }, dist);
}

} // end of namespace ot. -----------------------------------------------------------------------

