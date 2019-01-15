#!/bin/bash

cleos="cleos -u http://10.197.70.202:8888"

user_account=accountnum13
user2_account=accountnum14

stakemine_account=stakemine112
stakemine_privkey=5JDivYQMTFF53uWp9oJJLAYaw8QLRrhATuVdEQ83Ua8CGLZ9P6G
stakemine_pubkey=EOS5KPP2UW6VN6aQDTHK2AWK9LAJ5i9uwqua5A3zPrrW8wVCvgp6q
stakemine_sym=STAKE

token_account=darmatoken11
token_privkey=5KiK5Ds4LY9159Km5GPWrrR69iEz3HPf6bpUAYKCugFpPoqXcaC
token_pubkey=EOS8aKH63HLph6Z4wMSvZ5sY67wWzY1mAZ8ZfiWBrymSMYnQ7iMm4
token_sym=DRAMA

wallet=local
walletpw=$(cat ~/eosio-wallet/local.passwd)

prompt_input_yN()
{
    printf "$1? [y|N] "
    while true; do
        read YN
        case $YN in
            [Yy]* ) printf "\n"; return 0; break;;
            * ) printf "\n"; return 1; break;;
        esac
    done
}

if [ "$($cleos wallet list | grep $wallet' \*')" = "" ]; then
    $cleos wallet unlock -n $wallet --password=$walletpw
fi

if prompt_input_yN "import keys to wallet $wallet"; then
    $cleos wallet import -n $wallet --private-key $stakemine_privkey
    $cleos wallet import -n $wallet --private-key $token_privkey
fi

if prompt_input_yN "create accounts [ $stakemine_account $token_account ]"; then
    $cleos system newaccount eosio --buy-ram-kbytes 8 --transfer $stakemine_account $stakemine_pubkey --stake-net "1000.0000 EOS" --stake-cpu "1000.0000 EOS"
    $cleos push action eosio.token transfer '["eosio", "'$stakemine_account'", "10000.0000 EOS", ""]' -p eosio
    $cleos system newaccount eosio --buy-ram-kbytes 8 --transfer $token_account $token_pubkey --stake-net "1000.0000 EOS" --stake-cpu "1000.0000 EOS"
    $cleos push action eosio.token transfer '["eosio", "'$token_account'", "10000.0000 EOS", ""]' -p eosio
fi

if prompt_input_yN "buyram for accounts"; then
    $cleos system buyram $stakemine_account $stakemine_account -k 1024
    $cleos system buyram $token_account $token_account -k 1024
fi

if prompt_input_yN "deploy code to accounts"; then
    $cleos set contract $stakemine_account . build/stakemine.token/stakemine.token.wasm build/stakemine.token/stakemine.token.abi -p $stakemine_account
    $cleos set contract $token_account . build/test.token/test.token.wasm build/test.token/test.token.abi -p $token_account
fi

if prompt_input_yN "updateauth"; then
    $cleos set account permission $stakemine_account active '{"threshold": 1,"keys": [{"key": "'$stakemine_pubkey'","weight": 1}],"accounts": [{"permission":{"actor":"'$stakemine_account'","permission":"eosio.code"},"weight":1}]}' owner -p $stakemine_account
    $cleos set account permission $token_account active '{"threshold": 1,"keys": [{"key": "'$token_pubkey'","weight": 1}],"accounts": [{"permission":{"actor":"'$token_account'","permission":"eosio.code"},"weight":1}]}' owner -p $token_account
fi

if prompt_input_yN "init token stats"; then
    $cleos push action $stakemine_account create '["'$stakemine_account'","10000.0000 '$stakemine_sym'"]' -p $stakemine_account
    $cleos push action $token_account create '["'$token_account'","10000.0000 '$token_sym'"]' -p $token_account
fi

if prompt_input_yN "create $token_account listing"; then
    $cleos push action $stakemine_account list '["'$token_account'","EOS Drama","https://twitter.com/EOSDrama","https://pbs.twimg.com/profile_images/1084455680382242816/BhWR5LK0_400x400.jpg","100","10.0000 EOS","1.0000 '$token_sym'","10.0000 EOS","1.0000 '$token_sym'"]' -p $stakemine_account
    $cleos get table $stakemine_account $stakemine_account listing
fi

if prompt_input_yN "stake to $token_account from $user_account"; then
    $cleos system delegatebw $user_account $token_account "0.0000 EOS" "100.0000 EOS" -p $user_account
    $cleos push action $stakemine_account stake '["'$token_account'","'$user_account'"]' -p $user_account
    $cleos get table $stakemine_account $token_account holders
fi

if prompt_input_yN "stake to $token_account from $user2_account"; then
    $cleos system delegatebw $user2_account $token_account "0.0000 EOS" "100.0000 EOS" -p $user2_account
    $cleos push action $stakemine_account stake '["'$token_account'","'$user2_account'"]' -p $user2_account
    $cleos get table $stakemine_account $token_account holders
fi

if prompt_input_yN "claim $token_account staking rewards from $user_account"; then
    $cleos get currency balance $token_account $user_account $token_symbol
    $cleos push action $token_account claim '["'$user_account'"]' -p $user_account
    $cleos get currency balance $token_account $user_account $token_symbol
fi

if prompt_input_yN "claim $token_account staking rewards from $user2_account"; then
    $cleos get currency balance $token_account $user2_account $token_symbol
    $cleos push action $token_account claim '["'$user2_account'"]' -p $user2_account
    $cleos get currency balance $token_account $user2_account $token_symbol
fi

if prompt_input_yN "unstake to $token_account from $user_account"; then
    $cleos system undelegatebw $user_account $token_account "0.0000 EOS" "100.0000 EOS" -p $user_account
    $cleos push action $stakemine_account unstake '["'$token_account'","'$user_account'"]' -p $user_account
    $cleos get table eosio $user_account delband
    $cleos get table $stakemine_account $token_account holders
fi

if prompt_input_yN "unstake to $token_account from $user2_account"; then
    $cleos system undelegatebw $user2_account $token_account "0.0000 EOS" "100.0000 EOS" -p $user2_account
    $cleos push action $stakemine_account unstake '["'$token_account'","'$user2_account'"]' -p $user2_account
    $cleos get table eosio $user2_account delband
    $cleos get table $stakemine_account $token_account holders
fi

