#include <vector>
#include <iostream>

#include <aff3ct.hpp>
using namespace aff3ct::module;

#include "Sink.hpp"
#include "BB_scrambler.hpp"
#include "../common/PL_scrambler/PL_scrambler.hpp"


#include "Filter/Filter_UPFIR/Filter_UPRRC/Filter_UPRRC_ccr_naive.hpp"

int main(int argc, char** argv)
{
	using namespace aff3ct;

	const std::string mat2aff_file_name = "../build/matlab_to_aff3ct.txt";
	const std::string aff2mat_file_name = "../build/aff3ct_to_matlab.txt";

	Sink sink_to_matlab    (mat2aff_file_name, aff2mat_file_name);

	const int K_BCH = 14232;
	const int N_BCH = 14400;
	const int N_LDPC = 16200;
	const int K_LDPC = N_BCH;
	const int BPS = 2; // QPSK
	const int M = 90; // number of symbols per slot
	const int P = 36; // number of symbols per pilot

	const int N_XFEC_FRAME = N_LDPC / BPS; // number of complex symbols
	const int N_PILOTS = N_XFEC_FRAME / (16*M);
	const int S = N_XFEC_FRAME / 90; // number of slots	
	const int PL_FRAME_SIZE = M*(S+1) + (N_PILOTS*P);

	// Tracer
	tools::Frame_trace<>     tracer            (20, 5, std::cout);

	if (sink_to_matlab.destination_chain_name == "descramble")
	{

		PL_scrambler<float> complex_scrambler(2*PL_FRAME_SIZE, M, false);
		std::vector<float  > SCRAMBLED_PL_FRAME(2*PL_FRAME_SIZE);
		//std::vector<float  > PL_FRAME(2*PL_FRAME_SIZE);
		std::vector<float  > PL_FRAME_OUTPUT(2*PL_FRAME_SIZE);


		std::vector<float> PL_FRAME(2*PL_FRAME_SIZE-2*M);

		

		sink_to_matlab.pull_vector( SCRAMBLED_PL_FRAME );

		PL_FRAME.insert(PL_FRAME.begin(), SCRAMBLED_PL_FRAME.begin(), SCRAMBLED_PL_FRAME.begin()+2*M);

		complex_scrambler.scramble(SCRAMBLED_PL_FRAME, PL_FRAME);
		//tracer.display_real_vector(SCRAMBLED_PL_FRAME);
		//std::copy(PL_FRAME.begin(), PL_FRAME.end(), PL_FRAME_OUTPUT.begin());
		sink_to_matlab.push_vector( PL_FRAME , true);
	}
	else if (sink_to_matlab.destination_chain_name == "demod_decod")
	{
		std::vector<float  > PL_FRAME(2*PL_FRAME_SIZE);
		std::vector<float  > XFEC_FRAME(2*N_XFEC_FRAME);
		
		sink_to_matlab.pull_vector( PL_FRAME );

		PL_FRAME.erase(PL_FRAME.begin(), PL_FRAME.begin() + 2*M); // erase the PLHEADER

		for( int i = 1; i < N_PILOTS+1; i++)
		{
			PL_FRAME.erase(PL_FRAME.begin()+(i*90*16*2), PL_FRAME.begin()+(i*90*16*2)+(36*2) );
		}
		XFEC_FRAME = PL_FRAME;

		float moment2 = 0, moment4 = 0;

		for (int i = 0; i < N_XFEC_FRAME; i++)
		{
			float tmp = XFEC_FRAME[2*i]*XFEC_FRAME[2*i] + XFEC_FRAME[2*i+1]*XFEC_FRAME[2*i+1];
			moment2 += tmp;
			moment4 += tmp*tmp;
		}
		moment2 /= N_XFEC_FRAME;
		moment4 /= N_XFEC_FRAME;
		//std::cout << "mom2=" << moment2 << std::endl;
		//std::cout << "mom4=" << moment4 << std::endl;

		float Se = sqrt( abs(2 * moment2 * moment2 - moment4 ) );
		float Ne = abs( moment2 - Se );
		float SNR_est = 10 * log10(Se / Ne);

		//std::cout << "SNR_est = " << SNR_est << std::endl;

		float pow_tot, pow_sig_util, sigma_n2;

		pow_tot = moment2;
		//pow_sig_util = pow_tot / (1+(Ne/Se));
		//SNR_est = 6.8;
		
		float denom = (+ pow(10, (-1*SNR_est/10)));

		pow_sig_util = pow_tot / (1+denom);
		sigma_n2 = pow_tot - pow_sig_util;

		float H = sqrt(pow_sig_util);

		std::vector<float  > H_vec(2*N_XFEC_FRAME);
		for (int i = 0; i < N_XFEC_FRAME; i++)
		{
			H_vec[2*i] = H;
			H_vec[2*i+1] = 0;
		}

		std::unique_ptr<tools::Constellation<R>> cstl(new tools::Constellation_user<R>("../conf/4QAM_GRAY.mod"));

		//std::cout << "cmpx = " << cstl->is_complex() << std::endl;

		module::Modem_generic<int, float, float, tools::max_star <float>> modulator(N_LDPC, std::move(cstl), tools::Sigma<R >(1.0, 0, 0), false, 1);

		modulator.set_noise(tools::Sigma<float>(sqrt(sigma_n2/2), 0, 0));

		std::vector<float  > LDPC_encoded(N_LDPC);

		modulator.demodulate_wg(H_vec, XFEC_FRAME, LDPC_encoded, 1);


		const std::vector<int > BCH_gen_poly{1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1};

		// buffers to store the data
		std::vector<int  > scrambler_in(K_BCH);	
		std::vector<int  > BCH_encoded(N_BCH);
		std::vector<int  > parity(N_BCH-K_BCH);
		std::vector<int  > msg(K_BCH);

		// Base Band scrambler
		BB_scrambler             my_scrambler;

		auto dvbs2 = tools::build_dvbs2(K_LDPC, N_LDPC);

		tools::Sparse_matrix H_dvbs2;
		H_dvbs2 = build_H(*dvbs2);

		std::vector<uint32_t> info_bits_pos(K_LDPC);

		for(int i = 0; i< K_LDPC; i++)
			info_bits_pos[i] = i;//+N_LDPC-K_LDPC;

		Decoder_LDPC_BP_horizontal_layered_ONMS_inter<int, float> LDPC_decoder(K_LDPC, N_LDPC, 20, H_dvbs2, info_bits_pos);
		//Decoder_LDPC_BP_flooding_SPA<int, float> LDPC_decoder(K_LDPC, N_LDPC, 20, H_dvbs2, info_bits_pos, false, 1);

		std::vector<int  > LDPC_cw(N_LDPC);
		LDPC_decoder.decode_siho_cw(LDPC_encoded, LDPC_cw);
		
		for(int i = 0; i< N_BCH; i++)
			BCH_encoded[i] = LDPC_cw[i];

		// BCH decoding

		tools::BCH_polynomial_generator<int  > poly_gen(16383, 12);
		poly_gen.set_g(BCH_gen_poly);
		Decoder_BCH_std<int> BCH_decoder(K_BCH, N_BCH, poly_gen);

		parity.assign(BCH_encoded.begin()+K_BCH, BCH_encoded.begin()+N_BCH); // retrieve parity
		std::reverse(parity.begin(), parity.end()); // revert parity bits

		msg.assign(BCH_encoded.begin(), BCH_encoded.begin()+K_BCH); // retrieve message
		std::reverse(msg.begin(), msg.end()); // revert msg bits

		BCH_encoded.insert(BCH_encoded.begin(), parity.begin(), parity.end());
		BCH_encoded.insert(BCH_encoded.begin()+(N_BCH-K_BCH), msg.begin(), msg.end());

		BCH_encoded.erase(BCH_encoded.begin()+N_BCH, BCH_encoded.end());

		BCH_decoder.decode_hiho(BCH_encoded, scrambler_in);

		std::reverse(scrambler_in.begin(), scrambler_in.end());

		// BB descrambling
		my_scrambler.scramble(scrambler_in);

		sink_to_matlab.push_vector( scrambler_in , false);
		//sink_to_matlab.push_vector( LDPC_cw , false);

	}

	
	else
	{
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, "Invalid destination name.");
	}

	return 0;
}