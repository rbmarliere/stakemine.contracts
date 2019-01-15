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

    void token::unstake( name contract,
                         name holder )
    {
        require_auth( holder );

        // remove holder from stake table
        holders holders_table( _self, contract.value );
        auto itr = holders_table.find( holder.value );
        eosio_assert( itr != holders_table.end(), "holder not found" );
        asset holder_cpu = itr->cpu_weight;
        asset holder_net = itr->net_weight;
        holders_table.erase( itr );

        // check if listing exists
        listings listings_table( _self, _self.value );
        auto listing = listings_table.find( contract.value );
        eosio_assert( listing != listings_table.end(), "listing not found" );

        // decrement listing totals
        listings_table.modify( listing, _self, [&]( auto& l ) {
            l.cpu_total -= holder_cpu;
            l.net_total -= holder_net;
        });
    }

    void token::list( name     contract,
                      string   description,
                      string   url,
                      string   image_url,
                      uint64_t period,
                      asset    cpu_target,
                      asset    cpu_reward,
                      asset    net_target,
                      asset    net_reward )
    {
        require_auth( _self );

        class symbol eos( symbol_code("EOS"), 4 );
        asset zero = asset( 0, eos );

        listings listings_table( _self, _self.value );
        auto itr = listings_table.find( contract.value );
        if( itr == listings_table.end() )
            listings_table.emplace( _self, [&]( auto& l ) {
                l.contract    = contract;
                l.description = description;
                l.url         = url;
                l.image_url   = image_url;
                l.period      = period;
                l.cpu_target  = cpu_target;
                l.cpu_reward  = cpu_reward;
                l.cpu_total   = zero;
                l.net_target  = net_target;
                l.net_reward  = net_reward;
                l.net_total   = zero;
            });
        else
            listings_table.modify( itr, _self, [&]( auto& l ) {
                l.contract    = contract;
                l.description = description;
                l.url         = url;
                l.image_url   = image_url;
                l.period      = period;
                l.cpu_target  = cpu_target;
                l.cpu_reward  = cpu_reward;
                l.net_target  = net_target;
                l.net_reward  = net_reward;
            });
    }

    void token::delist( name contract )
    {
        require_auth( _self );

        // check if listing exists
        listings listings_table( _self, _self.value );
        auto listing = listings_table.find( contract.value );
        eosio_assert( listing != listings_table.end(), "listing not found" );

        // delete listing
        listings_table.erase( listing );

        // delete all holders associated
        vector<uint64_t> holders_to_delete;
        holders holders_table( _self, contract.value );
        for( auto& holder : holders_table ) {
            if( holder.contract == contract )
                holders_to_delete.push_back( holder.holder.value );
        }
        for( uint64_t holder : holders_to_delete ) {
            auto itr = holders_table.find( holder );
            if( itr != holders_table.end() )
                holders_table.erase( itr );
        }
    }

    void token::refresh( name contract )
    {
        require_auth( contract );

        // check if listing exists
        listings listings_table( _self, _self.value );
        auto listing = listings_table.find( contract.value );
        eosio_assert( listing != listings_table.end(), "listing not found" );

        // check if there are holders
        holders holders_table( _self, contract.value );
        eosio_assert( holders_table.begin() != holders_table.end(), "holder not found" );

        // check eosio.system delband table for each holder
        vector<uint64_t> holders_to_delete;
        for( auto& holder : holders_table ) {
            del_bandwidth_table del_table( "eosio"_n, holder.holder.value );
            auto deleg = del_table.find( contract.value );
            if(
                deleg == del_table.end()
                ||
                deleg->cpu_weight != holder.cpu_weight
                ||
                deleg->net_weight != holder.net_weight
            ) {
                // if user stopped or changed delegation, erase it
                holders_to_delete.push_back( holder.holder.value );
            }
        }
        for( uint64_t holder : holders_to_delete ) {
            auto itr = holders_table.find( holder );
            if( itr != holders_table.end() ) {
                asset holder_cpu = itr->cpu_weight;
                asset holder_net = itr->net_weight;

                holders_table.erase( itr );

                // decrement listing totals
                listings_table.modify( listing, _self, [&]( auto& l ) {
                    l.cpu_total -= holder_cpu;
                    l.net_total -= holder_net;
                });
            }
        }
    }

    void token::update( name           contract,
                        name           holder,
                        time_point_sec request_time )
    {
        require_auth( contract );

        // check if listing exists
        listings listings_table( _self, _self.value );
        auto listing = listings_table.find( contract.value );
        eosio_assert( listing != listings_table.end(), "listing not found" );

        // check if holder exists
        holders holders_table( _self, contract.value );
        auto itr = holders_table.find( holder.value );
        eosio_assert( itr != holders_table.end(), "holder not found" );

        // update holder request_time
        holders_table.modify( itr, _self, [&]( auto& h ) {
            h.request_time = request_time;
        });
    }

}

