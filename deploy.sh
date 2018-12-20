#!/bin/bash

cleos="cleos -u http://10.197.70.202:8888"
privkey=5KiK5Ds4LY9159Km5GPWrrR69iEz3HPf6bpUAYKCugFpPoqXcaC
pubkey=EOS8aKH63HLph6Z4wMSvZ5sY67wWzY1mAZ8ZfiWBrymSMYnQ7iMm4
account=darmatoken11
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
    $cleos set contract $account . build/darma.token/darma.token.wasm build/darma.token/darma.token.abi -p $account
fi

if prompt_input_yN "init $account token stats"; then
    $cleos push action $account create '["'$account'","10000.0000 DRAMA"]' -p $account
fi

if prompt_input_yN "stake $account from accountnum11"; then
    $cleos system delegatebw accountnum11 $account "0.0000 EOS" "1.0000 EOS" -p accountnum11
    $cleos push action $account stake '["accountnum11"]' -p accountnum11
fi

