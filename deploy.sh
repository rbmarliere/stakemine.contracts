#!/bin/bash

account=stakemine112
account_test=accountnum13
cleos="cleos -u http://10.197.70.202:8888"
privkey=5KiK5Ds4LY9159Km5GPWrrR69iEz3HPf6bpUAYKCugFpPoqXcaC
pubkey=EOS8aKH63HLph6Z4wMSvZ5sY67wWzY1mAZ8ZfiWBrymSMYnQ7iMm4
symbol=STK
wallet=local
walletpw=$(cat ~/eosio-wallet/local.passwd)

prompt_input_yN()
{
    printf "$1? [y|N] " ; shift
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

if prompt_input_yN "import $account key to wallet"; then
    $cleos wallet import -n $wallet --private-key $privkey
fi

if prompt_input_yN "create $account account"; then
    set -x
    $cleos get info
    $cleos system newaccount eosio --buy-ram-kbytes 8 --transfer $account $pubkey --stake-net "10.0000 EOS" --stake-cpu "10.0000 EOS"
    $cleos push action eosio.token transfer '["eosio", "'$account'", "10000.0000 EOS", ""]' -p eosio
    set +x
fi

if prompt_input_yN "buyram to $account account"; then
    $cleos system buyram $account $account -k 250 || return 1
    echo
fi

if prompt_input_yN "deploy code to $account account"; then
    $cleos set contract $account . build/stakemine.token/stakemine.token.wasm build/stakemine.token/stakemine.token.abi -p $account
fi

if prompt_input_yN "init $account token stats"; then
    $cleos push action $account create '["'$account'","10000.0000 '$symbol'"]' -p $account
fi

if prompt_input_yN "stake $account from $account_test"; then
    $cleos system delegatebw $account_test $account "0.0000 EOS" "1.0000 EOS" -p $account_test
    $cleos push action $account stake '["'$account_test'"]' -p $account_test
    $cleos get table $account $account_test stake
fi

if prompt_input_yN "claim $account staking rewards from $account_test"; then
    $cleos get currency balance $account $account_test $symbol
    $cleos push action $account claim '["'$account_test'"]' -p $account_test
    $cleos get currency balance $account $account_test $symbol
fi

if prompt_input_yN "unstake $account from $account_test"; then
    $cleos system undelegatebw $account_test $account "0.0000 EOS" "1.0000 EOS" -p $account_test
    $cleos push action $account unstake '["'$account_test'"]' -p $account_test
    $cleos get table eosio $account_test delband
    $cleos get table $account $account_test stake
fi

