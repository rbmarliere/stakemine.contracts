#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/time.hpp>

namespace stakemine
{
    using namespace eosio;
    using std::string;

    struct [[eosio::table]] bandwidth
    {
        name           holder;
        name           contract;
        asset          cpu_weight;
        asset          net_weight;
        time_point_sec request_time;

        uint64_t primary_key()const { return holder.value; }
    };
    typedef eosio::multi_index< "holders"_n, bandwidth > holders;

    struct [[eosio::table]] listing
    {
        name     contract;
        string   description;
        string   url;
        string   image_url;
        uint64_t period;
        asset    cpu_target;
        asset    cpu_reward;
        asset    cpu_total;
        asset    net_target;
        asset    net_reward;
        asset    net_total;

        uint64_t primary_key()const { return contract.value; }
    };
    typedef eosio::multi_index< "listing"_n, listing > listings;
}

