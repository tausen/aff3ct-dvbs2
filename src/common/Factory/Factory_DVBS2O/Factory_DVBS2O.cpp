#include "Factory_DVBS2O.hpp"

#include "Module/Encoder_BCH_DVBS2O/Encoder_BCH_DVBS2O.hpp"
#include "Module/Decoder_BCH_DVBS2O/Decoder_BCH_DVBS2O.hpp"

template <typename B>
module::Source<B>* Factory_DVBS2O
::build_source(const Params_DVBS2O& params, const int seed)
{
	if (params.src_type == "RAND")
		return new module::Source_random_fast<B>(params.K_bch, seed);
	else if (params.src_type == "USER")
		return new module::Source_user<B>(params.K_bch,params.src_path);
	else if (params.src_type == "USER_BIN")
		return new module::Source_user_binary<B>(params.K_bch,params.src_path);
	else if (params.src_type == "AZCW")
		return new module::Source_AZCW<B>(params.K_bch);
	else
		throw tools::cannot_allocate(__FILE__, __LINE__, __func__, "Wrong Source type.");

}

template <typename B>
module::Sink<B>* Factory_DVBS2O
::build_sink(const Params_DVBS2O& params)
{
	return new module::Sink_binary<B>(params.K_bch, params.sink_path);
}

template <typename B>
module::Encoder_BCH<B>* Factory_DVBS2O
::build_bch_encoder(const Params_DVBS2O& params, tools::BCH_polynomial_generator<B>& poly_gen)
{
	return new module::Encoder_BCH_DVBS2O<B>(params.K_bch, params.N_bch, poly_gen, 1);
}

template <typename B,typename Q>
module::Decoder_BCH_std<B,Q>* Factory_DVBS2O
::build_bch_decoder(const Params_DVBS2O& params, tools::BCH_polynomial_generator<B>& poly_gen)
{
	return new module::Decoder_BCH_DVBS2O<B,Q>(params.K_bch, params.N_bch, poly_gen, 1);
}

template <typename B,typename Q>
module::Codec_LDPC<B,Q>* Factory_DVBS2O
::build_ldpc_cdc(const Params_DVBS2O& params)
{
	factory::Codec_LDPC p_cdc;
	auto enc_ldpc = dynamic_cast<factory::Encoder_LDPC*>(p_cdc.enc.get());
	auto dec_ldpc = dynamic_cast<factory::Decoder_LDPC*>(p_cdc.dec.get());

	// store parameters
	enc_ldpc->type          = "LDPC_DVBS2";
	dec_ldpc->type          = "BP_HORIZONTAL_LAYERED";
	dec_ldpc->simd_strategy = params.ldpc_simd;
	dec_ldpc->implem        = params.ldpc_implem;
	enc_ldpc->N_cw          = params.N_ldpc;
	enc_ldpc->K             = params.N_bch;
	dec_ldpc->N_cw          = p_cdc.enc->N_cw;
	dec_ldpc->K             = p_cdc.enc->K;
	dec_ldpc->n_frames      = p_cdc.enc->n_frames;
	enc_ldpc->R             = (float)p_cdc.enc->K / (float)p_cdc.enc->N_cw;
	dec_ldpc->R             = (float)p_cdc.dec->K / (float)p_cdc.dec->N_cw;
	dec_ldpc->n_ite         = params.ldpc_nite;
	p_cdc.K                 = p_cdc.enc->K;
	p_cdc.N_cw              = p_cdc.enc->N_cw;
	p_cdc.N                 = p_cdc.N_cw;
	// build ldpc codec
	return p_cdc.build();
}

template <typename D>
tools::Interleaver_core<D>* Factory_DVBS2O
::build_itl_core(const Params_DVBS2O& params)
{
	if (params.modcod == "QPSK-S_8/9" || params.modcod == "QPSK-S_3/5")
		return new tools::Interleaver_core_NO<D>(params.N_ldpc);
	else // "8PSK-S_8/9" "8PSK-S_3/5" "16APSK-S_8/9"
		return new tools::Interleaver_core_column_row<D>(params.N_ldpc, params.itl_n_cols, params.read_order);
}

template <typename D, typename T>
module::Interleaver<D,T>* Factory_DVBS2O
::build_itl(const Params_DVBS2O& params, tools::Interleaver_core<T>& itl_core)
{
	 return new module::Interleaver<D,T>(itl_core);
}

template <typename B, typename R, typename Q, tools::proto_max<Q> MAX>
module::Modem_generic<B,R,Q,MAX>* Factory_DVBS2O
::build_modem(const Params_DVBS2O& params, std::unique_ptr<tools::Constellation<R>> cstl)
{
	 return new module::Modem_generic<B,R,Q,MAX>(params.N_ldpc, std::move(cstl), tools::Sigma<R >(1.0), false, 1);
}

template <typename R>
module::Framer<R>* Factory_DVBS2O
::build_framer(const Params_DVBS2O& params)
{
	return new module::Framer<R>(2 * params.N_ldpc / params.bps, 2 * params.pl_frame_size, params.modcod);
}

template <typename B>
module::Scrambler_BB<B>* Factory_DVBS2O
::build_bb_scrambler(const Params_DVBS2O& params)
{
	return new module::Scrambler_BB<B>(params.K_bch);
}

template <typename R>
module::Scrambler_PL<R>* Factory_DVBS2O
::build_pl_scrambler(const Params_DVBS2O& params)
{
	return new module::Scrambler_PL<R>(2*params.pl_frame_size, params.M);
}

template <typename R>
module::Filter_UPRRC_ccr_naive<R>* Factory_DVBS2O
::build_uprrc_filter(const Params_DVBS2O& params)
{
	return new module::Filter_UPRRC_ccr_naive<float>(params.pl_frame_size * 2,
	                                                 params.rolloff,
	                                                 params.osf,
	                                                 params.grp_delay);
}

template <typename R>
module::Filter_Farrow_ccr_naive<R>* Factory_DVBS2O
::build_channel_delay(const Params_DVBS2O& params)
{
	return new module::Filter_Farrow_ccr_naive <float>(params.pl_frame_size * 2 * params.osf,
	                                                   params.max_delay);
}

template <typename R>
module::Filter_RRC_ccr_naive<R>* Factory_DVBS2O
::build_matched_filter(const Params_DVBS2O& params)
{
	return new module::Filter_RRC_ccr_naive<float>(params.osf * params.pl_frame_size * 2,
	                                               params.rolloff,
	                                               params.osf,
	                                               params.grp_delay);
}

template <typename R>
module::Estimator<R>* Factory_DVBS2O
::build_estimator(const Params_DVBS2O& params)
{
	return new module::Estimator<R>(2 * params.N_xfec_frame);
}



template <typename B>
module::Monitor_BFER<B>* Factory_DVBS2O
::build_monitor(const Params_DVBS2O& params)
{
	// BEGIN DEBUG: ensure that the monitor is making the reduction each time is_done_all() is called
	// Please comment the two following line to increase the simulation throughput
	auto freq = std::chrono::milliseconds(0);

	tools::Monitor_reduction::set_reduce_frequency(freq);
	// END DEBUG

	return new module::Monitor_BFER<B>(params.K_bch, params.max_fe);
}

template <typename R>
module::Channel<R>* Factory_DVBS2O
::build_channel(const Params_DVBS2O& params, const int seed)
{
	std::unique_ptr<tools::Gaussian_noise_generator<R>> n = nullptr;
	n.reset(new tools::Gaussian_noise_generator_fast<R>(seed));
	if (params.filtered)
		return new module::Channel_AWGN_LLR<R>(2 * params.pl_frame_size * params.osf, std::move(n));
	else
		return new module::Channel_AWGN_LLR<R>(2 * params.pl_frame_size             , std::move(n));
}

template <typename R>
module::Multiplier_sine_ccc_naive<R>* Factory_DVBS2O
::build_freq_shift(const Params_DVBS2O& params)
{
	return new module::Multiplier_sine_ccc_naive<R>(2*params.pl_frame_size * params.osf, params.max_freq_shift);
}

template <typename R>
module::Synchronizer_LR_cc_naive<R>* Factory_DVBS2O
::build_synchronizer_lr(const Params_DVBS2O& params)
{
	return new module::Synchronizer_LR_cc_naive<R>(2 * params.pl_frame_size, params.pilot_values, params.pilot_start);
}

template <typename R>
module::Synchronizer_fine_pf_cc_DVBS2O<R>* Factory_DVBS2O
::build_synchronizer_fine_pf(const Params_DVBS2O& params)
{
	return new module::Synchronizer_fine_pf_cc_DVBS2O<R>(2 * params.pl_frame_size, params.pilot_values, params.pilot_start);
}

template <typename R>
module::Synchronizer_Gardner_cc_naive<R>* Factory_DVBS2O
::build_synchronizer_gardner(const Params_DVBS2O& params)
{
	return new module::Synchronizer_Gardner_cc_naive<R>(2 * params.pl_frame_size * params.osf, params.osf);
}

template <typename R>
module::Multiplier_AGC_cc_naive<R>* Factory_DVBS2O
::build_agc_shift(const Params_DVBS2O& params)
{
	return new module::Multiplier_AGC_cc_naive<R>(2 * params.pl_frame_size);
}

template <typename R>
module::Synchronizer_frame_cc_naive<R>* Factory_DVBS2O
::build_synchronizer_frame(const Params_DVBS2O& params)
{
	return new module::Synchronizer_frame_cc_naive<R>(2 * params.pl_frame_size);
}

template <typename R>
module::Synchronizer_coarse_freq<R>* Factory_DVBS2O
::build_synchronizer_coarse_freq(const Params_DVBS2O& params)
{
	if(params.no_pll)
		return (module::Synchronizer_coarse_freq<R> *)(new module::Synchronizer_coarse_freq_NO    <R>(2 * params.pl_frame_size * params.osf));
	else
		return (module::Synchronizer_coarse_freq<R> *)(new module::Synchronizer_coarse_freq_DVBS2O<R>(2 * params.pl_frame_size * params.osf, params.osf, 0.707, 1e-4));
}

template <typename B>
module::Filter_unit_delay<B>* Factory_DVBS2O
::build_unit_delay(const Params_DVBS2O& params)
{
	return new module::Filter_unit_delay<B>(params.K_bch);
}

template <typename R>
module::Synchronizer_step_mf_cc<R>* Factory_DVBS2O
::build_synchronizer_step_mf_cc(aff3ct::module::Synchronizer_coarse_freq<R>         *sync_coarse_f,
	                            aff3ct::module::Filter_RRC_ccr_naive<R>             *matched_filter,
	                            aff3ct::module::Synchronizer_Gardner_cc_naive<R>    *sync_gardner)
{
	return new module::Synchronizer_step_mf_cc<R>(sync_coarse_f, matched_filter, sync_gardner);
}

template <typename R>
module::Radio<R>* Factory_DVBS2O
::build_radio (const Params_DVBS2O& params)
{
	return params.p_rad.build<R>();
}

template aff3ct::module::Source<B>*                            Factory_DVBS2O::build_source<B>                  (const Params_DVBS2O& params, const int seed);
template aff3ct::module::Sink<B>*                              Factory_DVBS2O::build_sink<B>                    (const Params_DVBS2O& params);
template aff3ct::module::Encoder_BCH<B>*                       Factory_DVBS2O::build_bch_encoder<B>             (const Params_DVBS2O& params, tools::BCH_polynomial_generator<B>& poly_gen);
template aff3ct::module::Decoder_BCH_std<B>*                   Factory_DVBS2O::build_bch_decoder<B>             (const Params_DVBS2O& params, tools::BCH_polynomial_generator<B>& poly_gen);
template aff3ct::module::Codec_LDPC<B,Q>*                      Factory_DVBS2O::build_ldpc_cdc<B,Q>              (const Params_DVBS2O& params);
template aff3ct::module::Interleaver<int32_t,uint32_t>*        Factory_DVBS2O::build_itl<int32_t,uint32_t>      (const Params_DVBS2O& params, tools::Interleaver_core<uint32_t>& itl_core);
template aff3ct::module::Interleaver<float,uint32_t>*          Factory_DVBS2O::build_itl<float,uint32_t>        (const Params_DVBS2O& params, tools::Interleaver_core<uint32_t>& itl_core);
template aff3ct::module::Modem_generic<B,R,Q,tools::max_star>* Factory_DVBS2O::build_modem                      (const Params_DVBS2O& params, std::unique_ptr<tools::Constellation<R>> cstl);
template aff3ct::module::Framer<R>*                            Factory_DVBS2O::build_framer                     (const Params_DVBS2O& params);
template aff3ct::module::Scrambler_BB<B>*                      Factory_DVBS2O::build_bb_scrambler<B>            (const Params_DVBS2O& params);
template aff3ct::module::Scrambler_PL<R>*                      Factory_DVBS2O::build_pl_scrambler<R>            (const Params_DVBS2O& params);
template aff3ct::module::Filter_UPRRC_ccr_naive<R>*            Factory_DVBS2O::build_uprrc_filter<R>            (const Params_DVBS2O& params);
template aff3ct::module::Filter_Farrow_ccr_naive<R>*           Factory_DVBS2O::build_channel_delay<R>           (const Params_DVBS2O& params);
template aff3ct::module::Filter_RRC_ccr_naive<R>*              Factory_DVBS2O::build_matched_filter<R>          (const Params_DVBS2O& params);
template aff3ct::module::Estimator<R>*                         Factory_DVBS2O::build_estimator<R>               (const Params_DVBS2O& params);
template aff3ct::module::Monitor_BFER<B>*                      Factory_DVBS2O::build_monitor<B>                 (const Params_DVBS2O& params);
template aff3ct::module::Channel<R>*                           Factory_DVBS2O::build_channel<R>                 (const Params_DVBS2O& params, const int seed);
template aff3ct::module::Multiplier_sine_ccc_naive<R>*         Factory_DVBS2O::build_freq_shift<R>              (const Params_DVBS2O& params);
template aff3ct::module::Synchronizer_LR_cc_naive<R>*          Factory_DVBS2O::build_synchronizer_lr<R>         (const Params_DVBS2O& params);
template aff3ct::module::Synchronizer_fine_pf_cc_DVBS2O<R>*    Factory_DVBS2O::build_synchronizer_fine_pf<R>    (const Params_DVBS2O& params);
template aff3ct::module::Synchronizer_Gardner_cc_naive<R>*     Factory_DVBS2O::build_synchronizer_gardner<R>    (const Params_DVBS2O& params);
template aff3ct::module::Multiplier_AGC_cc_naive<R>*           Factory_DVBS2O::build_agc_shift<R>               (const Params_DVBS2O& params);
template aff3ct::module::Synchronizer_frame_cc_naive<R>*       Factory_DVBS2O::build_synchronizer_frame<R>      (const Params_DVBS2O& params);
template aff3ct::module::Synchronizer_coarse_freq<R>*          Factory_DVBS2O::build_synchronizer_coarse_freq<R>(const Params_DVBS2O& params);
template aff3ct::module::Filter_unit_delay<B>*                 Factory_DVBS2O::build_unit_delay<B>              (const Params_DVBS2O& params);
template aff3ct::tools ::Interleaver_core<uint32_t>*           Factory_DVBS2O::build_itl_core<uint32_t>         (const Params_DVBS2O& params);
template aff3ct::module::Synchronizer_step_mf_cc<R>*           Factory_DVBS2O::build_synchronizer_step_mf_cc<R>(
                                                                        aff3ct::module::Synchronizer_coarse_freq<R>         *sync_coarse_f,
                                                                        aff3ct::module::Filter_RRC_ccr_naive<R>             *matched_filter,
                                                                        aff3ct::module::Synchronizer_Gardner_cc_naive<R>    *sync_gardner   );
template aff3ct::module::Radio<R>*                             Factory_DVBS2O::build_radio<R>                   (const Params_DVBS2O& params);
