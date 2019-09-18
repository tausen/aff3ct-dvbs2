#ifndef FACTORY_RADIO_USRP_HPP_
#define FACTORY_RADIO_USRP_HPP_

#include <string>
#include <memory>
#include <aff3ct.hpp>

#include "Radio/Radio_USRP/Radio_USRP.hpp"

namespace aff3ct
{
namespace factory
{
extern const std::string Radio_USRP_name;
extern const std::string Radio_USRP_prefix;
struct Radio_USRP : Factory
{
	class parameters : public Factory::parameters
	{
	public:
		// ------------------------------------------------------------------------------------------------- PARAMETERS
		// required parameters

		// optional parameters
		int N                      = 0;
		int n_frames               = 1;
		double clk_rate            = 125e6;

		std::string rx_subdev_spec = "A:0";
		double rx_rate             = 8e6;
		double rx_freq             = 1090e6;
		double rx_gain             = 10;


		std::string tx_subdev_spec = "A:0";
		double tx_rate             = 8e6;
		double tx_freq             = 1090e6;
		double tx_gain             = 10;

		std::string usrp_addr      = "192.168.20.2";
		// deduced parameters

		// ---------------------------------------------------------------------------------------------------- METHODS
		explicit parameters(const std::string &prefix = Radio_USRP_prefix);
		virtual ~parameters() = default;
		virtual Radio_USRP::parameters* clone() const;

		// parameters construction
		virtual void get_description(tools::Argument_map_info &args) const;
		virtual void store          (const tools::Argument_map_value &vals);
		virtual void get_headers    (std::map<std::string,header_list>& headers, const bool full = true) const;


		template <typename D = double>
		module::Radio<D>* build() const;
	};

	template <typename D = double>
	static module::Radio<D>* build(const parameters &params);
};
}
}

#endif /* FACTORY_RADIO_USRP_HPP_ */

