#include "Factory_DVBS2O.hpp"

#include "../Encoder_BCH_DVBS2O/Encoder_BCH_DVBS2O.hpp"
#include "../Decoder_BCH_DVBS2O/Decoder_BCH_DVBS2O.hpp"

template <typename B>
module::Source<B>* Factory_DVBS2O
::build_source(const Params_DVBS2O& params, const int seed)
{
	return new module::Source_random_fast<B>(params.K_BCH, seed);
}

template <typename B>
module::Encoder_BCH<B>* Factory_DVBS2O
::build_bch_encoder(const Params_DVBS2O& params, tools::BCH_polynomial_generator<B>& poly_gen)
{
	return new module::Encoder_BCH_DVBS2O<B>(params.K_BCH, params.N_BCH, poly_gen, 1);
}

template <typename B,typename Q>
module::Decoder_BCH_std<B,Q>* Factory_DVBS2O
::build_bch_decoder(const Params_DVBS2O& params, tools::BCH_polynomial_generator<B>& poly_gen)
{
	return new module::Decoder_BCH_DVBS2O<B,Q>(params.K_BCH, params.N_BCH, poly_gen, 1);
}

template <typename B,typename Q>
module::Codec_LDPC<B,Q>* Factory_DVBS2O
::build_ldpc_cdc(const Params_DVBS2O& params)
{
	factory::Codec_LDPC::parameters p_cdc;
	auto enc_ldpc = dynamic_cast<factory::Encoder_LDPC::parameters*>(p_cdc.enc.get());
	auto dec_ldpc = dynamic_cast<factory::Decoder_LDPC::parameters*>(p_cdc.dec.get());

	// store parameters
	enc_ldpc->type          = "LDPC_DVBS2";
	dec_ldpc->type          = "BP_HORIZONTAL_LAYERED";
	dec_ldpc->simd_strategy = params.LDPC_SIMD;
	dec_ldpc->implem        = params.LDPC_IMPLEM;
	enc_ldpc->N_cw          = params.N_LDPC;
	enc_ldpc->K             = params.N_BCH;
	dec_ldpc->N_cw          = p_cdc.enc->N_cw;
	dec_ldpc->K             = p_cdc.enc->K;
	dec_ldpc->n_frames      = p_cdc.enc->n_frames;
	enc_ldpc->R             = (float)p_cdc.enc->K / (float)p_cdc.enc->N_cw;
	dec_ldpc->R             = (float)p_cdc.dec->K / (float)p_cdc.dec->N_cw;
	dec_ldpc->n_ite         = params.LDPC_NITE;
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
	if (params.MODCOD == "QPSK-S_8/9" || params.MODCOD == "QPSK-S_3/5")
		return new tools::Interleaver_core_NO<D>(params.N_LDPC);
	else // "8PSK-S_8/9" "8PSK-S_3/5" "16APSK-S_8/9"
		return new tools::Interleaver_core_column_row<D>(params.N_LDPC, params.ITL_N_COLS, params.READ_ORDER);
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
	 return new module::Modem_generic<B,R,Q,MAX>(params.N_LDPC, std::move(cstl), tools::Sigma<R >(1.0), false, 1);
}

template <typename R>
module::Framer<R>* Factory_DVBS2O
::build_framer(const Params_DVBS2O& params)
{
	return new module::Framer<R>(2 * params.N_LDPC / params.BPS, 2 * params.PL_FRAME_SIZE, params.MODCOD);
}

template <typename B>
module::Scrambler_BB<B>* Factory_DVBS2O
::build_bb_scrambler(const Params_DVBS2O& params)
{
	return new module::Scrambler_BB<B>(params.K_BCH);
}

template <typename R>
module::Scrambler_PL<R>* Factory_DVBS2O
::build_pl_scrambler(const Params_DVBS2O& params)
{
	return new module::Scrambler_PL<R>(2*params.PL_FRAME_SIZE, params.M);
}

template <typename R>
module::Filter_UPRRC_ccr_naive<R>* Factory_DVBS2O
::build_uprrc_filter(const Params_DVBS2O& params)
{
	return new module::Filter_UPRRC_ccr_naive<float>((params.PL_FRAME_SIZE + params.GRP_DELAY) * 2,
	                                                  params.ROLLOFF,
	                                                  params.OSF,
	                                                  params.GRP_DELAY);
}

template <typename R>
module::Estimator<R>* Factory_DVBS2O
::build_estimator(const Params_DVBS2O& params)
{
	return new module::Estimator<R>(2 * params.N_XFEC_FRAME);
}

template <typename B>
module::Monitor_BFER<B>* Factory_DVBS2O
::build_monitor(const Params_DVBS2O& params)
{
	return new module::Monitor_BFER<B>(params.K_BCH, params.MAX_FE);
}

template <typename R>
module::Channel<R>* Factory_DVBS2O
::build_channel(const Params_DVBS2O& params, const int seed)
{
	std::unique_ptr<tools::Gaussian_noise_generator<R>> n = nullptr;
	n.reset(new tools::Gaussian_noise_generator_fast<R>(seed));
	return new module::Channel_AWGN_LLR<R>(2*params.PL_FRAME_SIZE, std::move(n));
}

template aff3ct::module::Source<B>*                            Factory_DVBS2O::build_source<B>             (const Params_DVBS2O& params, const int seed);
template aff3ct::module::Encoder_BCH<B>*                       Factory_DVBS2O::build_bch_encoder<B>        (const Params_DVBS2O& params, tools::BCH_polynomial_generator<B>& poly_gen);
template aff3ct::module::Decoder_BCH_std<B>*                   Factory_DVBS2O::build_bch_decoder<B>        (const Params_DVBS2O& params, tools::BCH_polynomial_generator<B>& poly_gen);
template aff3ct::module::Codec_LDPC<B,Q>*                      Factory_DVBS2O::build_ldpc_cdc<B,Q>         (const Params_DVBS2O& params);
template aff3ct::tools::Interleaver_core<uint32_t>*            Factory_DVBS2O::build_itl_core<uint32_t>    (const Params_DVBS2O& params);
template aff3ct::module::Interleaver<int32_t,uint32_t>*        Factory_DVBS2O::build_itl<int32_t,uint32_t> (const Params_DVBS2O& params, tools::Interleaver_core<uint32_t>& itl_core);
template aff3ct::module::Interleaver<float,uint32_t>*          Factory_DVBS2O::build_itl<float,uint32_t>   (const Params_DVBS2O& params, tools::Interleaver_core<uint32_t>& itl_core);
template aff3ct::module::Modem_generic<B,R,Q,tools::max_star>* Factory_DVBS2O::build_modem                 (const Params_DVBS2O& params, std::unique_ptr<tools::Constellation<R>> cstl);
template aff3ct::module::Framer<R>*                            Factory_DVBS2O::build_framer                (const Params_DVBS2O& params);
template aff3ct::module::Scrambler_BB<B>*                      Factory_DVBS2O::build_bb_scrambler<B>       (const Params_DVBS2O& params);
template aff3ct::module::Scrambler_PL<R>*                      Factory_DVBS2O::build_pl_scrambler<R>       (const Params_DVBS2O& params);
template aff3ct::module::Filter_UPRRC_ccr_naive<R>*            Factory_DVBS2O::build_uprrc_filter<R>       (const Params_DVBS2O& params);
template aff3ct::module::Estimator<R>*                         Factory_DVBS2O::build_estimator<R>          (const Params_DVBS2O& params);
template aff3ct::module::Monitor_BFER<B>*                      Factory_DVBS2O::build_monitor<B>            (const Params_DVBS2O& params);
template aff3ct::module::Channel<R>*                           Factory_DVBS2O::build_channel<R>            (const Params_DVBS2O& params, const int seed);
