#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>

namespace eosiosystem
{
    using namespace eosio;

    struct delegated_bandwidth
    {
        name  from;
        name  to;
        asset net_weight;
        asset cpu_weight;

        uint64_t primary_key()const { return from.value; }
    };
    typedef eosio::multi_index< "delband"_n, delegated_bandwidth > del_bandwidth_table;
}

