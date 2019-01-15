#include <stakemine.token/stakemine.token.hpp>

namespace stakemine
{
    void token::stake( name contract,
                       name holder )
    {
        require_auth( holder );

        // check if listing exists
        listings listings_table( _self, _self.value );
        auto listing = listings_table.find( contract.value );
        eosio_assert( listing != listings_table.end(), "listing not found" );

        // check eosio.system delband table for holder
        del_bandwidth_table del_table( "eosio"_n, holder.value );
        auto deleg = del_table.find( contract.value );
        eosio_assert( deleg != del_table.end(), "delegation not found" );

        // insert holder in the token's stake table
        holders holders_table( _self, contract.value );
        auto itr = holders_table.find( holder.value );
        if( itr == holders_table.end() ) {
            holders_table.emplace( _self, [&]( auto& h ) {
                h.holder       = holder;
                h.contract     = contract;
                h.cpu_weight   = deleg->cpu_weight;
                h.net_weight   = deleg->net_weight;
                h.request_time = time_point_sec( now() );
            });

            // increment listing totals
            listings_table.modify( listing, _self, [&]( auto& l ) {
                l.cpu_total += deleg->cpu_weight;
                l.net_total += deleg->net_weight;
            });
        } else {
            auto cpu_offset = listing->cpu_total - itr->cpu_weight;
            auto net_offset = listing->net_total - itr->net_weight;

            holders_table.modify( itr, _self, [&]( auto& h ) {
                h.holder       = holder;
                h.contract     = contract;
                h.cpu_weight   = deleg->cpu_weight;
                h.net_weight   = deleg->net_weight;
                h.request_time = time_point_sec( now() );
            });

            // increment listing totals
            listings_table.modify( listing, _self, [&]( auto& l ) {
                l.cpu_total = cpu_offset + deleg->cpu_weight;
                l.net_total = net_offset + deleg->net_weight;
            });
        }

        // todo: stakemine's token mining
    }

    void token::unstake( name holder )
    {
        require_auth( holder );

        // remove holder from stake table
        holders holders_table( _self, holder.value );
        auto itr = holders_table.find( holder.value );
        if( itr != holders_table.end() )
            holders_table.erase( itr );
    }

}

