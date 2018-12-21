#include <darma.token/darma.token.hpp>

namespace darma
{
    void token::claim( name holder )
    {
        require_auth( holder );

        // check eosio.system delband table for holder
        del_bandwidth_table del_table( "eosio"_n, holder.value );
        auto deleg = del_table.find( _self.value );
        eosio_assert( deleg != del_table.end(), "no delegation found");

        // check token's stake table for holder
        holders holders_table( _self, holder.value );
        auto itr = holders_table.find( holder.value );
        eosio_assert( itr != holders_table.end(), "no staking found" );

        // calculate reward based on staking time
        auto code = symbol_code( SYMBOL_CODE );
        auto _now = time_point_sec( now() );
        auto offset = ( _now - (*itr).request_time ).to_seconds();
        uint64_t aux = PERIOD_REWARD;// * 40 / 100;
        asset reward = asset( aux, symbol( code, SYMBOL_PRECISION ) );

        // initialize stats object
        stats statstable( _self, code.raw() );
        auto existing = statstable.find( code.raw() );
        eosio_assert( existing != statstable.end(), "token with symbol does not exist" );
        const auto& st = *existing;

        // checks if reward is valid
        eosio_assert( reward.is_valid(), "invalid quantity" );
        eosio_assert( reward.amount > 0, "must issue positive quantity" );
        eosio_assert( reward.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

        // issue reward to holder
        statstable.modify( st, same_payer, [&]( auto& s ) {
            s.supply += reward;
        });
        add_balance( holder, reward, holder );

        // update staking time of holder, to properly calculate future rewards
        holders_table.modify( itr, _self, [&]( auto& h ) {
            h.holder = holder;
            h.request_time = _now;
            h.net_quantity = (*deleg).net_weight;
            h.cpu_quantity = (*deleg).cpu_weight;
        });
    }

    void token::stake( name holder )
    {
        require_auth( holder );

        // check eosio.system delband table for holder
        del_bandwidth_table del_table( "eosio"_n, holder.value );
        auto deleg = del_table.find( _self.value );
        eosio_assert( deleg != del_table.end(), "no delegation found" );

        // insert holder in the token's stake table
        holders holders_table( _self, holder.value );
        auto itr = holders_table.find( holder.value );
        if( itr == holders_table.end() ) {
            holders_table.emplace( _self, [&]( auto& h ) {
                h.holder = holder;
                h.request_time = time_point_sec( now() );
                h.net_quantity = (*deleg).net_weight;
                h.cpu_quantity = (*deleg).cpu_weight;
            });
        } else {
            holders_table.modify( itr, _self, [&]( auto& h ) {
                h.holder = holder;
                h.request_time = time_point_sec( now() );
                h.net_quantity = (*deleg).net_weight;
                h.cpu_quantity = (*deleg).cpu_weight;
            });
        }
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

