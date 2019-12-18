#ifndef SYNCHRONIZER_FRAME_HXX_
#define SYNCHRONIZER_FRAME_HXX_

#include <string>
#include <memory>
#include <stdexcept>
#include <cmath>
#include <sstream>

#include "Tools/Exception/exception.hpp"

namespace aff3ct
{
namespace module
{

template <typename R>
Synchronizer_frame<R>::
Synchronizer_frame(const int N, const int n_frames)
: Module(n_frames), N_in(N), N_out(N)
{
	const std::string name = "Synchronizer_frame";
	this->set_name(name);
	this->set_short_name(name);

	if (N_in <= 0)
	{
		std::stringstream message;
		message << "'N_in' has to be greater than 0 ('N_in' = " << N_in << ").";
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
	}

	if (N_out <= 0)
	{
		std::stringstream message;
		message << "'N_out' has to be greater than 0 ('N_out' = " << N_out << ").";
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
	}

	auto &p1 = this->create_task("synchronize");
	auto p1s_X_N1  = this->template create_socket_in <R>  (p1, "X_N1" , this->N_in );
	auto p1s_Y_N2  = this->template create_socket_out<R>  (p1, "Y_N2" , this->N_out);
	auto p1s_delay = this->template create_socket_out<int>(p1, "delay", 1          );
	this->create_codelet(p1, [p1s_X_N1, p1s_Y_N2, p1s_delay](Module &m, Task &t) -> int
	{
		static_cast<Synchronizer_frame<R>&>(m).synchronize(static_cast<R*  >(t[p1s_X_N1 ].get_dataptr()),
		                                                   static_cast<R*  >(t[p1s_Y_N2 ].get_dataptr()),
		                                                   static_cast<int*>(t[p1s_delay].get_dataptr()));

		return 0;
	});
}

template <typename R>
int Synchronizer_frame<R>::
get_N_in() const
{
	return this->N_in;
}

template <typename R>
int Synchronizer_frame<R>::
get_N_out() const
{
	return this->N_out;
}

template <typename R>
template <class AR>
void Synchronizer_frame<R>::
synchronize(const std::vector<R,AR>& X_N1, std::vector<R,AR>& Y_N2, std::vector<int>& delay, const int frame_id)
{
	if (this->N_in * this->n_frames != (int)X_N1.size())
	{
		std::stringstream message;
		message << "'X_N1.size()' has to be equal to 'N' * 'n_frames' ('X_N1.size()' = " << X_N1.size()
		        << ", 'N' = " << this->N_in << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}

	if (this->N_out * this->n_frames != (int)Y_N2.size())
	{
		std::stringstream message;
		message << "'Y_N2.size()' has to be equal to 'N' * 'n_frames' ('Y_N2.size()' = " << Y_N2.size()
		        << ", 'N' = " << this->N_out << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}

	if (this->n_frames != (int)delay.size())
	{
		std::stringstream message;
		message << "'delay.size()' has to be equal to '1' * 'n_frames' ('delay.size()' = " << delay.size()
		        << ", 'N' = " << this->N_out << ", 'n_frames' = " << this->n_frames << ").";
		throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
	}

	this->synchronize(X_N1.data(), Y_N2.data(), delay.data(), frame_id);
}

template <typename R>
void Synchronizer_frame<R>::
synchronize(const R *X_N1, R *Y_N2, int* delay, const int frame_id)
{
	const auto f_start = (frame_id < 0) ? 0 : frame_id % this->n_frames;
	const auto f_stop  = (frame_id < 0) ? this->n_frames : f_start +1;

	for (auto f = f_start; f < f_stop; f++)
		this->_synchronize(X_N1 + f * this->N_in,
		                   Y_N2 + f * this->N_out,
		                   delay + f,
		                   f);
}

template <typename R>
void Synchronizer_frame<R>::
_synchronize(const R *X_N1, R *Y_N2, int* delay, const int frame_id)
{
	throw tools::unimplemented_error(__FILE__, __LINE__, __func__);
}

}
}

#endif
