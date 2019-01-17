#include <test.token/test.token.hpp>

namespace test
{
    void token::claim( name holder_name )
    {
        require_auth( holder_name );

        auto stakemine = "stakemine112"_n;

        // call stakemine refresh action
        action(
            permission_level({ _self, "active"_n }),
            stakemine,
            "refresh"_n,
            std::make_tuple( _self )
        ).send();

        // initialize holder
        holders holders_table( stakemine, _self.value );
        auto holder = holders_table.find( holder_name.value );
        eosio_assert( holder != holders_table.end(), "holder not found" );

        // initialize listing
        listings listings_table( stakemine, stakemine.value );
        auto listing = listings_table.find( _self.value );
        eosio_assert( listing != listings_table.end(), "listing not found" );

        // calculate reward
        time_point_sec _now = time_point_sec( now() );
        uint64_t offset = ( _now - holder->request_time ).to_seconds();
        uint64_t cpu_total = listing->cpu_total.amount ?: 1;
        uint64_t net_total = listing->net_total.amount ?: 1;
        uint64_t total_cpu_reward =
            cpu_total <= listing->cpu_target.amount ?
            listing->cpu_reward.amount * cpu_total / listing->cpu_target.amount :
            listing->cpu_reward.amount;
        uint64_t total_net_reward =
            net_total <= listing->net_target.amount ?
            listing->net_reward.amount * net_total / listing->net_target.amount :
            listing->net_reward.amount;
        uint64_t holder_cpu_reward =
            ( (holder->cpu_weight.amount * 100 / cpu_total ) * total_cpu_reward * offset / listing->period ) / 100;
        uint64_t holder_net_reward =
            ( (holder->net_weight.amount * 100 / net_total ) * total_net_reward * offset / listing->period ) / 100;
        uint64_t holder_reward = holder_cpu_reward + holder_net_reward;
        asset reward = asset( holder_reward, listing->cpu_reward.symbol );

        // initialize stats object
        auto raw_code = listing->cpu_reward.symbol.code().raw();
        stats statstable( _self, raw_code );
        auto existing = statstable.find( raw_code );
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
        add_balance( holder->holder, reward, holder->holder );

        // update staking time of holder, to properly calculate future rewards
        action(
            permission_level({ _self, "active"_n }),
            stakemine,
            "update"_n,
            std::make_tuple(
                _self,
                holder_name,
                _now
            )
        ).send();
    }

}

