/*!
 * \file
 * \brief Filters a signal.
 *
 * \section LICENSE
 * This file is under MIT license (https://opensource.org/licenses/MIT).
 */
#ifndef FILTER_HXX_
#define FILTER_HXX_

#include <stdexcept>
#include <cmath>
#include <sstream>

#include "Tools/Exception/exception.hpp"

#include "Filter.hpp"

namespace aff3ct
{
namespace module
{

template <typename R>
Filter<R>::
Filter(const int N, const int N_fil, const int n_frames)
: Module(n_frames), N(N), N_fil(N_fil)
{
	const std::string name = "Filter";
	this->set_name(name);
	this->set_short_name(name);

	if (N <= 0)
	{
		std::stringstream message;
		message << "'N' has to be greater than 0 ('N' = " << N << ").";
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
	}

	if (N_fil <= 0)
	{
		std::stringstream message;
		message << "'N_fil' has to be greater than 0 ('N_fil' = " << N_fil << ").";
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
	}

	this->init_processes();
}

template <typename R>
void Filter<R>::
init_processes()
{
	auto &p2 = this->create_task("filter");
	auto &p2s_X_N1 = this->template create_socket_in <R>(p2, "X_N1", this->N * this->n_frames);
	auto &p2s_Y_N2 = this->template create_socket_out<R>(p2, "Y_N2", this->N_fil * this->n_frames);
	this->create_codelet(p2, [this, &p2s_X_N1, &p2s_Y_N2]() -> int
	{
		this->filter(static_cast<R*>(p2s_X_N1.get_dataptr()),
		             static_cast<R*>(p2s_Y_N2.get_dataptr()));

		return 0;
	});

}

template <typename R>
int Filter<R>::
get_N() const
{
	return this->N;
}

template <typename R>
int Filter<R>::
get_N_fil() const
{
	return this->N_fil;
}

template <typename R>
template <class AR>
void Filter<R>::
filter(const std::vector<R,AR>& X_N1, std::vector<R,AR>& Y_N2, const int frame_id)
{
	if (this->N * this->n_frames != (int)X_N1.size())
	{
		std::stringstream message;
		message << "'X_N1.size()' has to be equal to 'N' * 'n_frames' ('X_N1.size()' = " << X_N1.size()
		        << ", 'N' = " << this->N << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}

	if (this->N_fil * this->n_frames != (int)Y_N2.size())
	{
		std::stringstream message;
		message << "'Y_N2.size()' has to be equal to 'N_fil' * 'n_frames' ('Y_N2.size()' = " << Y_N2.size()
		        << ", 'N_fil' = " << this->N_fil << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}

	this->filter(X_N1.data(), Y_N2.data(), frame_id);
}

template <typename R>
void Filter<R>::
filter(const R *X_N1, R *Y_N2, const int frame_id)
{
	const auto f_start = (frame_id < 0) ? 0 : frame_id % this->n_frames;
	const auto f_stop  = (frame_id < 0) ? this->n_frames : f_start +1;

	for (auto f = f_start; f < f_stop; f++)
		this->_filter(X_N1 + f * this->N,
		              Y_N2 + f * this->N_fil,
		              f);
}

template <typename R>
void Filter<R>::
_filter(const R *X_N1, R *Y_N2, const int frame_id)
{
	throw tools::unimplemented_error(__FILE__, __LINE__, __func__);
}


}
}

#endif
