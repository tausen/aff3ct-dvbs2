#include <cassert>
#include <iostream>

#include "Module/Synchronizer/Synchronizer_timing/Synchronizer_Gardner_fast.hpp"

// _USE_MATH_DEFINES does not seem to work on MSVC...
#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

using namespace aff3ct::module;

template <typename B, typename R>
Synchronizer_Gardner_fast<B,R>
::Synchronizer_Gardner_fast(const int N, int osf, const R damping_factor, const R normalized_bandwidth, const R detector_gain, const int n_frames)
: Synchronizer_timing<B,R>(N, osf, n_frames),
farrow_flt(N,(R)0),
strobe_history(0),
TED_error((R)0),
TED_buffer(osf, std::complex<R>((R)0,(R)0)),
TED_head_pos(osf - 1),
TED_mid_pos((osf - 1 - osf / 2) % osf),
lf_proportional_gain((R)0),
lf_integrator_gain   ((R)0),
lf_prev_in ((R)0),
lf_filter_state ((R)0),
lf_output((R)0),
NCO_counter((R)0)
{
	this->set_loop_filter_coeffs(damping_factor, normalized_bandwidth, detector_gain);
	// std::cerr << "# Gardner integrator_gain   = " << this->lf_integrator_gain << std::endl;
	// std::cerr << "# Gardner proportional_gain = " << this->lf_proportional_gain << std::endl;
}

template <typename B, typename R>
Synchronizer_Gardner_fast<B,R>
::~Synchronizer_Gardner_fast()
{}

template <typename B, typename R>
void Synchronizer_Gardner_fast<B,R>
::_synchronize(const R *X_N1, R *Y_N1, B *B_N1, const int frame_id)
{
	auto cX_N1 = reinterpret_cast<const std::complex<R>* >(X_N1);
	auto cY_N1 = reinterpret_cast<      std::complex<R>* >(Y_N1);

	R* TED_buffer_iq    = reinterpret_cast<R* >(this->TED_buffer.data());

	for (auto i = 0; i < this->N_in/2 ; i++)
	{
		R W = this->lf_output + this->INV_osf;
		this->is_strobe = (this->NCO_counter < W);
		this->strobe_history = this->strobe_history << 1;
		this->strobe_history = this->strobe_history % this->POW_osf;
		this->strobe_history = this->strobe_history + this->is_strobe;

		if (this->is_strobe ==1 && this->set_bits_nbr[this->strobe_history] == 1)// Update mu if a strobe
		{
			B_N1[2*i]     = this->is_strobe;
			B_N1[2*i + 1] = this->is_strobe;
			this->mu = this->NCO_counter / W;
			this->farrow_flt.set_mu(this->mu);
			this->NCO_counter += 1.0f;
			this->NCO_counter -= W; // Update counter*/
			farrow_flt.step( &cX_N1[i], &cY_N1[i]);

			this->TED_error = TED_buffer_iq[2*this->TED_mid_pos    ] * (TED_buffer_iq[2*this->TED_head_pos    ] - Y_N1[2*i    ]) +
			                  TED_buffer_iq[2*this->TED_mid_pos + 1] * (TED_buffer_iq[2*this->TED_head_pos + 1] - Y_N1[2*i + 1]);

			this->TED_buffer[this->TED_head_pos] = cY_N1[i];
			this->TED_head_pos = (this->TED_head_pos - 1 + this->osf) % this->osf;
			this->TED_mid_pos  = (this->TED_mid_pos  - 1 + this->osf) % this->osf;
			this->lf_prev_in += this->TED_error * this->lf_integrator_gain;
			this->lf_output = this->lf_prev_in + this->TED_error * this->lf_proportional_gain;
		}
		else if (this->is_strobe ==1 && this->set_bits_nbr[this->strobe_history] > 1)// Update mu if a strobe
		{
			B_N1[2*i]     = this->is_strobe;
			B_N1[2*i + 1] = this->is_strobe;
			this->mu = this->NCO_counter / W;
			this->farrow_flt.set_mu(this->mu);
			this->NCO_counter += 1.0f;
			this->NCO_counter -= W; // Update counter*/
			farrow_flt.step( &cX_N1[i], &cY_N1[i]);

			this->TED_error = 0.0f;
			this->TED_buffer[ this->TED_head_pos               ] = std::complex<R>(0.0f, 0.0f);
			this->TED_buffer[(this->TED_head_pos - 1 + this->osf)%this->osf] = cY_N1[i];

			this->TED_head_pos     = (this->TED_head_pos - 2 + this->osf) % this->osf;
			this->TED_mid_pos      = (this->TED_mid_pos  - 2 + this->osf) % this->osf;
			this->lf_prev_in += this->TED_error * this->lf_integrator_gain;
			this->lf_output = this->lf_prev_in + this->TED_error * this->lf_proportional_gain;
		}
		else if (this->is_strobe == 0 && this->set_bits_nbr[this->strobe_history] == 1)// Update mu if a strobe
		{
			B_N1[2*i]     = this->is_strobe;
			B_N1[2*i + 1] = this->is_strobe;
			this->NCO_counter -= W; // Update counter*/
			farrow_flt.step( &cX_N1[i], &cY_N1[i]);
			this->TED_error = 0.0f;
			this->TED_buffer[this->TED_head_pos] = cY_N1[i];
			this->TED_head_pos = (this->TED_head_pos - 1 + this->osf) % this->osf;
			this->TED_mid_pos  = (this->TED_mid_pos  - 1 + this->osf) % this->osf;
			this->lf_prev_in += this->TED_error * this->lf_integrator_gain;
			this->lf_output = this->lf_prev_in + this->TED_error * this->lf_proportional_gain;
		}
		else if (this->is_strobe == 0 && this->set_bits_nbr[this->strobe_history] > 1)// Update mu if a strobe
		{
			B_N1[2*i]     = this->is_strobe;
			B_N1[2*i + 1] = this->is_strobe;
			this->NCO_counter -= W; // Update counter*/
			farrow_flt.step( &cX_N1[i], &cY_N1[i]);
			this->TED_error = 0.0f;
			this->TED_buffer[ this->TED_head_pos               ] = std::complex<R>(0.0f, 0.0f);
			this->TED_buffer[(this->TED_head_pos - 1 + this->osf)%this->osf] = cY_N1[i];
			this->TED_head_pos     = (this->TED_head_pos - 2 + this->osf) % this->osf;
			this->TED_mid_pos      = (this->TED_mid_pos  - 2 + this->osf) % this->osf;
			this->lf_prev_in += this->TED_error * this->lf_integrator_gain;
			this->lf_output = this->lf_prev_in + this->TED_error * this->lf_proportional_gain;
		}
		else if (this->is_strobe == 0 && this->set_bits_nbr[this->strobe_history] == 0)// Update mu if a strobe
		{
			B_N1[2*i]     = this->is_strobe;
			B_N1[2*i + 1] = this->is_strobe;
			this->NCO_counter -= W; // Update counter*/
			farrow_flt.step( &cX_N1[i], &cY_N1[i]);
			this->lf_prev_in += this->TED_error * this->lf_integrator_gain;
			this->lf_output = this->lf_prev_in + this->TED_error * this->lf_proportional_gain;
		}
	}

}


template <typename B, typename R>
void Synchronizer_Gardner_fast<B,R>
::step(const std::complex<R> *X_N1, std::complex<R> *Y_N1, B *B_N1)
{
	farrow_flt.step( X_N1, Y_N1);
	*B_N1 = this->is_strobe;

	this->last_symbol = (this->is_strobe == 1)?*Y_N1:this->last_symbol;

	this->TED_update(*Y_N1);
	this->loop_filter();
	this->interpolation_control();
}

template <typename B, typename R>
void Synchronizer_Gardner_fast<B,R>
::_reset()
{
	this->mu = (R)0;
	this->farrow_flt.reset();
	this->farrow_flt.set_mu((R)0);

	for (auto i = 0; i<this->osf ; i++)
		this->TED_buffer[i] = std::complex<R>(R(0),R(0));

	this->strobe_history   = 0;
	this->TED_error        = (R)0;
	this->TED_head_pos     = 0;
	this->TED_mid_pos      = this->osf/2;//(osf - 1 - osf / 2) % osf;
	this->lf_prev_in       = (R)0;
	this->lf_filter_state  = (R)0;
	this->lf_output        = (R)0;
	this->NCO_counter      = (R)0;
}

template <typename B, typename R>
void Synchronizer_Gardner_fast<B,R>
::loop_filter()
{
	//this->lf_filter_state += this->lf_prev_in;
	//this->lf_output        = this->TED_error * this->lf_proportional_gain + this->lf_filter_state;
	//this->lf_prev_in       = this->TED_error * this->lf_integrator_gain;

	R vp = this->TED_error * this->lf_proportional_gain;
	R vi = this->lf_prev_in + this->TED_error * this->lf_integrator_gain;
	this->lf_prev_in = vi;
	this->lf_output = vp + vi;
}

template <typename B, typename R>
void Synchronizer_Gardner_fast<B,R>
::interpolation_control()
{
	// Interpolation Control
	R W = this->lf_output + this->INV_osf;
	this->is_strobe = (this->NCO_counter < W) ? 1:0; // Check if a strobe
	if (this->is_strobe == 1) // Update mu if a strobe
	{
		this->mu = this->NCO_counter / W;
		this->farrow_flt.set_mu(this->mu);
	}

	this->NCO_counter = (this->NCO_counter - W) - std::floor(this->NCO_counter - W); // Update counter*/

	//this->is_strobe = ((int)this->NCO_counter % 4 == 0) ? 1:0; // Check if a strobe
	//this->NCO_counter += 1.0f;
	//this->NCO_counter = (R)((int)this->NCO_counter % 4);
}

template <typename B, typename R>
void Synchronizer_Gardner_fast<B,R>
::TED_update(std::complex<R> sample)
{
	this->strobe_history = (this->strobe_history << 1) % this->POW_osf + this->is_strobe;
	if (this->strobe_history == 1)
	{
		this->TED_error = std::real(this->TED_buffer[this->TED_mid_pos]) * (std::real(this->TED_buffer[this->TED_head_pos]) - std::real(sample)) +
		                  std::imag(this->TED_buffer[this->TED_mid_pos]) * (std::imag(this->TED_buffer[this->TED_head_pos]) - std::imag(sample));
	}
	else
		this->TED_error = 0.0f;

	// Stuffing / skipping
	switch (this->set_bits_nbr[this->strobe_history])
	{
		case 0:
		break;

		case 1:
			this->TED_buffer[this->TED_head_pos] = sample;

			this->TED_head_pos = (this->TED_head_pos - 1 + this->osf) % this->osf;
			this->TED_mid_pos  = (this->TED_mid_pos  - 1 + this->osf) % this->osf;
		break;

		default:
			this->TED_buffer[ this->TED_head_pos               ] = std::complex<R>(0.0f, 0.0f);
			this->TED_buffer[(this->TED_head_pos - 1 + this->osf)%this->osf] = sample;

			this->TED_head_pos     = (this->TED_head_pos - 2 + this->osf) % this->osf;
			this->TED_mid_pos      = (this->TED_mid_pos  - 2 + this->osf) % this->osf;
		break;
	}
}

template <typename B, typename R>
void Synchronizer_Gardner_fast<B,R>
::set_loop_filter_coeffs (const R damping_factor, const R normalized_bandwidth, const R detector_gain)
{
	R K0   = -1;
	R theta = normalized_bandwidth/(R)this->osf/(damping_factor + 0.25/damping_factor);
	R d  = (1 + 2*damping_factor*theta + theta*theta) * K0 * detector_gain;

	this->lf_proportional_gain = (4*damping_factor*theta) /d;
	this->lf_integrator_gain   = (4*theta*theta)/d;
}

// ==================================================================================== explicit template instantiation
template class aff3ct::module::Synchronizer_Gardner_fast<int, float>;
template class aff3ct::module::Synchronizer_Gardner_fast<int, double>;
// ==================================================================================== explicit template instantiation
