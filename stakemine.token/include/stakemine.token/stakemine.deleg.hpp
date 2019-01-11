#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>

#define SYMBOL_CODE       "STK"
#define SYMBOL_PRECISION  4
#define STAKE_FACTOR      100  // EOS
#define PERIOD_IN_SEC     3600 // 1h
#define PERIOD_CPU_REWARD 1    // 1 token per STAKE_FACTOR
#define PERIOD_NET_REWARD 1    // 1 token per STAKE_FACTOR

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

