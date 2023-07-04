#ifndef OT_LIBERTY_DELAY_HPP_
#define OT_LIBERTY_DELAY_HPP_

#include <ot/headerdef.hpp>
#include <vector>
#include <ot/traits.hpp>
#include <gsl/gsl_vector.h>

namespace ot {

// Structure: the mean and standard deviation 
// of two distributions
struct Dist_param
{
    Dist_param(float m1, float m2, float s1, float s2) 
    { 
        mu1 = m1; sigma1 = s1;
        mu2 = m2; sigma2 = s2;
    }

    float mu1, sigma1;
    float mu2, sigma2;
};

// Class: gate delay distribution
// the mean, skewness, standard deviation of the delay 
// querying from nldm standard cell library
class Dist 
{
	public:
		// Constructor function
		Dist(float, float, float, float);

        // basic access methods
		inline float get_mean()	const;
		inline float get_std()	const;
		inline float get_skew()	const;

		// get the sampled value of the distribution
		std::vector<float> get_sample( int sample_num );

		// get the probability of the distribution
		// used in frequency domain calculation
		std::vector<float> get_dist( float time_step );

	private:

		// sample moments
		float _nominal;
		float _mean_shift;
		float _mean;
		float _std;
		float _skew;

		// distribution parameters
		float _epsilon;	// location parameter
		float _lambda;	// scale parameter
		float _omega;	// shape parameter which determines the skewness
};

// Class Statisical_delay
// record the at/rat/slew of a pin, generate by moment matching, 
// need to compute the statisical information according distribution parameters
class Statisical_delay {

	public:

		// Constructor function
		Statisical_delay(float, float, float, float);

        // basic access methods
		inline float get_mean() const;
		inline float get_std () const;
		inline float get_skew() const;
		inline float get_bias() const;
		
		// Get the maximum/minimum delay at 3σ/−3σ percentile point in LSN distribution
		inline float get_max() const;
		inline float get_min() const;

		Statisical_delay operator+ ( const std::variant<float, Dist> dist ) const ;
		Statisical_delay operator- ( const std::variant<float, Dist> dist ) const ;
		// Statisical_delay operator+ ( const Dist *d ) const ;
		// Statisical_delay operator- ( const Dist *d ) const ;
		// Statisical_delay operator+ ( float v ) const ;
		// Statisical_delay operator- ( float v ) const ;

	private:

        // statisical moments
		float _mean;
		float _std;
		float _skew;
		float _bias;

		// distribution parameters
		float _epsilon;	// location parameter
		float _lambda;	// scale parameter
		float _omega;	// shape parameter which determines the skewness
};

// ------------------------------------------------------------------------------------------------

// basic access methods
inline float Dist::get_mean() const
{
	return _mean;
}

inline float Dist::get_std() const
{
	return _std;
}

inline float Dist::get_skew() const
{
	return _skew;
}

// ------------------------------------------------------------------------------------------------

// basic access methods
inline float Statisical_delay::get_mean() const
{
    return _mean;
}

inline float Statisical_delay::get_std()  const
{
    return _std;
}

inline float Statisical_delay::get_skew() const
{
    return _skew;
}

inline float Statisical_delay::get_bias() const
{
    return _bias;
}

// Get the maximum/minimum delay at 3σ/−3σ percentile point in LSN distribution
inline float Statisical_delay::get_max() const
{
	// return _mean + 3 * _std;
    return exp(_epsilon + 3.21 * _omega) + get_bias();
}

inline float Statisical_delay::get_min() const
{
    // return _mean - 3 * _std;
    return exp(_epsilon - 1.79 * _omega) + get_bias();
}

// ------------------------------------------------------------------------------------------------

};  // end of namespace ot. -----------------------------------------------------------------------

#endif