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

    void token::create( name   issuer,
                        asset  maximum_supply )
    {
        require_auth( _self );

        auto sym = maximum_supply.symbol;
        eosio_assert( sym.is_valid(), "invalid symbol name" );
        eosio_assert( maximum_supply.is_valid(), "invalid supply");
        eosio_assert( maximum_supply.amount > 0, "max-supply must be positive");

        stats statstable( _self, sym.code().raw() );
        auto existing = statstable.find( sym.code().raw() );
        eosio_assert( existing == statstable.end(), "token with symbol already exists" );

        statstable.emplace( _self, [&]( auto& s ) {
            s.supply.symbol = maximum_supply.symbol;
            s.max_supply    = maximum_supply;
            s.issuer        = issuer;
        });
    }


    void token::issue( name to, asset quantity, string memo )
    {
        auto sym = quantity.symbol;
        eosio_assert( sym.is_valid(), "invalid symbol name" );
        eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

        stats statstable( _self, sym.code().raw() );
        auto existing = statstable.find( sym.code().raw() );
        eosio_assert( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
        const auto& st = *existing;

        require_auth( st.issuer );
        eosio_assert( quantity.is_valid(), "invalid quantity" );
        eosio_assert( quantity.amount > 0, "must issue positive quantity" );

        eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
        eosio_assert( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

        statstable.modify( st, same_payer, [&]( auto& s ) {
            s.supply += quantity;
        });

        add_balance( st.issuer, quantity, st.issuer );

        if( to != st.issuer ) {
            SEND_INLINE_ACTION( *this, transfer, { {st.issuer, "active"_n} },
                                { st.issuer, to, quantity, memo } );
        }
    }

    void token::retire( asset quantity, string memo )
    {
        auto sym = quantity.symbol;
        eosio_assert( sym.is_valid(), "invalid symbol name" );
        eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

        stats statstable( _self, sym.code().raw() );
        auto existing = statstable.find( sym.code().raw() );
        eosio_assert( existing != statstable.end(), "token with symbol does not exist" );
        const auto& st = *existing;

        require_auth( st.issuer );
        eosio_assert( quantity.is_valid(), "invalid quantity" );
        eosio_assert( quantity.amount > 0, "must retire positive quantity" );

        eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

        statstable.modify( st, same_payer, [&]( auto& s ) {
            s.supply -= quantity;
        });

        sub_balance( st.issuer, quantity );
    }

    void token::transfer( name    from,
                          name    to,
                          asset   quantity,
                          string  memo )
    {
        eosio_assert( from != to, "cannot transfer to self" );
        require_auth( from );
        eosio_assert( is_account( to ), "to account does not exist");
        auto sym = quantity.symbol.code();
        stats statstable( _self, sym.raw() );
        const auto& st = statstable.get( sym.raw() );

        require_recipient( from );
        require_recipient( to );

        eosio_assert( quantity.is_valid(), "invalid quantity" );
        eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
        eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
        eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

        auto payer = has_auth( to ) ? to : from;

        sub_balance( from, quantity );
        add_balance( to, quantity, payer );
    }

    void token::sub_balance( name owner, asset value )
    {
        accounts from_acnts( _self, owner.value );

        const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found" );
        eosio_assert( from.balance.amount >= value.amount, "overdrawn balance" );

        from_acnts.modify( from, owner, [&]( auto& a ) {
            a.balance -= value;
        });
    }

    void token::add_balance( name owner, asset value, name ram_payer )
    {
        accounts to_acnts( _self, owner.value );
        auto to = to_acnts.find( value.symbol.code().raw() );
        if( to == to_acnts.end() ) {
            to_acnts.emplace( ram_payer, [&]( auto& a ){
                a.balance = value;
            });
        } else {
            to_acnts.modify( to, same_payer, [&]( auto& a ) {
                a.balance += value;
            });
        }
    }

    void token::open( name owner, const symbol& symbol, name ram_payer )
    {
        require_auth( ram_payer );

        auto sym_code_raw = symbol.code().raw();

        stats statstable( _self, sym_code_raw );
        const auto& st = statstable.get( sym_code_raw, "symbol does not exist" );
        eosio_assert( st.supply.symbol == symbol, "symbol precision mismatch" );

        accounts acnts( _self, owner.value );
        auto it = acnts.find( sym_code_raw );
        if( it == acnts.end() ) {
            acnts.emplace( ram_payer, [&]( auto& a ){
                a.balance = asset{0, symbol};
            });
        }
    }

    void token::close( name owner, const symbol& symbol )
    {
        require_auth( owner );
        accounts acnts( _self, owner.value );
        auto it = acnts.find( symbol.code().raw() );
        eosio_assert( it != acnts.end(), "Balance row already deleted or never existed. Action won't have any effect." );
        eosio_assert( it->balance.amount == 0, "Cannot close because the balance is not zero." );
        acnts.erase( it );
    }
}

EOSIO_DISPATCH( darma::token, (claim)(stake)(create)(issue)(transfer)(open)(close)(retire) )

