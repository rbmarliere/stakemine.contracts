#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/time.hpp>
#include <string>
#include "stakemine.deleg.hpp"

namespace stakemine
{
    using namespace eosio;
    using namespace eosiosystem;
    using std::string;

    class [[eosio::contract("stakemine.token")]] token : public contract
    {
        public:
            using contract::contract;

            [[eosio::action]]
            void claim( name holder );

            [[eosio::action]]
            void stake( name holder );

            [[eosio::action]]
            void unstake( name holder );

            [[eosio::action]]
            void create( name  issuer,
                         asset maximum_supply );

            [[eosio::action]]
            void issue( name   to,
                        asset  quantity,
                        string memo );

            [[eosio::action]]
            void retire( asset  quantity,
                         string memo );

            [[eosio::action]]
            void transfer( name   from,
                           name   to,
                           asset  quantity,
                           string memo );

            [[eosio::action]]
            void open( name          owner,
                       const symbol& symbol,
                       name          ram_payer );

            [[eosio::action]]
            void close( name          owner,
                        const symbol& symbol );

            static asset get_supply( name        token_contract_account,
                                     symbol_code sym_code )
            {
                stats statstable( token_contract_account, sym_code.raw() );
                const auto& st = statstable.get( sym_code.raw() );
                return st.supply;
            }

            static asset get_balance( name        token_contract_account,
                                      name        owner,
                                      symbol_code sym_code )
            {
                accounts accountstable( token_contract_account, owner.value );
                const auto& ac = accountstable.get( sym_code.raw() );
                return ac.balance;
            }

        private:
            void sub_balance( name  owner,
                              asset value );

            void add_balance( name  owner,
                              asset value,
                              name  ram_payer );

            struct [[eosio::table]] account
            {
                asset balance;

                uint64_t primary_key()const { return balance.symbol.code().raw(); }
            };
            typedef eosio::multi_index< "accounts"_n, account > accounts;

            struct [[eosio::table]] currency_stats
            {
                asset supply;
                asset max_supply;
                name  issuer;

                uint64_t primary_key()const { return supply.symbol.code().raw(); }
            };
            typedef eosio::multi_index< "stat"_n, currency_stats > stats;

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
    };
}

